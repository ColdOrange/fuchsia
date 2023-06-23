//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include "fuchsia/epoll_context.h"

namespace fuchsia::net {

template <typename Protocol>
class Socket {
public:
    using ContextType = EpollContext;
    using ProtocolType = Protocol;
    using EndpointType = typename ProtocolType::Endpoint;

    // Create a socket with non-blocking mode.
    Socket(ContextType& context, ProtocolType protocol) noexcept : context_{context} {
        OpenNonBlocking(protocol);
    }

    // Create a socket and bind to the given endpoint.
    Socket(ContextType& context, const EndpointType& endpoint) noexcept : context_{context} {
        OpenNonBlocking(endpoint.Protocol());
        Bind(endpoint);
    }

    Socket(Socket&& other) noexcept : fd_{std::exchange(other.fd_, -1)}, context_{other.context_} {}

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            Close();
            fd_ = std::exchange(other.fd_, -1);
            context_ = other.context_;
        }
        return *this;
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    ~Socket() noexcept { Close(); }

    ContextType& Context() const noexcept { return context_; }

    int Fd() const noexcept { return fd_; }

    void Bind(const EndpointType& endpoint) {
        if (::bind(fd_, endpoint.Data(), endpoint.Size()) < 0) {
            throw std::system_error(errno, std::system_category(), "bind socket failed");
        }
    }

    void Listen() {
        if (::listen(fd_, SOMAXCONN) < 0) {
            throw std::system_error(errno, std::system_category(), "listen socket failed");
        }
    }

    void Shutdown(int type) {
        if (::shutdown(fd_, type) < 0) {
            throw std::system_error(errno, std::system_category(), "shutdown socket failed");
        }
    }

    void Close() noexcept {
        if (fd_ < 0) {
            throw std::runtime_error("invalid socket");
        }
        ::close(fd_);
        fd_ = -1;
    }

    constexpr auto operator<=>(const Socket&) const noexcept = default;

private:
    void OpenNonBlocking(ProtocolType protocol) {
        fd_ = ::socket(protocol.Family(), protocol.Yype() | SOCK_NONBLOCK, protocol.Protocol());
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category(), "open socket failed");
        }
    }

private:
    int fd_;
    ContextType& context_;
};

}  // namespace fuchsia::net
