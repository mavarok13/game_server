#pragma once

#include <filesystem>

namespace http_path_utils {

namespace fs = std::filesystem;

int PathBased(fs::path target_path, fs::path base);

bool MatchPaths(fs::path target_path, fs::path path);

std::string UrlUncode(std::string_view encoded_target);
} // namespace http_path_utils