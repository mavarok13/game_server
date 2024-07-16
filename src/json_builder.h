#pragma once

#include <boost/json.hpp>

#include "model.h"
#include "app.h"
#include "extra_data.h"
#include "json_fields.h"

namespace json_builder {

namespace json = boost::json;

// * Get Page not found
json::value GetPageNotFound ();
std::string GetPageNotFound_s ();

// * Get JSON of error: Bad request
json::value GetBadRequest ();
std::string GetBadRequest_s ();

// * Get JSON that inform about json parse error
json::value GetParseJsonError(std::string_view message = "");
std::string GetParseJsonError_s(std::string_view message = ""); 

// * Get JSON with method not allowed
json::value GetMethodNotAllowed(std::string_view methods); 
std::string GetMethodNotAllowed_s(std::string_view methods);

// * Get JSON Invalid argument
json::value GetInvalidArgument(const char * msg);
std::string GetInvalidArgument_s(const char * msg);

// * Get JSON of error: Map Not Found
json::value GetMapNotFound ();
std::string GetMapNotFound_s ();

// * Get JSON of maps
json::value GetMaps (const std::vector<model::Map> & maps);
std::string GetMaps_s (const std::vector<model::Map> & maps);

// * Get JSON of this map
json::value GetMap(const model::Map & map);
std::string GetMap_s(const model::Map & map);
json::value GetMapWithExtraData(const model::Map & map, const extra_data::MapExtraData & extra_data);
std::string GetMapWithExtraData_s(const model::Map & map, const extra_data::MapExtraData & extra_data);

// * Get JSON that keep token and player id
json::value GetTokenAndPlayerId(std::string_view token, int player_id);
std::string GetTokenAndPlayerId_s(std::string_view token, int player_id);

// * Get players list
json::value GetPlayers(model::GameSession * session);
std::string GetPlayers_s(model::GameSession * session);

// * Get players info
json::value GetPlayersInfo(model::GameSession * session);
std::string GetPlayersInfo_s(model::GameSession * session);

// * namespace json__build
}