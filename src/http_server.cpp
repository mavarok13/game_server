#include "http_server.h"

namespace http_server {

void SessionBase::Run() {
    auto self = GetSharedPtr();
    net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, self));
}

void SessionBase::Read() {
        using namespace std::literals;

        stream_.expires_after(15s);

        request_ = {};

        http::async_read(stream_, buf_, request_, [self = GetSharedPtr(), this] (beast::error_code ec, size_t bytes) {

            if (ec == http::error::end_of_stream) {
                Close();
            }

            if (ec) {
                // std::cerr << ec.what() << std::endl;
                BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "code", ec.value() }, {"text", ec.message()}, {"where", "read"}}) << logging::add_value(message_, "error");
                return;
            }

            BOOST_LOG_TRIVIAL(info) << logging::add_value(data_, {{ "ip", stream_.socket().remote_endpoint().address().to_string() }, {"URI", request_.target()}, {"method", request_.method_string()}}) << logging::add_value(message_, "request received");

            HandleRequest(std::move(request_));
        });
    }

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

}  // namespace http_server
