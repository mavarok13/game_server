#include "json_loader.h"

#include <fstream>
#include <cstring>
#include <iostream>
#include <optional>

namespace json_loader {

void ExtractRoadsToMap(const json::value & val_map, model::Map * map) {
    for (auto val_road : val_map.at(json_fields::MAP_ROADS_LIST).as_array()) {
        int x0 = val_road.at(json_fields::ROAD_START_X_POS).as_int64();
        int y0 = val_road.at(json_fields::ROAD_START_Y_POS).as_int64();

        if (val_road.as_object().contains(json_fields::ROAD_END_X_POS)) {
            int x1 = val_road.at(json_fields::ROAD_END_X_POS).as_int64();
            map->AddRoad({model::Road::HORIZONTAL, {x0, y0}, x1});
        } else if (val_road.as_object().contains(json_fields::ROAD_END_Y_POS)) {
            int y1 = val_road.at(json_fields::ROAD_END_Y_POS).as_int64();
            map->AddRoad({model::Road::VERTICAL, {x0, y0}, y1});
        }
    }
}

void ExtractBuildingsToMap(const json::value & val_map, model::Map * map) {
    for (auto val_buildings : val_map.at(json_fields::MAP_BUILDINGS_LIST).as_array()) {
        int x = val_buildings.at(json_fields::BUILDING_POS_X).as_int64();
        int y = val_buildings.at(json_fields::BUILDING_POS_Y).as_int64();
        int w = val_buildings.at(json_fields::BUILDING_WIDTH).as_int64();
        int h = val_buildings.at(json_fields::BUILDING_HEIGHT).as_int64();

        map->AddBuilding(model::Building{model::Rectangle{{x, y}, {w, h}}});
    }
}

void ExtractOfficesToMap(const json::value & val_map, model::Map * map) {
    for (auto val_offices : val_map.at(json_fields::MAP_OFFICES_LIST).as_array()) {
        std::string id(val_offices.at(json_fields::OFFICE_ID).as_string().c_str());
        int x = val_offices.at(json_fields::OFFICE_POS_X).as_int64();
        int y = val_offices.at(json_fields::OFFICE_POS_Y).as_int64();
        int offset_x = val_offices.at(json_fields::OFFICE_OFFSET_X).as_int64();
        int offset_y = val_offices.at(json_fields::OFFICE_OFFSET_Y).as_int64();

        map->AddOffice(model::Office{model::Office::Id{id}, {x, y}, {offset_x, offset_y}});
    }
}

std::vector<extra_data::MapExtraData> GetMapsExtraData(const std::filesystem::path& json_path) {
    std::ifstream is(json_path.c_str());

    std::string json_str;
    std::string buf;

    if(!is.is_open()) {
        throw std::runtime_error(std::strstr({"Couldn't open file with next path: "}, {json_path.c_str()}));
    }

    while (std::getline(is, buf)) {
        json_str.append(buf);
    }

    auto val = json::parse(json_str);

    std::vector<extra_data::MapExtraData> maps_extra_data;

    for (auto val_map : val.at(json_fields::MAPS_LIST).as_array()) {
        maps_extra_data.emplace_back(extra_data::MapExtraData{{val_map.at(json_fields::MAP_ID).as_string().c_str()}, val_map.at(json_fields::MAP_LOOT_TYPES)});
    }

    return maps_extra_data;
}

std::optional<unsigned int> GetGameResultsTableOffset(std::string_view json) {
    try {
        std::string json_str{json.begin(), json.end()};
        json::value val = json::parse(json_str);

        if (val.as_object().contains( json_fields::DB_GAME_RESULTS_OFFSET )) {
            return std::make_optional<unsigned int>(val.at( json_fields::DB_GAME_RESULTS_OFFSET ).as_int64());
        } else {
            return std::nullopt;
        }
    } catch (...) {
        return std::nullopt;
    }
}
std::optional<unsigned int> GetGameResultsTableLimit(std::string_view json) {
    try {
        std::string json_str{json.begin(), json.end()};
        json::value val = json::parse(json_str);

        if (val.as_object().contains( json_fields::DB_GAME_RESULTS_LIMIT )) {
            return std::make_optional<unsigned int>(val.at( json_fields::DB_GAME_RESULTS_LIMIT ).as_int64());
        } else {
            return std::nullopt;
        }
    } catch (...) {
        return std::nullopt;
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream is(json_path.c_str());

    std::string json_str;
    std::string buf;

    if(!is.is_open()) {
        throw std::runtime_error(std::strstr({"Couldn't open file with next path: "}, {json_path.c_str()}));
    }

    while (std::getline(is, buf)) {
        json_str.append(buf);
    }

    auto val = json::parse(json_str);

    model::Game game;

    json::value lootSpawnOptions = val.at(json_fields::LOOT_SPAWN_CONFIG);

    double lootSpawnPeriod = lootSpawnOptions.at(json_fields::LOOT_SPAWN_PERIOD).as_double();
    double lootSpawnProbability = lootSpawnOptions.at(json_fields::LOOT_SPAWN_PROBABILITY).as_double();

    game.SetLootSpawnPeriod(lootSpawnPeriod);
    game.SetLootSpawnProbability(lootSpawnProbability);

    double default_dog_speed = val.at(json_fields::DEFAULT_DOG_SPEED).as_double();
    unsigned int default_inventory_size = val.at(json_fields::DEFAULT_INVENTORY_SIZE).as_int64();

    if (val.as_object().contains( json_fields::DOG_IDLE_TIME_THRESHOLD )) {
        game.SetDogIdleTimeThreshold(val.at( json_fields::DOG_IDLE_TIME_THRESHOLD ).as_double());
    }

    for (auto val_map : val.at(json_fields::MAPS_LIST).as_array()) {
        model::Map map{model::Map::Id{val_map.at(json_fields::MAP_ID).as_string().c_str()}, val_map.at(json_fields::MAP_NAME).as_string().c_str()};

        ExtractRoadsToMap(val_map, &map);
        ExtractBuildingsToMap(val_map, &map);
        ExtractOfficesToMap(val_map, &map);

        json::array loot_types = val_map.at(json_fields::MAP_LOOT_TYPES).as_array();

        for (int loot_type_id = 0; loot_type_id < loot_types.size(); ++loot_type_id) {
            map.AddItemType(model::ItemType{loot_type_id, loot_types[loot_type_id].at(json_fields::ITEM_COST).as_int64()});
        }

        if (val_map.as_object().contains(json_fields::DOG_SPEED)) {
            map.SetSpeed(val_map.at(json_fields::DOG_SPEED).as_double());
        } else {
            map.SetSpeed(default_dog_speed);
        }

        if (val_map.as_object().contains(json_fields::INVENTORY_SIZE)) {
            map.SetInventorySize(val.at(json_fields::INVENTORY_SIZE).as_int64());
        } else {
            map.SetInventorySize(default_inventory_size);
        }

        game.AddMap(map);

    }

    return game;
}

}  // namespace json_loader
