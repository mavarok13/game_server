#include "serializer.h"
#include "save_manager.h"

std::string serializer::SerializeGame(model::Game& game) {
    std::stringstream ss;

    serializer::GameSerializationProvider game_ser_provider(game);
    serializer::PlayersManagerSerializationProvider player_manager_ser_provider(app::PlayersManager::Instance());

    serializer::SerializationProvider ser_provider(game_ser_provider, player_manager_ser_provider);

    boost::archive::polymorphic_text_oarchive oa{ss};

    oa << ser_provider;

    return ss.str();
}


void serializer::DeserializeGame(std::string serialized_data, model::Game & game) {
    if (serialized_data.empty()) {
        return;
    }

    std::stringstream ss;
    ss << serialized_data;
    boost::archive::polymorphic_text_iarchive ia(ss);

    serializer::SerializationProvider ser_provider;

    ia >> ser_provider;

    serializer::GameSerializationProvider game_provider = ser_provider.game_ser_provider;
    serializer::PlayersManagerSerializationProvider players_manager_provider = ser_provider.players_manager_ser_provider;

    app::PlayersManager::Instance().SetNextPlayerId(players_manager_provider.player_id);

    for (serializer::GameSessionSerializationProvider & session_ser_provider : game_provider.sessions_providers) {
        model::Map * map = const_cast<model::Map *>(game.FindMap(model::Map::Id{session_ser_provider.map_id}));

        model::GameSession * session = game.NewSession(map);

        for (serializer::DogSerializationProvider & dog_ser_provider : session_ser_provider.dogs_providers) {
            model::Dog dog(dog_ser_provider.id, model::Vector2{dog_ser_provider.x, dog_ser_provider.y});

            for (serializer::ItemSerializationProvider item_ser_provider : dog_ser_provider.inventory_provider) {
                for (const model::ItemType & type : map->GetItemsTypes()) {
                    if (type.GetType() == item_ser_provider.type_id) {
                        model::Item item(item_ser_provider.id, type, model::Point{item_ser_provider.x, item_ser_provider.y});
                        dog.AddItem(item);
                    }
                }
            }

            session->GetDogs().emplace_back(dog);
        }

        for (serializer::ItemSerializationProvider & item_ser_provider : session_ser_provider.items_providers) {
            for (const model::ItemType & type : map->GetItemsTypes()) {
                if (type.GetType() == item_ser_provider.type_id) {
                    model::Item item(item_ser_provider.id, type, model::Point{item_ser_provider.x, item_ser_provider.y});
                    session->GetItems().emplace_back(item);
                }
            }
        }

        for (serializer::PlayerSerializationProvider & player_ser_provider : players_manager_provider.players_providers) {
            if (player_ser_provider.session_id == session_ser_provider.id) {
                app::Player player(player_ser_provider.token, player_ser_provider.player_id, player_ser_provider.player_name, player_ser_provider.session_id);
                player.AddScores(player_ser_provider.scores);
                app::PlayersManager::Instance().GetPlayers().emplace_back(player);
            }
        }
    }
}