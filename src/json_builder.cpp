#include <sstream>

#include "json_builder.h"

namespace json_builder {

// * Get JSON of error: Page not found
json::value GetPageNotFound () {
    json::object obj;

    obj [ json_fields::RESPONSE_STATUS_CODE ] = "pageNotFound";
    obj [ json_fields::RESPONSE_MESSAGE ] = "pageNotFound";

    return json::value(obj);
}

std::string GetPageNotFound_s () {
    return json::serialize(GetPageNotFound());
}

// * Get JSON of error: Bad request
json::value GetBadRequest () {
    json::object obj;

    obj [ json_fields::RESPONSE_STATUS_CODE ] = "badRequest";
    obj [ json_fields::RESPONSE_MESSAGE ] = "Bad request";

    return json::value(obj);
}

std::string GetBadRequest_s () {
    return json::serialize(GetBadRequest());
}

// * Get JSON that inform about json parse error
json::value GetParseJsonError(std::string_view message) {

    json::object obj;

    obj [ json_fields::RESPONSE_STATUS_CODE ] = "invalidArgument";
    obj [ json_fields::RESPONSE_MESSAGE ] = message.size() != 0 ? message.data() : "JSON parse error";

    return json::value(obj);
}
std::string GetParseJsonError_s(std::string_view message) {

    return json::serialize(GetParseJsonError(message));
}

// * Get JSON with method not allowed
json::value GetMethodNotAllowed(std::string_view methods) {

    json::object obj;

    std::stringstream ss;

    ss << "Only " << methods << " method is expected";

    obj [ json_fields::RESPONSE_STATUS_CODE ] = "invalidMethod";
    obj [ json_fields::RESPONSE_MESSAGE ] = ss.str();

    return json::value(obj);
}
std::string GetMethodNotAllowed_s(std::string_view methods) {
    return json::serialize(GetMethodNotAllowed(methods));
} 

// * Get JSON Invalid argument
json::value GetInvalidArgument(std::string_view msg) {

    json::object obj;

    obj [ json_fields::RESPONSE_STATUS_CODE ] = "invalidArgument";
    obj [ json_fields::RESPONSE_MESSAGE ] = msg.data();

    return json::value(obj);
}
std::string GetInvalidArgument_s(std::string_view msg) {

    return json::serialize(GetInvalidArgument(msg));
}

// * Get JSON of error: Map Not Found
json::value GetMapNotFound () {
    json::object obj;

    obj [ json_fields::RESPONSE_STATUS_CODE ] = "mapNotFound";
    obj [ json_fields::RESPONSE_MESSAGE ] = "Map not found";

    return json::value(obj);
}

std::string GetMapNotFound_s () {
    return json::serialize(GetMapNotFound());
}

// * Get JSON of maps
json::value GetMaps (const std::vector<model::Map> & maps) {
    json::array maps_array;

    for (const model::Map & map : maps) {

        json::object map_obj;
        map_obj [ json_fields::MAP_ID ] = *map.GetId();
        map_obj [ json_fields::MAP_NAME ] = map.GetName();

        maps_array.emplace_back(map_obj);

    }

    return json::value(maps_array);
}

std::string GetMaps_s (const std::vector<model::Map> & maps) {
    return json::serialize(GetMaps(maps));
}

// * Get JSON of this map
json::value GetMap(const model::Map & map) {
    json::object map_obj;

    map_obj [ json_fields::MAP_ID ] = *map.GetId();
    map_obj [ json_fields::MAP_NAME ] = map.GetName();

    json::array roads_array;
    json::array buildings_array;
    json::array offices_array;

    for (model::Road road : map.GetRoads()) {

        json::object obj;

        obj [ json_fields::ROAD_START_X_POS ] = road.GetStart().x;
        obj [ json_fields::ROAD_START_Y_POS ] = road.GetStart().y;

        if (road.IsHorizontal()) {
            obj [ json_fields::ROAD_END_X_POS ] = road.GetEnd().x;
        } else {
            obj [ json_fields::ROAD_END_Y_POS ] = road.GetEnd().y;
        }

        roads_array.emplace_back(obj);
    }

    for (model::Building building : map.GetBuildings()) {

        json::object obj;

        obj [ json_fields::BUILDING_POS_X ] = building.GetBounds().position.x;
        obj [ json_fields::BUILDING_POS_Y ] = building.GetBounds().position.y;
        obj [ json_fields::BUILDING_WIDTH ] = building.GetBounds().size.width;
        obj [ json_fields::BUILDING_HEIGHT ] = building.GetBounds().size.height;

        buildings_array.emplace_back(obj);
    }

    for (model::Office office : map.GetOffices()) {

        json::object obj;

        obj [ json_fields::OFFICE_ID ] = *office.GetId();

        obj [ json_fields::OFFICE_POS_X ] = office.GetPosition().x;
        obj [ json_fields::OFFICE_POS_Y ] = office.GetPosition().y;
        obj [ json_fields::OFFICE_OFFSET_X ] = office.GetOffset().dx;
        obj [ json_fields::OFFICE_OFFSET_Y ] = office.GetOffset().dy;

        offices_array.emplace_back(obj);
    }

    map_obj [ json_fields::MAP_ROADS_LIST ] = roads_array;
    map_obj [ json_fields::MAP_BUILDINGS_LIST ] = buildings_array;
    map_obj [ json_fields::MAP_OFFICES_LIST ] = offices_array;

    return json::value(map_obj);
}

std::string GetMap_s(const model::Map & map) {
    return json::serialize(GetMap(map));
}

json::value GetMapWithExtraData(const model::Map & map, const extra_data::MapExtraData & extra_data) {
    json::value val = GetMap(map);
    val.as_object()[ json_fields::MAP_LOOT_TYPES ] = extra_data.GetMapLootTypes();

    return val;
}

std::string GetMapWithExtraData_s(const model::Map & map, const extra_data::MapExtraData & extra_data) {
    return json::serialize(GetMapWithExtraData(map, extra_data));
}

// * Get JSON that keep token and player id
json::value GetTokenAndPlayerId(std::string_view token, int player_id) {

    json::object obj;

    obj [json_fields::AUTH_TOKEN] = token.data();
    obj [json_fields::PLAYER_ID] = player_id;

    return json::value(obj);
}
std::string GetTokenAndPlayerId_s(std::string_view token, int player_id) {

    return json::serialize(GetTokenAndPlayerId(token, player_id));
}

// * Get JSON unknown token
json::value GetUnknownToken() {
    json::value val = {{"code", "unknownToken"}, {"message", "Player token has not been found"}};
    return val;
}
std::string GetUnknownToken_s() {
    return json::serialize(GetUnknownToken());
}

// * Get JSON invalid token
json::value GetInvalidToken() {
    json::value val = {{"code", "invalidToken"}, {"message", "Authorization header is missing or not incorrect"}};
    return val;
}
std::string GetInvalidToken_s() {
    return json::serialize(GetInvalidToken());
}

// * Get players list
json::value GetPlayers(model::GameSession * session) {

    json::object obj_players;

    for (app::Player & p : app::PlayersManager::Instance().GetPlayers()) {

        if (session == p.GetSession()) {
            json::object obj_player_data;

            obj_player_data [ json_fields::PLAYER_NAME ] = p.GetPlayerName().data();

            obj_players [ std::to_string(p.GetPlayerId()) ] = obj_player_data;
        }
    }

    return json::value(obj_players);
}
std::string GetPlayers_s(model::GameSession * session) {

    return json::serialize(GetPlayers(session));
}

// * Get players info
json::value GetPlayersInfo(model::GameSession * session) {

    json::object obj_state;
    json::object obj_players;
    json::object obj_items;

    for (app::Player & p : app::PlayersManager::Instance().GetPlayers()) {

        if (session == p.GetSession()) {

            json::object obj_player_data;

            model::Dog * dog = session->GetDogById(p.GetPlayerId());

            json::array pos_arr;
            pos_arr.emplace_back(dog->GetPosition().x);
            pos_arr.emplace_back(dog->GetPosition().y);

            json::array speed_arr;
            speed_arr.emplace_back(dog->GetSpeed().x);
            speed_arr.emplace_back(dog->GetSpeed().y);

            obj_player_data [ json_fields::PLAYER_POSITOIN ] = pos_arr;
            obj_player_data [ json_fields::PLAYER_SPEED ] = speed_arr;
            obj_player_data [ json_fields::PLAYER_SCORES ] = p.GetScores();

            switch (dog->GetDirection()) {
                case model::Direction::NORTH: obj_player_data [ json_fields::PLAYER_MOVE_DIRECTION ] = "U"; break;
                case model::Direction::SOUTH: obj_player_data [ json_fields::PLAYER_MOVE_DIRECTION ] = "D"; break;
                case model::Direction::WEST: obj_player_data [ json_fields::PLAYER_MOVE_DIRECTION ] = "L"; break;
                case model::Direction::EAST: obj_player_data [ json_fields::PLAYER_MOVE_DIRECTION ] = "R"; break;
                default: obj_player_data [ json_fields::PLAYER_MOVE_DIRECTION ] = ""; break;
            }

            json::array obj_player_items_data;

            for (model::Item item : dog->GetItems()) {
                json::object obj_player_item_data;

                obj_player_item_data [ json_fields::ITEM_ID ] = item.GetId();
                obj_player_item_data [ json_fields::ITEM_TYPE ] = item.GetType().GetType();
                obj_player_items_data.emplace_back(obj_player_item_data);
            }
            
            obj_player_data [ json_fields::PLAYER_INVENTORY ] = obj_player_items_data;
            obj_players[ std::to_string(p.GetPlayerId()) ] = obj_player_data;
            
        }
    }

    for (model::Item & item : session->GetItems()) {
        json::object obj_item_data;

        json::array item_pos;
        item_pos.emplace_back((float)item.GetPosition().x);
        item_pos.emplace_back((float)item.GetPosition().y);

        obj_item_data [ json_fields::ITEM_TYPE ] = item.GetType().GetType();
        obj_item_data [ json_fields::ITEM_POSITION ] = item_pos;

        obj_items [ std::to_string(item.GetId()) ] = obj_item_data;
    }

    obj_state[ json_fields::PLAYERS_LIST ] = obj_players;
    obj_state[ json_fields::MAP_LOOT ] = obj_items;

    return json::value(obj_state);
}

std::string GetPlayersInfo_s(model::GameSession * session) {

    return json::serialize(GetPlayersInfo(session));
}

// * namespace json__build
}