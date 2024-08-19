#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <string_view>

namespace save_manager {

class Saver {
public:
    using Milliseconds = std::chrono::milliseconds;
    using Handler = std::function<void()>;

    Saver(Milliseconds autosave_period, Handler handler) : autosave_period_(autosave_period), handler_(handler) {}

    void Save(Milliseconds delta_time = std::chrono::milliseconds(0)) {
        last_save_interval_ += delta_time;

        if (last_save_interval_ >= autosave_period_) {
            handler_();
        }
    }
private:
    Milliseconds autosave_period_;
    Milliseconds last_save_interval_ = std::chrono::milliseconds(0);
    Handler handler_;
};

void SaveToFile(std::string_view path, std::string_view content);
std::string LoadSavedFile(std::string_view path);

} //namespace save_manager