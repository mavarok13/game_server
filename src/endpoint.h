#pragma once

#include <boost/beast.hpp>

#include "http_utils.h"

namespace endpoint {

namespace fs = std::filesystem;
namespace beast = boost::beast;
namespace http = beast::http;

template <typename Body, typename Allocator>
class Endpoint {
public:
    using Handler = std::function<void(http::request<Body, Allocator>&&)>;

    Endpoint (const fs::path & path, http::verb method, Handler handler, int args_count = 0) : path_{path}, method_(method), handler_{handler}, args_count_(args_count), is_root_(false) {}
    Endpoint (const fs::path & path, http::verb method, Handler handler, bool is_root) : path_{path}, method_(method), handler_{handler}, args_count_(0), is_root_(is_root) {}

    bool IsMatch(const fs::path & path, http::verb method) const {

        return method_ == method && ( (args_count_ == 0 && http_utils::MatchPaths(path, path_)) || (args_count_ > 0 && http_utils::PathBased(path, path_) == args_count_) || (is_root_ && http_utils::PathBased(path, path_) >= 0) );
    }

    void Invoke(http::request<Body, Allocator> && request) {
        handler_(std::move(request));
    }

    void Invoke(http::request<Body, Allocator> && request) const {
        handler_(std::move(request));
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