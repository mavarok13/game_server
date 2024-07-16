#include "request_handler.h"

#include <string>

namespace http_handler {

HttpResponse ConstructJsonResponse (http::status status, unsigned version, bool keep_alive) {

    HttpResponse response(status, version);
    response.set(http::field::content_type, "application/json");
    response.keep_alive(keep_alive);

    return response;
}

HttpFileResponse ConstructFileResponse (HttpRequest req, const fs::path & root, sys::error_code & ec) {

    std::string target_str(req.target().data(), req.target().size());

    fs::path root_path;
    root_path += root;

    if (target_str == "/" || target_str == "") {

        target_str = "/index.html";
    }
    
    root_path += target_str.c_str();

    root_path = fs::weakly_canonical(root_path);

    http::file_body::value_type file;

    HttpFileResponse response(http::status::ok, req.version());
    response.keep_alive(req.keep_alive());

    file.open(root_path.c_str(), beast::file_mode::read, ec);

    response.set(http::field::content_type, http_content_type::GetContentTypeByExtension({root_path.extension().c_str()}));
    response.body() = std::move(file);
    response.prepare_payload();

    return response;
}

HttpResponse ConstructMethodNotAllowedResponse (HttpRequest request, std::string_view methods) {
    HttpResponse response{ConstructJsonResponse(http::status::method_not_allowed, request.version())};
            
    response.set(http::field::allow, "GET, HEAD");
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_builder::GetMethodNotAllowed_s("GET, HEAD");
    response.prepare_payload();
}
}  // namespace http_handler
