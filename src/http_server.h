#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once
#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <iostream>
#include <chrono>

#include "logger.h"

namespace http_server {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = net::ip::tcp;
using namespace std::literals;
// Разместите здесь реализацию http-сервера, взяв её из задания по разработке асинхронного сервера

class SessionBase {
public:
    explicit SessionBase(tcp::socket && socket) : stream_(std::move(socket)) {}

    void Run();

protected:
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = http::response<http::string_body>;

    virtual std::shared_ptr<SessionBase> GetSharedPtr() = 0;
    
    virtual void HandleRequest (HttpRequest request) = 0;

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields> && response, std::chrono::time_point<std::chrono::system_clock> start_time_response) {

        auto response_ptr = std::make_shared<http::response<Body, Fields>>(std::move(response));
        http::async_write(stream_, *response_ptr, [response_ptr, self = GetSharedPtr(), start_time_response, this] (beast::error_code ec, std::size_t bytes) {

            if (ec) {
                // std::cerr << ec.what() << std::endl;
                BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", ec.value() }, {"text", ec.message()}, {"where", "write"}}) << logging::add_value(message_, "error");
                return;
            }

            std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
            auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_response);

            BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", response_ptr->result_int() }, {"response_time", response_time.count()}, {"content_type", (*response_ptr)[http::field::content_type]}}) << logging::add_value(message_, "response sent");

            if (response_ptr->need_eof()) {
                return Close();
            }

            Read();
        });
    }

private:
    void Read();

    void Close();

    beast::tcp_stream stream_;
    beast::flat_buffer buf_;
    HttpRequest request_;

};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session (tcp::socket && socket, Handler && handler) : SessionBase(std::move(socket)), handler_(std::forward<Handler>(handler)) {}

protected:
    void HandleRequest(HttpRequest request) override {

        handler_(std::move(request), [self=this->shared_from_this()] (auto && response, std::chrono::time_point<std::chrono::system_clock> start_time_response) {

            self->Write(std::move(response), start_time_response);
        });
    }

    std::shared_ptr<SessionBase> GetSharedPtr() override {
        return this->shared_from_this();
    }

private:
    RequestHandler handler_;

};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    Listener (net::io_context & io, const tcp::endpoint & endpoint, RequestHandler && handler) : 
    io_(io),
    endpoint_(endpoint),
    acceptor_(net::make_strand(io)),
    handler_(handler) {

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
    }

    void Accept() {
        acceptor_.async_accept(net::make_strand(io_), [self = this->shared_from_this()] (beast::error_code ec, tcp::socket socket) {
            if (ec) {
                // std::cerr << ec.what() << std::endl;
                BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", ec.value() }, {"text", ec.message()}, {"where", "accept"}}) << logging::add_value(message_, "error");
                return;
            }

            std::make_shared<Session<RequestHandler>>(std::move(socket), std::move(self->handler_))->Run();
            self->shared_from_this()->Accept();
        });
    }

private:
    net::io_context & io_;
    tcp::endpoint endpoint_;
    tcp::acceptor acceptor_;
    RequestHandler handler_;

};

template <typename RequestHandler>
void ServeHttp(net::io_context & io, const tcp::endpoint & endpoint, RequestHandler && handler) {
    using ThisListener = Listener<RequestHandler>;

    std::make_shared<ThisListener>(io, endpoint, std::forward<RequestHandler>(handler))->Accept();
}

}  // namespace http_server
