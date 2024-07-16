#pragma once

#include <boost/beast.hpp>

#include "http_path_utils.h"

namespace endpoint {

namespace fs = std::filesystem;
namespace beast = boost::beast;
namespace http = beast::http;

using HttpRequest = http::request<http::string_body>;

class Endpoint {
public:
    using Handler = std::function<void(const HttpRequest &)>;

    Endpoint (const fs::path & path, http::verb method, Handler && handler, int args_count = 0) : path_{path}, method_(method), handler_{handler}, args_count_(args_count), is_root_(false) {}
    Endpoint (const fs::path & path, http::verb method, Handler && handler, bool is_root) : path_{path}, method_(method), handler_{handler}, args_count_(0), is_root_(is_root) {}

    bool IsMatch(const fs::path & path, http::verb method) const {

        return method_ == method && ( (args_count_ == 0 && http_path_utils::MatchPaths(path, path_)) || (args_count_ > 0 && http_path_utils::PathBased(path, path_) == args_count_) || (is_root_ && http_path_utils::PathBased(path, path_) >= 0) );
    }

    void Invoke(const HttpRequest & request) {
        handler_(request);
    }

    void Invoke(const HttpRequest & request) const {
        handler_(request);
    }

    fs::path GetPath() const {

        return path_;
    }

private:
    fs::path path_;
    http::verb method_;
    Handler handler_;
    int args_count_ = 0;
    bool is_root_ = false;
};
} //namespace endpoint