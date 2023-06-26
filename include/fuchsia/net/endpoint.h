//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include <cstdint>

#include "fuchsia/net/address.h"

namespace fuchsia::net {

using PortType = uint_least16_t;

template <typename InternetProtocol>
class Endpoint {
public:
    using ProtocolType = InternetProtocol;

    Endpoint() noexcept {
        data_.v4.sin_family = AF_INET;
        data_.v4.sin_port = 0;
        data_.v4.sin_addr.s_addr = INADDR_ANY;
    }

    Endpoint(ProtocolType protocol, PortType port) noexcept {
        if (protocol.Family() == AF_INET) {
            data_.v4.sin_family = AF_INET;
            data_.v4.sin_port = ::htons(port);
            data_.v4.sin_addr.s_addr = INADDR_ANY;
        } else {
            data_.v6.sin6_family = AF_INET6;
            data_.v6.sin6_port = ::htons(port);
            data_.v6.sin6_addr = IN6ADDR_ANY_INIT;
        }
    }

    Endpoint(const Address& addr, PortType port) noexcept {
        if (addr.IsV4()) {
            data_.v4.sin_family = AF_INET;
            data_.v4.sin_port = ::htons(port);
            data_.v4.sin_addr = std::bit_cast<in_addr>(addr.ToV4().ToBytes());
        } else if (addr.IsV6()) {
            data_.v6.sin6_family = AF_INET6;
            data_.v6.sin6_port = ::htons(port);
            data_.v6.sin6_addr = std::bit_cast<in6_addr>(addr.ToV6().ToBytes());
        }
    }

    explicit Endpoint(::sockaddr* addr) noexcept {
        if (addr->sa_family == AF_INET) {
            data_.v4 = *std::bit_cast<::sockaddr_in*>(addr);
        } else if (addr->sa_family == AF_INET6) {
            data_.v6 = *std::bit_cast<::sockaddr_in6*>(addr);
        }
    }

    Endpoint(const Endpoint& other) noexcept : data_(other.data_) {}

    Endpoint(Endpoint&& other) noexcept : data_(other.data_) {}

    Endpoint& operator=(const Endpoint& other) noexcept {
        data_ = other.data_;
        return *this;
    }

    Endpoint& operator=(Endpoint&& other) noexcept {
        data_ = other.data_;
        return *this;
    }

    bool IsV4() const noexcept { return data_.base.sa_family == AF_INET; }

    ProtocolType Protocol() const noexcept {
        if (IsV4()) {
            return ProtocolType::V4();
        } else {
            return ProtocolType::V6();
        }
    }

    fuchsia::net::Address Address() const noexcept {
        if (IsV4()) {
            return AddressV4{std::bit_cast<AddressV4::BytesType>(data_.v4.sin_addr)};
        } else {
            return AddressV6{std::bit_cast<AddressV6::BytesType>(data_.v6.sin6_addr)};
        }
    }

    PortType Port() const noexcept {
        if (IsV4()) {
            return ::ntohs(data_.v4.sin_port);
        } else {
            return ::ntohs(data_.v6.sin6_port);
        }
    }

    const ::sockaddr* Data() const noexcept { return &data_.base; }

    std::size_t Size() const noexcept {
        if (IsV4()) {
            return sizeof(data_.v4);
        } else {
            return sizeof(data_.v6);
        }
    }

    std::string ToString() const noexcept {
        if (IsV4()) {
            return Address().ToV4().ToString() + ":" + std::to_string(Port());
        } else {
            return "[" + Address().ToV6().ToString() + "]:" + std::to_string(Port());
        }
    }

    friend bool operator==(Endpoint lhs, Endpoint rhs) noexcept {
        return lhs.Address() == rhs.Address() && lhs.Port() == rhs.Port();
    }

    friend bool operator!=(Endpoint lhs, Endpoint rhs) noexcept { return !(lhs == rhs); }

    friend bool operator<(Endpoint lhs, Endpoint rhs) noexcept {
        return lhs.Address() < rhs.Address() ||
               (lhs.Address() == rhs.Address() && lhs.Port() < rhs.Port());
    }

    friend bool operator>(Endpoint lhs, Endpoint rhs) noexcept { return rhs < lhs; }

    friend bool operator<=(Endpoint lhs, Endpoint rhs) noexcept { return !(lhs > rhs); }

    friend bool operator>=(Endpoint lhs, Endpoint rhs) noexcept { return !(lhs < rhs); }

private:
    union {
        ::sockaddr base;
        ::sockaddr_in v4;
        ::sockaddr_in6 v6;
    } data_{};
};

}  // namespace fuchsia::net

namespace std {

template <typename Protocol>
struct hash<fuchsia::net::Endpoint<Protocol>> {
    std::size_t operator()(const fuchsia::net::Endpoint<Protocol>& ep) const noexcept {
        std::size_t hash1 = std::hash<fuchsia::net::Address>()(ep.Address());
        std::size_t hash2 = std::hash<fuchsia::net::PortType>()(ep.Port());
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
};

}  // namespace std
