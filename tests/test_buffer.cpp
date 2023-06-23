//
// Created by wenjuxu on 2023/6/23.
//

#include "catch2/catch_test_macros.hpp"
#include "fuchsia/buffer.h"

TEST_CASE("Constructor of MutableBuffer", "[MutableBuffer]") {
    SECTION("Default constructor") {
        fuchsia::MutableBuffer b;
        REQUIRE(b.Data() == nullptr);
        REQUIRE(b.Size() == 0);
    }
    SECTION("Constructor with data and size") {
        char data[10];
        fuchsia::MutableBuffer b{data, 10};
        REQUIRE(b.Data() == data);
        REQUIRE(b.Size() == 10);
    }
}

TEST_CASE("Constructor of ConstBuffer", "[ConstBuffer]") {
    SECTION("Default constructor") {
        fuchsia::ConstBuffer b;
        REQUIRE(b.Data() == nullptr);
        REQUIRE(b.Size() == 0);
    }
    SECTION("Constructor with data and size") {
        char data[10];
        fuchsia::ConstBuffer b{data, 10};
        REQUIRE(b.Data() == data);
        REQUIRE(b.Size() == 10);
    }
    SECTION("Constructor with MutableBuffer") {
        char data[10];
        fuchsia::MutableBuffer mb{data, 10};
        fuchsia::ConstBuffer b{mb};
        REQUIRE(b.Data() == data);
        REQUIRE(b.Size() == 10);
    }
}

TEST_CASE("Create buffers with Buffer function", "[Buffer]") {
    SECTION("Create from array") {
        char data[10];
        fuchsia::MutableBuffer b = fuchsia::Buffer(data);
        REQUIRE(b.Data() == data);
        REQUIRE(b.Size() == 10);
    }
    SECTION("Create from std::array") {
        std::array<char, 10> data{};
        fuchsia::MutableBuffer b = fuchsia::Buffer(data);
        REQUIRE(b.Data() == data.data());
        REQUIRE(b.Size() == 10);
    }
    SECTION("Create from string") {
        fuchsia::ConstBuffer b = fuchsia::Buffer(std::string{"123"});
        REQUIRE(b.Data() != nullptr);
        REQUIRE(b.Size() == 3);
    }
    SECTION("Create from string_view") {
        fuchsia::ConstBuffer b = fuchsia::Buffer(std::string_view{"123"});
        REQUIRE(b.Data() != nullptr);
        REQUIRE(b.Size() == 3);
    }
    SECTION("Create from vector") {
        fuchsia::ConstBuffer b = fuchsia::Buffer(std::vector<std::byte>{std::byte{0}});
        REQUIRE(b.Data() != nullptr);
        REQUIRE(b.Size() == 1);
    }
}

TEST_CASE("Calculate buffer size with BufferSize function", "[BufferSize]") {
    SECTION("MutableBuffer") {
        char data[10];
        fuchsia::MutableBuffer b = fuchsia::Buffer(data);
        REQUIRE(fuchsia::BufferSize(b) == 10);
    }
    SECTION("ConstBuffer") {
        char data[10];
        fuchsia::ConstBuffer b = fuchsia::Buffer(data);
        REQUIRE(fuchsia::BufferSize(b) == 10);
    }
    SECTION("BufferSequence") {
        std::vector<fuchsia::ConstBuffer> buffers{
            fuchsia::Buffer("123"),  // size == 4 !
            fuchsia::Buffer(std::string{"456"}),
            fuchsia::Buffer(std::string_view{"789"}),
        };
        REQUIRE(fuchsia::BufferSize(buffers) == 10);
    }
}
