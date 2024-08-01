#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/archive/polymorphic_text_iarchive.hpp>

#include <filesystem>
#include <sstream>

#include "../src/save_manager.h"
#include "../src/serializer.h"
#include "../src/model.h"
#include "../src/app.h"
#include "../src/json_loader.h"

namespace fs = std::filesystem;
using namespace Catch::Matchers;

static constexpr char AUTOSAVE_FILE_PATH[] = "saved_data";

SCENARIO("Test serialization") {
    GIVEN("Game object with one session and player is added by PlayerManager") {
        fs::path config_file_path{"config.json"};
        model::Game game = json_loader::LoadGame(config_file_path);

        auto session = game.NewSession(const_cast<model::Map *>(game.FindMap(model::Map::Id{"town"})));

        app::PlayersManager::Instance().AddNewPlayer("Test", session);

        WHEN("Init GameSerializationProvider object") {
            serializer::GameSerializationProvider game_provider(game);
            serializer::PlayersManagerSerializationProvider players_manager_provider(app::PlayersManager::Instance());

            serializer::SerializationProvider ser_provider(game_provider, players_manager_provider);

            WHEN ("Try serialize it") {
                std::stringstream ss;

                boost::archive::polymorphic_text_oarchive oa{ss};
                oa << ser_provider;

                AND_THEN("Try save serialized game object") {
                    save_manager::SaveToFile(AUTOSAVE_FILE_PATH, ss.str());

                    REQUIRE(fs::is_regular_file(AUTOSAVE_FILE_PATH));
                }
            }
        }

    }

    GIVEN("Autosave file") {
        WHEN("Try load saved data from file") {
            std::stringstream ss(save_manager::LoadSavedFile(AUTOSAVE_FILE_PATH));
            boost::archive::polymorphic_text_iarchive ia(ss);

            AND_THEN("Parse data and insert it into object") {
                serializer::SerializationProvider ser_provider;

                ia >> ser_provider;

                CHECK(ser_provider.game_ser_provider.sessions_providers.size() == 1);
                CHECK(ser_provider.players_manager_ser_provider.players_providers.size() == 1);
            }
        }
    }
}