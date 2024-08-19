#pragma once

#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/archive/polymorphic_text_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>

#include "model.h"
#include "app.h"

namespace serializer {

class ItemSerializationProvider {
public:
    ItemSerializationProvider() {}

    explicit ItemSerializationProvider(const model::Item & item) {
        id = item.GetId();
        type_id = item.GetType().GetType();
        x = item.GetPosition().x;
        y = item.GetPosition().y;
    }

    int id;
    int type_id;
    int x;
    int y;
private:
    friend class boost::serialization::access;

    void serialize(boost::archive::polymorphic_oarchive & oa, [[maybe_unused]] const unsigned int version) {
        oa & id;
        oa & type_id;
        oa & x;
        oa & y;
    }

    void serialize(boost::archive::polymorphic_iarchive & ia, [[maybe_unused]] const unsigned int version) {
        ia & id;
        ia & type_id;
        ia & x;
        ia & y;
    }
};

class DogSerializationProvider {
public:
    DogSerializationProvider() {}

    explicit DogSerializationProvider(const model::Dog & dog) {
        id = dog.GetId();
        x = dog.GetPosition().x;
        y = dog.GetPosition().y;

        for (const model::Item & item : dog.GetItems()) {
            inventory_provider.emplace_back(item);
        }
    }

    unsigned int id;
    double x;
    double y;
    std::vector<ItemSerializationProvider> inventory_provider;
private:
    friend class boost::serialization::access;

    void serialize(boost::archive::polymorphic_oarchive & oa, [[maybe_unused]] const unsigned int version) {
        oa & id;
        oa & x;
        oa & y;
        oa & inventory_provider;
    }

    void serialize(boost::archive::polymorphic_iarchive & ia, [[maybe_unused]] const unsigned int version) {
        ia & id;
        ia & x;
        ia & y;
        ia & inventory_provider;
    }
};

class GameSessionSerializationProvider {
public:
    GameSessionSerializationProvider() {}

    explicit GameSessionSerializationProvider(model::GameSession & game_session) {
        id = game_session.GetId();

        for (const model::Dog & dog : game_session.GetDogs()) {
            dogs_providers.emplace_back(dog);
        }

        for (const model::Item & item : game_session.GetItems()) {
            items_providers.emplace_back(item);
        }

        map_id = *(game_session.GetMap()->GetId());
    }

    unsigned int id;
    std::vector<DogSerializationProvider> dogs_providers;
    std::vector<ItemSerializationProvider> items_providers;
    std::string map_id;
private:
    friend class boost::serialization::access;

    void serialize(boost::archive::polymorphic_oarchive & oa, [[maybe_unused]] const unsigned int version) {
        oa & id;
        oa & dogs_providers;
        oa & items_providers;
        oa & map_id;
    }

    void serialize(boost::archive::polymorphic_iarchive & ia, [[maybe_unused]] const unsigned int version) {
        ia & id;
        ia & dogs_providers;
        ia & items_providers;
        ia & map_id;
    }
};

class GameSerializationProvider {
public:
    GameSerializationProvider() {}

    explicit GameSerializationProvider(model::Game & game) {
        for (model::GameSession & session : game.GetSessions()) {
            sessions_providers.emplace_back(session);
        }
    }

    std::vector<GameSessionSerializationProvider> sessions_providers;
private:
    friend class boost::serialization::access;

    void serialize(boost::archive::polymorphic_oarchive & oa, [[maybe_unused]] const unsigned int version) {
        oa & sessions_providers;
    }

    void serialize(boost::archive::polymorphic_iarchive & ia, [[maybe_unused]] const unsigned int version) {
        ia & sessions_providers;
    }
};

class PlayerSerializationProvider {
public:
    PlayerSerializationProvider() {}

    explicit PlayerSerializationProvider(const app::Player & player) {
        token = player.GetToken();
        player_id = player.GetPlayerId();
        player_name = player.GetPlayerName();
        session_id = player.GetSessionId();
        scores = player.GetScores();
    }

    std::string token;
    int player_id;
    std::string player_name;
    unsigned int session_id;
    unsigned int scores;

private:
    friend class boost::serialization::access;

    void serialize(boost::archive::polymorphic_oarchive & oa, [[maybe_unused]] const unsigned int version) {
        oa & token;
        oa & player_id;
        oa & player_name;
        oa & session_id;
        oa & scores;
    }

    void serialize(boost::archive::polymorphic_iarchive & ia, [[maybe_unused]] const unsigned int version) {
        ia & token;
        ia & player_id;
        ia & player_name;
        ia & session_id;
        ia & scores;
    }
};

class PlayersManagerSerializationProvider {
public:
    PlayersManagerSerializationProvider() {}

    explicit PlayersManagerSerializationProvider(const app::PlayersManager & player_manager) {
        for (const app::Player & player : player_manager.GetPlayers()) {
            players_providers.emplace_back(player);
        }

        player_id = player_manager.GetNextPlayerId();
    }

    std::vector<PlayerSerializationProvider> players_providers;
    int player_id;

private:
    friend class boost::serialization::access;

    void serialize(boost::archive::polymorphic_oarchive & oa, [[maybe_unused]] const unsigned int version) {
        oa & players_providers;
        oa & player_id;
    }

    void serialize(boost::archive::polymorphic_iarchive & ia, [[maybe_unused]] const unsigned int version) {
        ia & players_providers;
        ia & player_id;
    }
};

class SerializationProvider {
public: 
    SerializationProvider() {}

    SerializationProvider(GameSerializationProvider game_ser_provider, PlayersManagerSerializationProvider players_manager_ser_provider) {
        this->game_ser_provider = game_ser_provider;
        this->players_manager_ser_provider = players_manager_ser_provider;
    }

    GameSerializationProvider game_ser_provider;
    PlayersManagerSerializationProvider players_manager_ser_provider;
private:
    friend class boost::serialization::access;

    void serialize(boost::archive::polymorphic_oarchive & oa, [[maybe_unused]] const unsigned int version) {
        oa & game_ser_provider;
        oa & players_manager_ser_provider;
    }

    void serialize(boost::archive::polymorphic_iarchive & ia, [[maybe_unused]] const unsigned int version) {
        ia & game_ser_provider;
        ia & players_manager_ser_provider;
    }
};

std::string SerializeGame(model::Game& game);
void DeserializeGame(std::string serialized_data, model::Game & game);

} //namespace serializer