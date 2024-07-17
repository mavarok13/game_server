#pragma once

#include <boost/json.hpp>

#include "model.h"

namespace json = boost::json;

namespace extra_data {
class MapExtraData {
public:
    MapExtraData(std::string map_id, json::value map_loot_types) : map_id_(map_id), map_loot_types_(map_loot_types) {}

    std::string GetMapId() noexcept {
        return map_id_;
    }

    std::string GetMapId() noexcept const {
        return map_id_;
    }

    json::value GetMapLootTypes() {
        return map_loot_types_;
    }

    json::value GetMapLootTypes() const {
        return map_loot_types_;
    }
private:
    std::string map_id_;
    json::value map_loot_types_;
    
};
} //namespace extra_data