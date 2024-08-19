#include "http_server.h"

namespace http_server {

void SessionBase::Run() {
    net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
        using namespace std::literals;

        request_ = {};
        stream_.expires_after(15s);

        http::async_read(stream_, buf_, request_, beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

}  // namespace http_server
