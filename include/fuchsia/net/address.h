//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <variant>

namespace fuchsia::net {

class AddressV4 {
public:
    using BytesType = std::array<std::byte, 4>;
    using UintType = uint_least32_t;

    AddressV4() noexcept : addr_{0} {}

    explicit AddressV4(UintType addr) noexcept : addr_{::htonl(addr)} {}

    explicit AddressV4(BytesType bytes) noexcept : addr_{std::bit_cast<in_addr_t>(bytes)} {}

    UintType ToUint() const noexcept { return ::ntohl(addr_.s_addr); }

    BytesType ToBytes() const noexcept { return std::bit_cast<BytesType>(addr_.s_addr); }

    std::string ToString() const {
        char buf[INET_ADDRSTRLEN + 1] = {};
        ::inet_ntop(AF_INET, &addr_, buf, INET_ADDRSTRLEN);
        return buf;
    }

    static AddressV4 Loopback() noexcept { return AddressV4{INADDR_LOOPBACK}; }

    static AddressV4 Any() noexcept { return AddressV4{INADDR_ANY}; }

    bool IsLoopback() const noexcept { return (ToUint() & 0xFF000000) == 0x7F000000; }

    bool IsUnspecified() const noexcept { return ToUint() == 0; }

    friend bool operator==(AddressV4 lhs, AddressV4 rhs) noexcept {
        return lhs.addr_.s_addr == rhs.addr_.s_addr;
    }

    friend bool operator!=(AddressV4 lhs, AddressV4 rhs) noexcept {
        return lhs.addr_.s_addr != rhs.addr_.s_addr;
    }

    friend bool operator<(AddressV4 lhs, AddressV4 rhs) noexcept {
        return lhs.ToUint() < rhs.ToUint();
    }

    friend bool operator>(AddressV4 lhs, AddressV4 rhs) noexcept {
        return lhs.ToUint() > rhs.ToUint();
    }

    friend bool operator<=(AddressV4 lhs, AddressV4 rhs) noexcept {
        return lhs.ToUint() <= rhs.ToUint();
    }

    friend bool operator>=(AddressV4 lhs, AddressV4 rhs) noexcept {
        return lhs.ToUint() >= rhs.ToUint();
    }

private:
    in_addr addr_;
};

class AddressV6 {
public:
    using BytesType = std::array<std::byte, 16>;
    using ScopeIdType = uint_least32_t;

    AddressV6() noexcept : scope_id_{0} { memset(&addr_, 0, sizeof(addr_)); }

    explicit AddressV6(BytesType bytes, ScopeIdType scope_id = 0) noexcept
        : addr_{std::bit_cast<in6_addr>(bytes)}, scope_id_{scope_id} {}

    ScopeIdType ScopeId() const noexcept { return scope_id_; }

    BytesType ToBytes() const noexcept { return std::bit_cast<BytesType>(addr_); }

    std::string ToString() const {
        char buf[INET6_ADDRSTRLEN + 1] = {};
        ::inet_ntop(AF_INET6, &addr_, buf, INET6_ADDRSTRLEN);
        return buf;
    }

    AddressV4 ToV4() const noexcept {
        if (!IsV4Mapped()) {
            return {};
        }
        auto bytes = ToBytes();
        return AddressV4{AddressV4::BytesType{bytes[12], bytes[13], bytes[14], bytes[15]}};
    }

    static AddressV6 Loopback() noexcept {
        AddressV6 addr;
        addr.addr_.s6_addr[15] = 1;
        return addr;
    }

    static AddressV6 Any() noexcept { return {}; }

    bool IsLoopback() const noexcept {
        return addr_.s6_addr[0] == 0 && addr_.s6_addr[1] == 0 && addr_.s6_addr[2] == 0 &&
               addr_.s6_addr[3] == 0 && addr_.s6_addr[4] == 0 && addr_.s6_addr[5] == 0 &&
               addr_.s6_addr[6] == 0 && addr_.s6_addr[7] == 0 && addr_.s6_addr[8] == 0 &&
               addr_.s6_addr[9] == 0 && addr_.s6_addr[10] == 0 && addr_.s6_addr[11] == 0 &&
               addr_.s6_addr[12] == 0 && addr_.s6_addr[13] == 0 && addr_.s6_addr[14] == 0 &&
               addr_.s6_addr[15] == 1;
    }

    bool IsUnspecified() const noexcept {
        return addr_.s6_addr[0] == 0 && addr_.s6_addr[1] == 0 && addr_.s6_addr[2] == 0 &&
               addr_.s6_addr[3] == 0 && addr_.s6_addr[4] == 0 && addr_.s6_addr[5] == 0 &&
               addr_.s6_addr[6] == 0 && addr_.s6_addr[7] == 0 && addr_.s6_addr[8] == 0 &&
               addr_.s6_addr[9] == 0 && addr_.s6_addr[10] == 0 && addr_.s6_addr[11] == 0 &&
               addr_.s6_addr[12] == 0 && addr_.s6_addr[13] == 0 && addr_.s6_addr[14] == 0 &&
               addr_.s6_addr[15] == 0;
    }

    bool IsV4Mapped() const noexcept {
        return addr_.s6_addr[0] == 0 && addr_.s6_addr[1] == 0 && addr_.s6_addr[2] == 0 &&
               addr_.s6_addr[3] == 0 && addr_.s6_addr[4] == 0 && addr_.s6_addr[5] == 0 &&
               addr_.s6_addr[6] == 0 && addr_.s6_addr[7] == 0 && addr_.s6_addr[8] == 0 &&
               addr_.s6_addr[9] == 0 && addr_.s6_addr[10] == 0xFF && addr_.s6_addr[11] == 0xFF;
    }

    friend bool operator==(AddressV6 lhs, AddressV6 rhs) noexcept {
        return memcmp(&lhs.addr_, &rhs.addr_, sizeof(in6_addr)) == 0 &&
               lhs.scope_id_ == rhs.scope_id_;
    }

    friend bool operator!=(AddressV6 lhs, AddressV6 rhs) noexcept { return !(lhs == rhs); }

    friend bool operator<(AddressV6 lhs, AddressV6 rhs) noexcept {
        auto cmp = memcmp(&lhs.addr_, &rhs.addr_, sizeof(in6_addr));
        return cmp < 0 || (cmp == 0 && lhs.scope_id_ < rhs.scope_id_);
    }

    friend bool operator>(AddressV6 lhs, AddressV6 rhs) noexcept { return rhs < lhs; }

    friend bool operator<=(AddressV6 lhs, AddressV6 rhs) noexcept { return !(lhs > rhs); }

    friend bool operator>=(AddressV6 lhs, AddressV6 rhs) noexcept { return !(lhs < rhs); }

private:
    in6_addr addr_;
    ScopeIdType scope_id_;
};

class Address {
public:
    Address() noexcept = default;

    Address(AddressV4 addr) noexcept : addr_{addr} {}

    Address(AddressV6 addr) noexcept : addr_{addr} {}

    bool IsV4() const noexcept { return std::holds_alternative<AddressV4>(addr_); }

    bool IsV6() const noexcept { return std::holds_alternative<AddressV6>(addr_); }

    AddressV4 ToV4() const noexcept { return std::get<AddressV4>(addr_); }

    AddressV6 ToV6() const noexcept { return std::get<AddressV6>(addr_); }

    std::string ToString() const {
        if (IsV4()) {
            return ToV4().ToString();
        } else {
            return ToV6().ToString();
        }
    }

    bool IsLoopback() const noexcept {
        if (IsV4()) {
            return ToV4().IsLoopback();
        } else {
            return ToV6().IsLoopback();
        }
    }

    bool IsUnspecified() const noexcept {
        if (IsV4()) {
            return ToV4().IsUnspecified();
        } else {
            return ToV6().IsUnspecified();
        }
    }

    friend bool operator==(Address lhs, Address rhs) noexcept { return lhs.addr_ == rhs.addr_; }

    friend bool operator!=(Address lhs, Address rhs) noexcept { return !(lhs == rhs); }

    friend bool operator<(Address lhs, Address rhs) noexcept {
        if (lhs.addr_.index() != rhs.addr_.index()) {
            return lhs.addr_.index() < rhs.addr_.index();
        }
        return lhs.IsV4() ? lhs.ToV4() < rhs.ToV4() : lhs.ToV6() < rhs.ToV6();
    }

    friend bool operator>(Address lhs, Address rhs) noexcept { return rhs < lhs; }

    friend bool operator<=(Address lhs, Address rhs) noexcept { return !(lhs > rhs); }

    friend bool operator>=(Address lhs, Address rhs) noexcept { return !(lhs < rhs); }

private:
    std::variant<AddressV4, AddressV6> addr_;
};

inline AddressV4 MakeAddressV4(AddressV4::UintType addr) noexcept { return AddressV4{addr}; }

inline AddressV4 MakeAddressV4(const char* addr) noexcept {
    AddressV4::BytesType bytes;
    return ::inet_pton(AF_INET, addr, &bytes[0]) > 0 ? AddressV4{bytes} : AddressV4{};
}

inline AddressV4 MakeAddressV4(const std::string& addr) noexcept {
    return MakeAddressV4(addr.c_str());
}

inline AddressV4 MakeAddressV4(std::string_view addr) noexcept {
    return MakeAddressV4(static_cast<std::string>(addr));
}

// TODO: MakeAddressV6

}  // namespace fuchsia::net

namespace std {

template <>
struct hash<fuchsia::net::AddressV4> {
    std::size_t operator()(const fuchsia::net::AddressV4& addr) const noexcept {
        return std::hash<unsigned int>()(addr.ToUint());
    };
};

template <>
struct hash<fuchsia::net::AddressV6> {
    std::size_t operator()(const fuchsia::net::AddressV6& addr) const noexcept {
        const fuchsia::net::AddressV6::BytesType bytes = addr.ToBytes();
        auto result = static_cast<std::size_t>(addr.ScopeId());
        combine_4_bytes(&result, &bytes[0]);
        combine_4_bytes(&result, &bytes[4]);
        combine_4_bytes(&result, &bytes[8]);
        combine_4_bytes(&result, &bytes[12]);
        return result;
    }

private:
    static void combine_4_bytes(std::size_t* seed, const std::byte* bytes) {
        std::byte bytes_hash = bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
        *seed ^= std::to_integer<size_t>(bytes_hash) + 0x9e3779b9 + (*seed << 6) + (*seed >> 2);
    }
};

template <>
struct hash<fuchsia::net::Address> {
    std::size_t operator()(const fuchsia::net::Address& addr) const noexcept {
        return addr.IsV4() ? std::hash<fuchsia::net::AddressV4>()(addr.ToV4())
                           : std::hash<fuchsia::net::AddressV6>()(addr.ToV6());
    }
};

}  // namespace std
