//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include "fuchsia/net/socket.h"

namespace fuchsia::net {

template <typename Protocol>
class Acceptor : public Socket<Protocol> {
public:
    using EndpointType = typename Protocol::Endpoint;
    using ContextType = typename Socket<Protocol>::ContextType;

    Acceptor(ContextType& context, const EndpointType& endpoint) noexcept
        : Socket<Protocol>{context, endpoint} {
        Socket<Protocol>::Listen();
    }

    Acceptor(Acceptor&& other) noexcept = default;
    Acceptor& operator=(Acceptor&& other) noexcept = default;
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    ~Acceptor() noexcept = default;
};

}  // namespace fuchsia::net
