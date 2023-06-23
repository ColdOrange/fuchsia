//
// Created by wenjuxu on 2023/6/23.
//

#include <unordered_set>

#include "catch2/catch_test_macros.hpp"
#include "fuchsia/net/endpoint.h"
#include "fuchsia/net/tcp.h"
#include "fuchsia/net/udp.h"

TEST_CASE("Constructor of Endpoint", "[Endpoint]") {
    SECTION("Default constructor") {
        fuchsia::net::Tcp::Endpoint ep;
        REQUIRE(ep.IsV4());
        REQUIRE(ep.Address().ToV4().ToUint() == 0);
        REQUIRE(ep.Port() == 0);
    }
    SECTION("Construct from family and port") {
        fuchsia::net::Udp::Endpoint ep{fuchsia::net::Udp::V4(), 80};
        REQUIRE(ep.IsV4());
        REQUIRE(ep.Address().ToV4().ToUint() == 0);
        REQUIRE(ep.Port() == 80);
    }
    SECTION("Construct from address and port") {
        fuchsia::net::Tcp::Endpoint ep{fuchsia::net::AddressV4::Loopback(), 80};
        REQUIRE(ep.IsV4());
        REQUIRE(ep.Address().ToV4().ToUint() == 0x7F000001);
        REQUIRE(ep.Port() == 80);
    }
}

TEST_CASE("Copy and move of Endpoint", "[Endpoint]") {
    SECTION("Copy constructor") {
        fuchsia::net::Tcp::Endpoint ep1{fuchsia::net::AddressV4::Loopback(), 80};
        fuchsia::net::Tcp::Endpoint ep2{ep1};
        REQUIRE(ep2.IsV4());
        REQUIRE(ep2.Address().ToV4().ToUint() == 0x7F000001);
        REQUIRE(ep2.Port() == 80);
    }
    SECTION("Copy assignment") {
        fuchsia::net::Tcp::Endpoint ep1{fuchsia::net::AddressV4::Loopback(), 80};
        fuchsia::net::Tcp::Endpoint ep2;
        ep2 = ep1;
        REQUIRE(ep2.IsV4());
        REQUIRE(ep2.Address().ToV4().ToUint() == 0x7F000001);
        REQUIRE(ep2.Port() == 80);
    }
    SECTION("Move constructor") {
        fuchsia::net::Tcp::Endpoint ep1{fuchsia::net::AddressV4::Loopback(), 80};
        fuchsia::net::Tcp::Endpoint ep2{std::move(ep1)};
        REQUIRE(ep2.IsV4());
        REQUIRE(ep2.Address().ToV4().ToUint() == 0x7F000001);
        REQUIRE(ep2.Port() == 80);
    }
    SECTION("Move assignment") {
        fuchsia::net::Tcp::Endpoint ep1{fuchsia::net::AddressV4::Loopback(), 80};
        fuchsia::net::Tcp::Endpoint ep2;
        ep2 = std::move(ep1);
        REQUIRE(ep2.IsV4());
        REQUIRE(ep2.Address().ToV4().ToUint() == 0x7F000001);
        REQUIRE(ep2.Port() == 80);
    }
}

TEST_CASE("Hash of Endpoint", "[Hash][Endpoint]") {
    std::unordered_set<fuchsia::net::Endpoint<fuchsia::net::Tcp>> s;
    s.emplace(fuchsia::net::AddressV4::Loopback(), 80);
    s.emplace(fuchsia::net::MakeAddressV4("127.0.0.1"), 80);
    REQUIRE(s.size() == 1);
}
