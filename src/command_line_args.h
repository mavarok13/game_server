#pragma once

#include <optional>

struct Args {

    int tick_period;
    int autosave_period = -1;
    std::string config_file;
    std::string wwwroot_dir;
    std::string save_file_path;
    bool random_position;
    bool no_tick_period;

};