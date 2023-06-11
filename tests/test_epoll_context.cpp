//
// Created by wenjuxu on 2023/6/11.
//

#include <random>

#include "catch2/catch_test_macros.hpp"
#include "exec/repeat_effect_until.hpp"
#include "exec/when_any.hpp"
#include "fuchsia/epoll_context.h"
#include "fuchsia/scope_guard.h"

using namespace std::chrono_literals;

TEST_CASE("EpollContext should be able to run and stop", "[EpollContext]") {
    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};
}

TEST_CASE("EpollContext has an associated (timed) scheduler", "[EpollContext]") {
    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    stdexec::scheduler auto scheduler = context.GetScheduler();
    STATIC_REQUIRE(exec::timed_scheduler<decltype(scheduler)>);
}

TEST_CASE("EpollContext scheduler can create senders", "[EpollContext]") {
    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    stdexec::scheduler auto scheduler = context.GetScheduler();

    stdexec::sender auto sender0 = stdexec::schedule(scheduler);
    stdexec::sender auto sender1 = stdexec::then(sender0, [] { return 42; });

    auto [result] = stdexec::sync_wait(sender1).value();
    REQUIRE(result == 42);
}

TEST_CASE("EpollContext scheduler can also create timer senders", "[EpollContext]") {
    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    stdexec::scheduler auto scheduler = context.GetScheduler();

    // clang-format off
    auto task = stdexec::when_all(
        stdexec::schedule(scheduler) |
            stdexec::then([] { return 42; }),
        exec::schedule_at(scheduler, std::chrono::steady_clock::now() + 10ms) |
            stdexec::then([] { return "Hello"; }),
        exec::schedule_after(scheduler, 5ms) |
            stdexec::then([] { return "World"; }));
    // clang-format on

    auto result = stdexec::sync_wait(task).value();
    REQUIRE(result == std::tuple{42, "Hello", "World"});
}

TEST_CASE("EpollContext scheduler is safe to be called from multiple threads", "[EpollContext]") {
    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    stdexec::scheduler auto scheduler = context.GetScheduler();

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution dist(1, 10);

    uint32_t counter = 0;  // no race because all operations are executed in io thread as for now
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&]() {
            uint32_t inner_counter = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(dist(mt)));
            // clang-format off
            stdexec::sync_wait(
                exec::repeat_effect_until(stdexec::schedule(scheduler) |
                                          stdexec::then([&] {
                                              ++counter;
                                              return ++inner_counter == 100;
                                          })));
            // clang-format on
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    REQUIRE(counter == 10000);
}

TEST_CASE("EpollContext scheduler can cancel timers", "[EpollContext]") {
    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    stdexec::scheduler auto scheduler = context.GetScheduler();

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution dist(1, 10);

    uint32_t counter = 0;  // no race because all operations are executed in io thread as for now
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&]() {
            uint32_t inner_counter = 0;
            stdexec::sync_wait(exec::when_any(
                // only one of the following tasks will be executed, the rest will be canceled
                exec::schedule_after(scheduler, std::chrono::milliseconds(dist(mt))) |
                    stdexec::then([&] { ++counter; }),
                exec::schedule_after(scheduler, std::chrono::milliseconds(dist(mt))) |
                    stdexec::then([&] { ++counter; }),
                exec::schedule_after(scheduler, std::chrono::milliseconds(dist(mt))) |
                    stdexec::then([&] { ++counter; })));
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    REQUIRE(counter == 100);
}
