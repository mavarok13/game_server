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

class SessionBase {
public:
    SessionBase(const SessionBase &) = delete;
    SessionBase& operator=(const SessionBase &) = delete;

    void Run();

protected:
    using HttpRequest = http::request<http::string_body>;

    explicit SessionBase(tcp::socket && socket) : stream_(std::move(socket)) {}
    ~SessionBase() = default;
    
    virtual void HandleRequest (HttpRequest && request) = 0;

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields> && response, std::chrono::time_point<std::chrono::system_clock> start_time_response) {
        auto response_ptr = std::make_shared<http::response<Body, Fields>>(std::move(response));
        auto self = GetSharedThis();

        http::async_write(stream_, *response_ptr, [response_ptr, self, start_time_response] (beast::error_code ec, std::size_t bytes) {
            self->OnWrite(response_ptr->need_eof(), response_ptr, start_time_response, ec, bytes);
        });
    }

private:
    void Read();

    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        if (ec == http::error::end_of_stream) {
            Close();
        }

        if (ec) {
            BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", ec.value() }, {"text", ec.message()}, {"where", "read"}}) << logging::add_value(message_, "error");
            return;
        }

        BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "ip", stream_.socket().remote_endpoint().address().to_string() }, {"URI", request_.target()}, {"method", request_.method_string()}}) << logging::add_value(message_, "request received");

        HandleRequest(std::move(request_));
    }

    void OnWrite(bool close, auto response_ptr, std::chrono::time_point<std::chrono::system_clock> start_time_response, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        if (ec) {
            BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", ec.value() }, {"text", ec.message()}, {"where", "write"}}) << logging::add_value(message_, "error");
            return;
        }

        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_response);

        BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", response_ptr->result_int() }, {"response_time", response_time.count()}, {"content_type", (*response_ptr)[http::field::content_type]}}) << logging::add_value(message_, "response sent");

        if (close) {
            return Close();
        }

        Read();
    }

    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

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
    void HandleRequest(HttpRequest && request) override {
        handler_(std::move(request), [self=this->shared_from_this()] (auto && response, std::chrono::time_point<std::chrono::system_clock> start_time_response) {
            self->Write(std::move(response), start_time_response);
        });
    }
private:
    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }

    RequestHandler handler_;

};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener (net::io_context & io, const tcp::endpoint & endpoint, Handler && handler) : 
    io_(io),
    endpoint_(endpoint),
    acceptor_(net::make_strand(io)),
    handler_(std::forward<Handler>(handler)) {
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
    }

    void DoAccept() {
        acceptor_.async_accept(net::make_strand(io_), beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept (beast::error_code ec, tcp::socket socket) {
        if (ec) {
            BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", ec.value() }, {"text", ec.message()}, {"where", "accept"}}) << logging::add_value(message_, "error");
            return;
        }

        AsyncRunSession(std::move(socket));

        DoAccept();
    }

    void AsyncRunSession(tcp::socket && socket) {
        std::make_shared<Session<RequestHandler>>(std::move(socket), std::move(handler_))->Run();
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

    std::make_shared<ThisListener>(io, endpoint, std::forward<RequestHandler>(handler))->DoAccept();
}

}  // namespace http_server
