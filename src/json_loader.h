#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "model.h"
#include "app.h"
#include "json_fields.h"
#include "extra_data.h"

namespace json_loader {

namespace json = boost::json; 

void ExtractRoadsToMap(const json::value & val_map, model::Map * map);
void ExtractBuildingsToMap(const json::value & val_map, model::Map * map);
void ExtractOfficesToMap(const json::value & val_map, model::Map * map);

std::vector<extra_data::MapExtraData> GetMapsExtraData(const std::filesystem::path& json_path);

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
