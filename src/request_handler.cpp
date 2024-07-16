#include "request_handler.h"

#include <string>

namespace http_handler {

HttpResponse ConstructJsonResponse (http::status status, unsigned version, bool keep_alive) {
    HttpResponse response(status, version);
    response.set(http::field::content_type, "application/json");
    response.keep_alive(keep_alive);

    return response;
}

HttpFileResponse ConstructFileResponse (HttpRequest request, const fs::path & root, sys::error_code & ec) {
    std::string target_str(request.target().data(), request.target().size());

    fs::path root_path;
    root_path += root;

    if (target_str == "/" || target_str == "") {

        target_str = "/index.html";
    }
    
    root_path += target_str.c_str();

    root_path = fs::weakly_canonical(root_path);

    http::file_body::value_type file;

    HttpFileResponse response(http::status::ok, request.version());
    response.keep_alive(request.keep_alive());

    file.open(root_path.c_str(), beast::file_mode::read, ec);

    response.set(http::field::content_type, http_content_type::GetContentTypeByExtension({root_path.extension().c_str()}));
    response.body() = std::move(file);
    response.prepare_payload();

    return response;
}

// RESPONSE: OK
HttpResponse ConstructOkResponse(HttpRequest request) {
    HttpResponse response = ConstructJsonResponse(http::status::ok, request.version());
    response.set(http::field::cache_control, "no-cache");

    return response;
}
HttpResponse ConstructOkResponse(HttpRequest request, std::string_view body) {
    HttpResponse response = ConstructOkResponse(request);
    response.body() = body;
    response.prepare_payload();

    return response;
}

// RESPONSE: METHOD NOT ALLOWED
HttpResponse ConstructMethodNotAllowedResponse (HttpRequest request, std::string_view methods) {
    HttpResponse response = ConstructJsonResponse(http::status::method_not_allowed, request.version());
    response.set(http::field::allow, "GET, HEAD");
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_builder::GetMethodNotAllowed_s("GET, HEAD");
    response.prepare_payload();

    return response;
}

// RESPONSE: INVALID ARGUMENT
HttpResponse ConstructInvalidArgumentResponse(HttpRequest request, std::string_view msg) {
    HttpResponse response = ConstructJsonResponse(http::status::bad_request, request.version());
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_builder::GetInvalidArgument_s(msg);
    response.prepare_payload();

    return response;
}

// RESPONSE: MAP NOT FOUNDs
HttpResponse ConstructMapNotFoundResponse(HttpRequest request) {
    HttpResponse response = ConstructJsonResponse(http::status::not_found, request.version());
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_builder::GetMapNotFound_s();
    response.prepare_payload();

    return response;
}
HttpResponse ConstructMapNotFoundResponse_head(HttpRequest request) {
    HttpResponse response = ConstructJsonResponse(http::status::not_found, request.version());
    response.set(http::field::cache_control, "no-cache");
    response.prepare_payload();

    return response;
}

// RESPONSE: BAD REQUEST
HttpResponse ConstructBadRequestResponse(HttpRequest request, std::string_view body) {
    HttpResponse response = ConstructJsonResponse(http::status::bad_request, request.version());
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.prepare_payload();

    return response;
}

HttpResponse ConstructBadRequestResponse(HttpRequest request) {
    HttpResponse response = ConstructJsonResponse(http::status::bad_request, request.version());
    response.set(http::field::cache_control, "no-cache");
    response.prepare_payload();

    return response;
}

// RESPONSE: UNAUTHORIZED
HttpResponse ConstructUnauthorizedResponse(HttpRequest request, std::string_view body) {
    HttpResponse response = ConstructJsonResponse(http::status::unauthorized, request.version());
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.prepare_payload();

    return response;
}
HttpResponse ConstructUnauthorizedResponse(HttpRequest request) {
    HttpResponse response = ConstructJsonResponse(http::status::unauthorized, request.version());
    response.set(http::field::cache_control, "no-cache");
    response.prepare_payload();

    return response;
}

}  // namespace http_handler
