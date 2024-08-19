#include "request_handler.h"

#include <string>

namespace http_handler {

HttpResponse ConstructJsonResponse (http::status status, unsigned version, bool keep_alive) {
    HttpResponse response(status, version);
    response.set(http::field::content_type, "application/json");
    response.keep_alive(keep_alive);

    return response;
}

HttpFileResponse ConstructFileResponse (fs::path root, fs::path request_target, unsigned version, bool keep_alive, sys::error_code & ec) {
    if (request_target.filename().empty()) {
        request_target.replace_filename("index.html");
    }
    
    root += request_target;

    http::file_body::value_type file;

    HttpFileResponse response(http::status::ok, version);
    response.keep_alive(keep_alive);

    file.open(root.c_str(), beast::file_mode::read, ec);

    response.set(http::field::content_type, http_content_type::GetContentTypeByExtension({root.extension().c_str()}));
    response.body() = std::move(file);
    response.prepare_payload();

    return response;
}

// RESPONSE: OK
HttpResponse ConstructOkResponse(unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::ok, version);
    response.set(http::field::cache_control, "no-cache");

    return response;
}
HttpResponse ConstructOkResponse(std::string_view body, unsigned version, bool keep_alive) {
    HttpResponse response = ConstructOkResponse(version, keep_alive);
    response.body() = body;
    response.prepare_payload();

    return response;
}

// RESPONSE: METHOD NOT ALLOWED
HttpResponse ConstructMethodNotAllowedResponse (std::string_view methods, unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::method_not_allowed, version);
    response.set(http::field::allow, methods);
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_builder::GetMethodNotAllowed_s(methods);
    response.prepare_payload();

    return response;
}

// RESPONSE: INVALID ARGUMENT
HttpResponse ConstructInvalidArgumentResponse(std::string_view msg, unsigned version, bool keep_alive) {
    HttpResponse response{ConstructJsonResponse(http::status::bad_request, version)};
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_builder::GetInvalidArgument_s(msg);
    response.prepare_payload();

    return response;
}

// RESPONSE: MAP NOT FOUNDs
HttpResponse ConstructMapNotFoundResponse(unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::not_found, version);
    response.set(http::field::cache_control, "no-cache");
    response.body() = json_builder::GetMapNotFound_s();
    response.prepare_payload();

    return response;
}
HttpResponse ConstructMapNotFoundResponse_head(unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::not_found, version);
    response.set(http::field::cache_control, "no-cache");
    response.prepare_payload();

    return response;
}

// RESPONSE: BAD REQUEST
HttpResponse ConstructBadRequestResponse(std::string_view body, unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::bad_request, version);
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.prepare_payload();

    return response;
}

HttpResponse ConstructBadRequestResponse(unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::bad_request, version);
    response.set(http::field::cache_control, "no-cache");
    response.prepare_payload();

    return response;
}

// RESPONSE: UNAUTHORIZED
HttpResponse ConstructUnauthorizedResponse(std::string_view body, unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::unauthorized, version);
    response.set(http::field::cache_control, "no-cache");
    response.body() = body;
    response.prepare_payload();

    return response;
}
HttpResponse ConstructUnauthorizedResponse(unsigned version, bool keep_alive) {
    HttpResponse response = ConstructJsonResponse(http::status::unauthorized, version);
    response.set(http::field::cache_control, "no-cache");
    response.prepare_payload();

    return response;
}

}  // namespace http_handler
