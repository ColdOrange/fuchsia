//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include <netinet/in.h>

#include "fuchsia/net/acceptor.h"
#include "fuchsia/net/endpoint.h"
#include "fuchsia/net/socket.h"

namespace fuchsia::net {

class Tcp {
public:
    using Endpoint = fuchsia::net::Endpoint<Tcp>;
    using Socket = fuchsia::net::Socket<Tcp>;
    using Acceptor = fuchsia::net::Acceptor<Tcp>;

    static constexpr Tcp V4() noexcept { return Tcp(AF_INET); }

    static constexpr Tcp V6() noexcept { return Tcp(AF_INET6); }

    constexpr int Type() const noexcept { return SOCK_STREAM; }

    constexpr int Protocol() const noexcept { return IPPROTO_TCP; }

    constexpr int Family() const noexcept { return family_; }

    friend bool operator==(const Tcp& lhs, const Tcp& rhs) noexcept {
        return lhs.family_ == rhs.family_;
    }

    friend bool operator!=(const Tcp& lhs, const Tcp& rhs) noexcept {
        return lhs.family_ != rhs.family_;
    }

private:
    explicit constexpr Tcp(int protocol_family) noexcept : family_(protocol_family) {}

    int family_;
};

}  // namespace fuchsia::net
