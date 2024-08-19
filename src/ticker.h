#pragma once

#include <chrono>
#include <memory>

#include <boost/asio.hpp>

namespace ticker {

namespace net = boost::asio;
namespace sys = boost::system;

using Strand = net::strand<net::io_context::executor_type>;
using Handler = std::function<void(int)>;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    Ticker (std::shared_ptr<Strand> strand, std::chrono::milliseconds interval, Handler && handler) : strand_(strand), timer_(net::steady_timer{*strand_}), interval_(interval), handler_(std::move(handler)) {}

    void Start () {
        net::dispatch(*strand_, [self = shared_from_this()] {
            self->Release();
        });
    }
private:
    void Release() {
        timer_.expires_after(interval_);
        timer_.async_wait([self = shared_from_this()] (sys::error_code ec) {
            if (!ec) {

                self->handler_(self->interval_.count());
                self->Release();
            }
        });
    }

    std::shared_ptr<Strand> strand_;
    net::steady_timer timer_;
    std::chrono::milliseconds interval_;
    Handler handler_;
};

} // namespace ticker