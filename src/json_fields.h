#pragma once

namespace json_fields {

static constexpr char MAPS_LIST[] = "maps"; 
static constexpr char MAP_ID[] = "id"; 
static constexpr char MAP_NAME[] = "name";
static constexpr char MAP_ROADS_LIST[] = "roads";
static constexpr char MAP_BUILDINGS_LIST[] = "buildings";
static constexpr char MAP_OFFICES_LIST[] = "offices";
static constexpr char MAP_LOOT_TYPES[] = "lootTypes";
static constexpr char MAP_LOOT[] = "lostObjects";

static constexpr char ROAD_START_X_POS[] = "x0";
static constexpr char ROAD_START_Y_POS[] = "y0";
static constexpr char ROAD_END_X_POS[] = "x1";
static constexpr char ROAD_END_Y_POS[] = "y1";

static constexpr char BUILDING_POS_X[] = "x";
static constexpr char BUILDING_POS_Y[] = "y";
static constexpr char BUILDING_WIDTH[] = "w";
static constexpr char BUILDING_HEIGHT[] = "h";

static constexpr char OFFICE_ID[] = "id";
static constexpr char OFFICE_POS_X[] = "x";
static constexpr char OFFICE_POS_Y[] = "y";
static constexpr char OFFICE_OFFSET_X[] = "offsetX";
static constexpr char OFFICE_OFFSET_Y[] = "offsetY";

static constexpr char AUTENTICATE_PLAYER_NAME[] = "userName";
static constexpr char AUTENTICATE_MAP_ID[] = "mapId";

static constexpr char AUTH_TOKEN[] = "authToken";
static constexpr char PLAYERS_LIST[] = "players";
static constexpr char PLAYER_ID[] = "playerId";
static constexpr char PLAYER_NAME[] = "name";
static constexpr char PLAYER_MOVE_DIRECTION[] = "dir";
static constexpr char PLAYER_POSITOIN[] = "pos";
static constexpr char PLAYER_SPEED[] = "speed";
static constexpr char PLAYER_INVENTORY[] = "bag";
static constexpr char PLAYER_SCORES[] = "score";

static constexpr char DEFAULT_DOG_SPEED[] = "defaultDogSpeed";
static constexpr char DOG_SPEED[] = "dogSpeed";
static constexpr char DOG_IDLE_TIME_THRESHOLD[] = "dogRetirementTime";

static constexpr char DEFAULT_INVENTORY_SIZE[] = "defaultBagCapacity";
static constexpr char INVENTORY_SIZE[] = "bagCapacity";

static constexpr char ITEM_ID[] = "id";
static constexpr char ITEM_TYPE[] = "type";
static constexpr char ITEM_POSITION[] = "pos";
static constexpr char ITEM_COST[] = "value";

static constexpr char TOP_GAME_RESULTS_PLAYER_NAME[] = "name";
static constexpr char TOP_GAME_RESULTS_PLAYER_SCORES[] = "score";
static constexpr char TOP_GAME_RESULTS_PLAYER_PLAYING_TIME[] = "playTime";

static constexpr char DB_GAME_RESULTS_OFFSET[] = "start";
static constexpr char DB_GAME_RESULTS_LIMIT[] = "maxItems";

static constexpr char LOOT_SPAWN_CONFIG[] = "lootGeneratorConfig";
static constexpr char LOOT_SPAWN_PERIOD[] = "period";
static constexpr char LOOT_SPAWN_PROBABILITY[] = "probability";
    
static constexpr char RESPONSE_STATUS_CODE[] = "code";
static constexpr char RESPONSE_MESSAGE[] = "message";

}; //namespace json_fields