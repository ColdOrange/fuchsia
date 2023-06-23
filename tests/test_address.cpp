//
// Created by wenjuxu on 2023/6/23.
//

#include <unordered_set>

#include "catch2/catch_test_macros.hpp"
#include "fuchsia/net/address.h"

TEST_CASE("Constructor of AddressV4", "[AddressV4]") {
    SECTION("Default constructor") {
        fuchsia::net::AddressV4 addr;
        REQUIRE(addr.ToUint() == 0);
    }
    SECTION("Construct from uint32_t") {
        fuchsia::net::AddressV4 addr{0x01020304};
        REQUIRE(addr.ToUint() == 0x01020304);
    }
    SECTION("Construct from bytes") {
        fuchsia::net::AddressV4::BytesType bytes{std::byte{1}, std::byte{2}, std::byte{3},
                                                 std::byte{4}};
        fuchsia::net::AddressV4 addr{bytes};
        REQUIRE(addr.ToUint() == 0x01020304);
    }
}

TEST_CASE("MakeAddressV4", "[AddressV4]") {
    SECTION("MakeAddressV4 from uint32_t") {
        auto addr = fuchsia::net::MakeAddressV4(0x01020304);
        REQUIRE(addr.ToUint() == 0x01020304);
    }
    SECTION("MakeAddressV4 from string") {
        auto addr = fuchsia::net::MakeAddressV4("127.0.0.1");
        REQUIRE(addr.ToString() == "127.0.0.1");
        REQUIRE(addr.ToUint() == 0x7F000001);
        REQUIRE(addr.IsLoopback());
    }
}

TEST_CASE("Hash of AddressV4", "[Hash][AddressV4]") {
    std::unordered_set<fuchsia::net::AddressV4> s;
    s.emplace(0x01020304);
    s.emplace(0x7F000001);
    s.emplace(fuchsia::net::MakeAddressV4("127.0.0.1"));
    REQUIRE(s.size() == 2);
}
