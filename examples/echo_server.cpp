//
// Created by wenjuxu on 2023/6/24.
//

#include "exec/repeat_effect_until.hpp"
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

inline uint32_t uuid() {
    static uint32_t n = 0;
    return ++n;
}

int main() {
    spdlog::set_level(spdlog::level::info);

    fuchsia::EpollContext context;
    std::jthread thread([&]() { context.Run(); });
    fuchsia::ScopeGuard guard{[&]() noexcept { context.Stop(); }};

    fuchsia::net::Tcp::Endpoint ep{fuchsia::net::AddressV4::Any(), 9876};
    fuchsia::net::Tcp::Acceptor acceptor{context, ep};
    spdlog::info("Server listening on {}", ep.ToString());

    std::unordered_map<uint32_t, client> clients;

    // clang-format off
    stdexec::sender auto task =
        fuchsia::AsyncAccept(acceptor)
        | stdexec::then([&clients](auto&& _socket) noexcept {
            uint32_t id = uuid();
            spdlog::info("Client {} connected: {}", id, _socket.Fd());

            clients[id].id = id;
            clients[id].socket = std::move(_socket);
            auto& socket = clients[id].socket.value();
            auto& buffer = clients[id].buffer;

            stdexec::sender auto echo =
                fuchsia::AsyncRecvSome(socket, fuchsia::Buffer(buffer))
                | stdexec::let_value([&socket, &buffer, id](size_t n_received) noexcept {
                    spdlog::info("Received {} bytes from client {}", n_received, id);
                    fuchsia::ConstBuffer const_buffer{buffer, n_received};
                    return fuchsia::AsyncSendSome(socket, const_buffer)
                           | stdexec::then([id](size_t n_sent) noexcept {
                               spdlog::info("Sent {} bytes to client {}", n_sent, id);
                               return false;
                           });
                })
                | stdexec::upon_error([&socket, id](auto&&) noexcept {
                    spdlog::info("Client {} disconnected", id);
                    socket.Close();
                    return true;
                })
                | exec::repeat_effect_until();

            stdexec::start_detached(std::move(echo));
            return false;
        })
        | stdexec::upon_error([](std::error_code error) noexcept {
            spdlog::error("Error: {}", error.message());
            return true;
        })
        | exec::repeat_effect_until();
    // clang-format on

    stdexec::sync_wait(task);

    return 0;
}
