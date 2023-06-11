//
// Created by wenjuxu on 2023/5/12.
//

#include <iostream>

#include "exec/when_any.hpp"
#include "fuchsia/epoll_context.h"
#include "fuchsia/scope_guard.h"
#include "spdlog/spdlog.h"

using namespace std::chrono_literals;

int main() {
    // spdlog::set_level(spdlog::level::trace);  // uncomment this line to enable debug log

    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    stdexec::scheduler auto scheduler = context.GetScheduler();
    static_assert(exec::timed_scheduler<decltype(scheduler)>);  // is also a timed scheduler

    // clang-format off
    auto task = stdexec::when_all(
        stdexec::schedule(scheduler) |
            stdexec::then([] { std::cout << "Hello from task1!\n"; }),
        exec::schedule_at(scheduler, std::chrono::steady_clock::now() + 1500ms) |
            stdexec::then([] { std::cout << "Hello from task2!\n"; }),
        exec::schedule_after(scheduler, 3s) |
            stdexec::then([] { std::cout << "Hello from task3!\n"; }));
    // clang-format on

    stdexec::sync_wait(task);
    return 0;
}
