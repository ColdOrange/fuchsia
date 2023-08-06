//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include "fuchsia/epoll_context.h"

namespace fuchsia::net {

enum class ShutdownMode { Read = SHUT_RD, Write = SHUT_WR, Both = SHUT_RDWR };

template <typename Protocol>
class Socket {
public:
    using ContextType = EpollContext;
    using ProtocolType = Protocol;
    using EndpointType = typename ProtocolType::Endpoint;

    // Default constructor.
    Socket(ContextType& context) noexcept : context_{&context} {}

    // Create a socket with native socket fd.
    Socket(ContextType& context, int fd) noexcept : fd_{fd}, context_{&context} {}

    // Create a socket and open with non-blocking mode.
    Socket(ContextType& context, ProtocolType protocol) noexcept : context_{&context} {
        OpenNonBlocking(protocol);
    }

    // Create a socket and bind to the given endpoint.
    Socket(ContextType& context, const EndpointType& endpoint) noexcept : context_{&context} {
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

    ContextType& Context() const noexcept { return *context_; }

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

    void SetReuseAddr() {
        int optval = 1;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            throw std::system_error(errno, std::system_category(),
                                    "setsockopt SO_REUSEADDR failed");
        }
    }

    std::optional<std::pair<Socket, EndpointType>> Accept(std::error_code& ec) {
        ::sockaddr_storage addr;
        ::socklen_t len = sizeof(addr);

        while (true) {
            int conn_fd = ::accept4(fd_, reinterpret_cast<::sockaddr*>(&addr), &len, SOCK_NONBLOCK);
            if (conn_fd >= 0) {
                return std::make_pair(Socket{*context_, conn_fd},
                                      EndpointType{reinterpret_cast<::sockaddr*>(&addr)});
            }

            if (errno == EINTR) {
                continue;
            }

            ec = std::error_code(errno, std::system_category());
            return std::nullopt;
        }
    }

    std::optional<size_t> Send(const void* data, size_t size, std::error_code& ec) {
        while (true) {
            ssize_t n = ::send(fd_, data, size, 0);
            if (n >= 0) {
                return static_cast<size_t>(n);
            }

            if (errno == EINTR) {
                continue;
            }

            ec = std::error_code(errno, std::system_category());
            return std::nullopt;
        }
    }

    std::optional<size_t> Recv(void* data, size_t size, std::error_code& ec) {
        while (true) {
            ssize_t n = ::recv(fd_, data, size, 0);
            if (n > 0) {
                return static_cast<size_t>(n);
            } else if (n == 0) {  // connection closed by peer
                ec = std::make_error_code(std::errc::connection_aborted);  // FIXME: error code
                return std::nullopt;
            }

            if (errno == EINTR) {
                continue;
            }

            ec = std::error_code(errno, std::system_category());
            return std::nullopt;
        }
    }

    std::optional<size_t> SendMsg(iovec* bufs, uint64_t count, std::error_code& ec) {
        msghdr msg{.msg_iov = bufs, .msg_iovlen = count};
        while (true) {
            ssize_t n = ::sendmsg(fd_, &msg, 0);
            if (n >= 0) {
                return static_cast<size_t>(n);
            }

            if (errno == EINTR) {
                continue;
            }

            ec = std::error_code(errno, std::system_category());
            return std::nullopt;
        }
    }

    std::optional<size_t> RecvMsg(iovec* bufs, uint64_t count, std::error_code& ec) {
        msghdr msg{.msg_iov = bufs, .msg_iovlen = count};
        while (true) {
            ssize_t n = ::recvmsg(fd_, &msg, 0);
            if (n > 0) {
                return static_cast<size_t>(n);
            } else if (n == 0) {  // connection closed by peer
                ec = std::make_error_code(std::errc::connection_aborted);  // FIXME: error code
                return std::nullopt;
            }

            if (errno == EINTR) {
                continue;
            }

            ec = std::error_code(errno, std::system_category());
            return std::nullopt;
        }
    }

    void Shutdown(ShutdownMode type) {
        if (::shutdown(fd_, static_cast<int>(type)) < 0) {
            throw std::system_error(errno, std::system_category(), "shutdown socket failed");
        }
    }

    void Close() {
        ::close(fd_);
        fd_ = -1;
    }

    constexpr auto operator<=>(const Socket&) const noexcept = default;

private:
    void OpenNonBlocking(ProtocolType protocol) {
        fd_ = ::socket(protocol.Family(), protocol.Type() | SOCK_NONBLOCK, protocol.Protocol());
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category(), "open socket failed");
        }
    }

private:
    int fd_;
    ContextType* context_;
};

}  // namespace fuchsia::net
