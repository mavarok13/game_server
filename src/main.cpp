//TO DO:
// fix copy when emplace_back session to game.sessions_

#include "sdk.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <thread>
#include <memory>
#include <signal.h>
#include <optional>
#include <filesystem>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json.hpp>
#include <boost/program_options.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/archive/polymorphic_text_iarchive.hpp>

#include "app.h"
#include "database.h"
#include "collision_detector.h"
#include "command_line_args.h"
#include "model.h"
#include "loot_generator.h"
#include "update_items.h"
#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include "ticker.h"
#include "extra_data.h"
#include "serializer.h"
#include "save_manager.h"

using namespace std::literals;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace sys = boost::system;
namespace json = boost::json;
namespace fs = std::filesystem;

using HttpRequest = beast::http::request<beast::http::string_body>;
using HttpResponse = beast::http::response<beast::http::string_body>;

using Strand = net::strand<net::io_context::executor_type>;

namespace {

static constexpr char DB_CONNECTION_URL_ENV_VAR[] = "GAME_DB_URL";

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned threads_count, const Fn& fn) {
    threads_count = std::max(1u, threads_count);
    std::vector<std::jthread> workers;
    workers.reserve(threads_count - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--threads_count) {
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
    add("save-state-period", po::value(&args.autosave_period)->value_name("milliseconds"), "set autosave period");
    add("config-file,c", po::value(&args.config_file)->value_name("file"), "set config file path");   
    add("www-root,w", po::value(&args.wwwroot_dir)->value_name("dir"), "set static files root");
    add("state-file", po::value(&args.save_file_path)->value_name("file"), "autosave file path");
    add("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s) || vm.empty()) {
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file")) throw std::runtime_error("Config file has not been specified");
    if (!vm.contains("www-root")) throw std::runtime_error("Static files have not been specified");
    if (!vm.contains("save-state-period")) args.autosave_period = -1;
    args.random_position = vm.contains("randomize-spawn-points");
    args.no_tick_period = !vm.contains("tick-period");

    return args;
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
//  *   PARSE COMMAND LINE
        auto args = ParseCommandLine(argc, argv);
        if (!args) { throw std::runtime_error("Application cannot start"); }

        std::string root = args->wwwroot_dir;

//  *   LOGGING SETUP
        SetupConsoleLogging();

//  *   LOAD GAME CONFIG
        std::shared_ptr<model::Game> game = std::make_shared<model::Game>(json_loader::LoadGame(args->config_file));

        if (!args->save_file_path.empty() && fs::is_regular_file(args->save_file_path)) {
            std::string serialized_data = save_manager::LoadSavedFile(args->save_file_path);
            serializer::DeserializeGame(serialized_data, *game);
        }

        std::vector<extra_data::MapExtraData> maps_extra_data = json_loader::GetMapsExtraData(args->config_file);

        game->SetRandomPosition(args->random_position);
        game->SetTimerStopped(args->no_tick_period);

// *    LOOT_GENERATOR
        loot_gen::LootGenerator lg(std::chrono::milliseconds{(int)game->GetLootSpawnPeriod()*1000}, game->GetLootSpawnProbability());

// *    CREATE IO CONTEXT
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        std::shared_ptr<Strand> strand = std::make_shared<Strand>(net::make_strand(ioc));

// *    SETUP AUTOSAVE
        app::Application application;
        save_manager::Saver saver(std::chrono::milliseconds(args->autosave_period), [game_ptr = std::shared_ptr<model::Game>{game}, &args, strand] () {
            if (!args->save_file_path.empty()) {
                net::dispatch(*strand, [game_ptr = std::shared_ptr<model::Game>(game_ptr), &args] {
                    std::string ser_data = serializer::SerializeGame(*game_ptr);
                    save_manager::SaveToFile(args->save_file_path, ser_data);
                });
            }
        });

//  *   SIGNALS HANDLER
        net::signal_set signal_set(ioc, SIGINT, SIGTERM);

        signal_set.async_wait([&ioc, &args, &saver] (const sys::error_code & ec, int signal_number) {
            if (!args->save_file_path.empty()) {
                saver.Save();
            }

            if (ec) {
                BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", EXIT_FAILURE }, {"exception", ec.what()}}) << logging::add_value(message_, "server exited");
            }

            BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", 0 }}) << logging::add_value(message_, "server exited");

            ioc.stop();
        });


//  *   INIT DB
        std::shared_ptr<database::ConnectionPool> connection_pool = std::make_shared<database::ConnectionPool>(4u, std::getenv(DB_CONNECTION_URL_ENV_VAR));
        std::shared_ptr<app::IUnitOfWorkFactory> unit_of_work_factory = std::make_shared<database::DbUnitOfWorkFactory>(connection_pool);

//  *   SUBSCRIBE HANDLER FOR TICKER
        application.DoOnTick([&saver, game, strand_ptr = std::shared_ptr<Strand>(strand), &unit_of_work_factory] (std::chrono::milliseconds delta_time) {
            auto unit_of_work = unit_of_work_factory->NewUnitOfWork();

            for (model::GameSession & session : game->GetSessions()) {
                std::vector<int> dogs_to_delete;

                for (model::Dog & dog : session.GetDogs()) {
                    app::Player * player = app::PlayersManager::Instance().GetPlayerById(dog.GetId());
                    player->AddPlayingTime(delta_time);
                    if (dog.GetSpeed() == model::Vector2{0, 0}) {
                        player->AddIdleTime(delta_time);

                        if (player->GetIdleTime() >= std::chrono::duration<double>(game->GetDogIdleTimeThreshold())) {
                            app::GameResult result{player->GetPlayerName(), player->GetScores(), static_cast<unsigned int>(player->GetPlayingTime().count())};
                            unit_of_work->Results()->AddResult(result);

                            dogs_to_delete.emplace_back(player->GetPlayerId());
                            app::PlayersManager::Instance().RemovePlayer(player->GetPlayerId());
                        }
                    } else {
                        player->ResetIdleTime();
                    }
                }

                for (int & dog_id : dogs_to_delete) {
                    session.RemoveDogById(dog_id);
                }
            }

            unit_of_work->Commit();

            saver.Save(delta_time);
        });

//  *   CREATE REQUEST HANDLER
        http_handler::RequestHandler handler{game, application, lg, unit_of_work_factory, maps_extra_data, strand, root};

//  *   LISTEN AND WAIT FOR NEW CONNECTION
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [&handler, &ioc](auto&& req, auto&& send) {
            handler(std::move(req), send);
        });

//  *   TICKER
        if (!args->no_tick_period) {
            std::make_shared<ticker::Ticker>(ticker::Ticker(strand, std::chrono::milliseconds(args->tick_period), [game, &application, &lg] (int interval) {
                application.Tick(std::chrono::milliseconds(interval));
                game->Tick(interval, [game, &lg, interval] {
                    for (model::GameSession & session : game->GetSessions()) {
//  *   *   *   *   *   Generate new items on the session map
                        unsigned int spawned_items = lg.Generate(std::chrono::milliseconds(interval), session.GetItems().size(), session.GetDogs().size()); 
                        session.AddItems(spawned_items);

                        collision_detector::UpdateSessionItems(session, interval);
                    }
                });
            }))->Start();

        }
        
        BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{"address", address.to_string() }, { "port", port }}) << logging::add_value(message_, "server started");

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        if (!args->save_file_path.empty()) {
            std::string ser_data = serializer::SerializeGame(*game);
            save_manager::SaveToFile(args->save_file_path, ser_data);
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
