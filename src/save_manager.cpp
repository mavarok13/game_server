#include "save_manager.h"

#include <fstream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

void save_manager::SaveToFile(std::string_view path, std::string_view content) {
    std::string str(path.begin(), path.end());
    fs::path path_fs(str);
    str.append("_temp");
    fs::path temp_path_fs(str);
    path_fs = fs::weakly_canonical(path_fs);

    std::ofstream ofs(path_fs, std::ios::out | std::ios::trunc);

    ofs << content;
    ofs.close();

    if (fs::is_regular_file(temp_path_fs)) {
        fs::rename(temp_path_fs, path_fs);
    }
}

std::string save_manager::LoadSavedFile(std::string_view path) {
    fs::path path_fs(path.begin(), path.end());
    path_fs = fs::weakly_canonical(path_fs);
    
    std::string data;

    std::ifstream ifs(path_fs, std::ios::in);

    if (ifs.is_open()) {
        char c = '\0';
        while (ifs.get(c)) {
            data.push_back(c);
        }
        
    } else {
        throw std::invalid_argument("Can't load file");
    }

    return data;
}