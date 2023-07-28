//
// Created by wenjuxu on 2023/6/27.
//

#include <iostream>

#include "exec/async_scope.hpp"
#include "exec/static_thread_pool.hpp"
#include "exec/task.hpp"
#include "fuchsia/epoll_context.h"
#include "fuchsia/net/tcp.h"
#include "fuchsia/scope_guard.h"
#include "fuchsia/socket_accept_op.h"
#include "fuchsia/socket_recv_some_op.h"
#include "fuchsia/socket_send_some_op.h"
#include "spdlog/spdlog.h"

struct client {
    uint32_t id;
    std::optional<fuchsia::net::Tcp::Socket> socket;
    char buffer[1024];
};

std::unordered_map<uint32_t, client> clients;

inline uint32_t uuid() {
    static uint32_t n = 0;
    return ++n;
}

exec::task<void> echo(auto id) noexcept {
    auto& socket = clients[id].socket.value();
    auto& buffer = clients[id].buffer;
    try {
        while (true) {
            size_t n1 = co_await fuchsia::AsyncRecvSome(socket, fuchsia::Buffer(buffer));
            spdlog::info("Received {} bytes from client {}", n1, id);
            size_t n2 = co_await fuchsia::AsyncSendSome(socket, fuchsia::Buffer(buffer, n1));
            spdlog::info("Sent {} bytes to client {}", n2, id);
        }
    } catch (std::system_error& e) {
        spdlog::info("Client {} disconnected: {}", id, e.what());
    }
}

exec::task<void> run(auto&& scheduler, auto&& acceptor) noexcept {
    exec::async_scope scope;
    while (true) {
        auto socket = co_await fuchsia::AsyncAccept(acceptor);
        uint32_t id = uuid();
        spdlog::info("Client {} connected: {}", id, socket.Fd());
        clients[id].id = id;
        clients[id].socket = std::move(socket);
        scope.spawn(stdexec::on(scheduler, echo(id)));
    }
}

int main() {
    spdlog::set_level(spdlog::level::trace);

    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    fuchsia::net::Tcp::Endpoint ep{fuchsia::net::AddressV4::Any(), 9876};
    fuchsia::net::Tcp::Acceptor acceptor{context, ep};
    spdlog::info("Server listening on {}", ep.ToString());

    stdexec::scheduler auto scheduler = context.GetScheduler();
    stdexec::sync_wait(run(scheduler, acceptor));
    return 0;
}
