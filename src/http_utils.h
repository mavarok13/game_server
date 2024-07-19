#pragma once

#include <filesystem>

namespace http_utils {

namespace fs = std::filesystem;

int PathBased(fs::path target_path, fs::path base);
bool MatchPaths(fs::path target_path, fs::path path);

std::string UrlDecode(std::string_view encoded_str);

std::string FormatToken(std::string_view token);

} // namespace http_utils