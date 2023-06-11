//
// Created by wenjuxu on 2023/6/8.
//

#include <iostream>

#include "exec/task.hpp"
#include "exec/when_any.hpp"
#include "fuchsia/epoll_context.h"
#include "fuchsia/scope_guard.h"
#include "spdlog/spdlog.h"

using namespace std::chrono_literals;

exec::task<int> task0(stdexec::scheduler auto sch) {
    co_await exec::schedule_after(sch, 1s);
    co_return 42;
}

exec::task<int> task1(stdexec::scheduler auto sch) {
    auto x = co_await stdexec::stopped_as_optional(task0(sch));
    std::cout << "Task1 done!" << std::endl;
    co_return x.has_value() ? x.value() + 1 : 0;
}

exec::task<std::string> task2(stdexec::scheduler auto sch) {
    co_await exec::schedule_after(sch, 2s);
    std::cout << "Task2 done!" << std::endl;
    co_return "Hello, world!";
}

exec::task<std::string> task3(stdexec::scheduler auto sch) {
    co_await exec::schedule_after(sch, 3s);
    std::cout << "Task3 will be cancelled, and this will not be printed." << std::endl;
    co_return "Hello, world again!";
}

int main() {
    // spdlog::set_level(spdlog::level::trace); // uncomment this line to enable debug log

    fuchsia::EpollContext context;
    std::thread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept {
        context.Stop();
        thread.join();
    }};

    stdexec::scheduler auto sch = context.GetScheduler();

    // clang-format off
    auto [num, str] =
        stdexec::sync_wait(
            stdexec::when_all(
                task1(sch),
                exec::when_any(
                    task2(sch),
                    task3(sch)
                    )
                )
            ).value();
    // clang-format on

    std::cout << "All done, num = " << num << ", str = " << str << std::endl;
    return 0;
}
