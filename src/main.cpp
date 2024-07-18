#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <iostream>
#include <thread>
#include <csignal>
#include <chrono>
#include <optional>
#include <cstdlib>
#include <cmath>

#include "model.h"
#include "app.h"
#include "loot_generator.h"
#include "collision_detector.h"
#include "game_item_player_provider.h"
#include "game_office_player_provider.h"
#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include "ticker.h"
#include "command_line_args.h"
#include "extra_data.h"

using namespace std::literals;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace sys = boost::system;
namespace json = boost::json;

using HttpRequest = beast::http::request<beast::http::string_body>;
using HttpResponse = beast::http::response<beast::http::string_body>;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;

    auto add = desc.add_options();
    add("help,h", "produce help message");
    add("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"), "set tick period");
    add("config-file,c", po::value(&args.config_file)->value_name("file"), "set config file path");   
    add("www-root,w", po::value(&args.wwwroot_dir)->value_name("dir"), "set static files root");   
    add("randomize-spawn-points", "spawn dogs at random positions");   

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file")) throw std::runtime_error("Config file has not been specified");
    if (!vm.contains("www-root")) throw std::runtime_error("Static files have not been specified");
    args.random_position = vm.contains("randomize-spawn-points");
    args.no_tick_period = !vm.contains("tick-period");

    return args;
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
//  *   PARSE COMMAND LINE
        auto args = ParseCommandLine(argc, argv);
        if (!args) { throw std::runtime_error("Start application trouble"); }

        std::string root = args->wwwroot_dir;

//  *   LOGGING SETUP
        SetupConsoleLogging();


//  *   LOAD GAME CONFIG
        model::Game game = json_loader::LoadGame(args->config_file);
        std::vector<extra_data::MapExtraData> maps_extra_data = json_loader::GetMapsExtraData(args->config_file);

        game.SetRandomPosition(args->random_position);
        game.SetTimerStopped(args->no_tick_period);

// *    LOOT_GENERATOR
        loot_gen::LootGenerator lg(std::chrono::milliseconds{(int)game.GetLootSpawnPeriod()*1000}, game.GetLootSpawnProbability());

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signal_set(ioc, SIGINT, SIGTERM);

        signal_set.async_wait([&ioc] (const sys::error_code & ec, int signal_number) {
            
            if (ec) {
                BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", EXIT_FAILURE }, {"exception", ec.what()}}) << logging::add_value(message_, "server exited");
            }

            BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", 0 }}) << logging::add_value(message_, "server exited");

            ioc.stop();
        });


        auto strand = net::make_strand(ioc);

        http_handler::RequestHandler handler{game, lg, maps_extra_data, strand, root};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [&handler, &ioc](auto&& req, auto&& send) {
            handler(std::move(req), send);
        });

        if (!args->no_tick_period) {
            std::make_shared<ticker::Ticker>(ticker::Ticker{strand, std::chrono::milliseconds(args->tick_period), [&game, &lg] (int interval) {
                game.Tick(interval, [&game, &lg, &interval] {
                    for (model::GameSession & session : game.GetSessions()) {
//  *   *   *   *   *   Generate new items on the session map
                        unsigned int spawned_items = lg.Generate(std::chrono::milliseconds(interval), session.GetItems().size(), session.GetDogs().size()); 
                        session.AddItems(spawned_items);

//  *   *   *   *   *   Detect items collision
                        collision_detector::GameItemPlayerProvider items_provider(session.GetItems(), session.GetDogs());
                        std::vector<collision_detector::GatheringEvent> items_events = collision_detector::FindGatherEvents(items_provider);

//  *   *   *   *   *   Detect offices collision
                        collision_detector::GameOfficePlayerProvider offices_provider(session.GetMap()->GetOffices(), session.GetDogs());
                        std::vector<collision_detector::GatheringEvent> offices_events = collision_detector::FindGatherEvents(offices_provider);

//  *   *   *   *   *   Convert local time to global
                        for (collision_detector::GatheringEvent & event : items_events) {
                            model::Dog dog = session.GetDogs()[event.gatherer_id];

                            model::Vector2 move_vector{dog.GetPosition().x - dog.GetPrevPosition().x, dog.GetPosition().y - dog.GetPrevPosition().y};

                            double distance = std::sqrt(move_vector.x*move_vector.x + move_vector.y*move_vector.y);

                            event.time *= distance/(session.GetMap()->GetDogSpeed()*interval);
                        }

                        for (collision_detector::GatheringEvent & event : offices_events) {
                            model::Dog dog = session.GetDogs()[event.gatherer_id];

                            model::Vector2 move_vector{dog.GetPosition().x - dog.GetPrevPosition().x, dog.GetPosition().y - dog.GetPrevPosition().y};

                            double distance = std::sqrt(move_vector.x*move_vector.x + move_vector.y*move_vector.y);

                            event.time *= distance/(session.GetMap()->GetDogSpeed()*interval);
                        }

                        std::sort(items_events.begin(), items_events.end(), collision_detector::CompareGatheringEvents);
                        std::sort(offices_events.begin(), offices_events.end(), collision_detector::CompareGatheringEvents);

//  *   *   *   *   *   Detection handler
                        std::vector<int> removed_items;

                        for (auto item_event = items_events.begin(), office_event = offices_events.begin(); item_event != items_events.end() || office_event != offices_events.end();) {
                            if (office_event == offices_events.end() || item_event != items_events.end() && item_event->time <= office_event->time) {
                                if (session.GetDogs()[item_event->gatherer_id].GetItemsCount() >= session.GetMap()->GetInventorySize()) {
                                    ++item_event;
                                    continue;
                                }

                                if (std::find(removed_items.begin(), removed_items.end(), item_event->item_id) == std::end(removed_items)) {
                                    session.GetDogs()[item_event->gatherer_id].AddItem(session.GetItems()[item_event->item_id]);

                                    removed_items.emplace_back(item_event->item_id);
                                }

                                ++item_event;
                            } else {
                                for (app::Player & player : app::PlayersManager::Instance().GetPlayers()) {
                                    if (player.GetSession() == &session && player.GetPlayerId() == office_event->gatherer_id) {
                                        for (model::Item & item : (session.GetDogs()[office_event->gatherer_id]).GetItems()) {
                                            player.AddScores(item.GetType().GetCost());
                                        }

                                        break;
                                    }
                                }

                                session.GetDogs()[office_event->gatherer_id].PutItems();

                                ++office_event;
                            }
                        }

                        while (removed_items.size() != 0) {
                            session.RemoveItemById((session.GetItems()[*removed_items.begin()]).GetId());
                            removed_items.erase(removed_items.begin());
                        }
                    }
                });
            }})->Start();

        }
        
        BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{"address", address.to_string() }, { "port", port }}) << logging::add_value(message_, "server started");

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
