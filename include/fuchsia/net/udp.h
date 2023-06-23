//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include <netinet/in.h>

#include "fuchsia/net/acceptor.h"
#include "fuchsia/net/endpoint.h"
#include "fuchsia/net/socket.h"

namespace fuchsia::net {

class Udp {
public:
    using Endpoint = fuchsia::net::Endpoint<Udp>;
    using Socket = fuchsia::net::Socket<Udp>;
    using Acceptor = fuchsia::net::Acceptor<Udp>;

    static constexpr Udp V4() noexcept { return Udp(AF_INET); }

    static constexpr Udp V6() noexcept { return Udp(AF_INET6); }

    constexpr int Type() const noexcept { return SOCK_DGRAM; }

    constexpr int Protocol() const noexcept { return IPPROTO_UDP; }

    constexpr int Family() const noexcept { return family_; }

    friend bool operator==(const Udp& lhs, const Udp& rhs) noexcept {
        return lhs.family_ == rhs.family_;
    }

    friend bool operator!=(const Udp& lhs, const Udp& rhs) noexcept {
        return lhs.family_ != rhs.family_;
    }

private:
    explicit constexpr Udp(int protocol_family) noexcept : family_(protocol_family) {}

    int family_;
};

}  // namespace fuchsia::net
