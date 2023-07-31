//
// Created by wenjuxu on 2023/7/30.
//

#include "fuchsia/http/server.h"

#include "exec/async_scope.hpp"
#include "fuchsia/http/session.h"
#include "fuchsia/logging.h"
#include "fuchsia/socket_accept_op.h"

namespace fuchsia::http {

Server::Server(const std::string& address, int port)
    : context_(),
      acceptor_{context_, fuchsia::net::Tcp::Endpoint{fuchsia::net::MakeAddressV4(address),
                                                      static_cast<fuchsia::net::PortType>(port)}} {}

Server::~Server() {
    async_scope_.request_stop();
    context_.Stop();
    acceptor_.Close();
    session_mgr_.StopAll();
}

static exec::task<void> StartSession(SessionMgr& session_mgr,
                                     std::shared_ptr<Session> session /* need a copy here */) {
    try {
        co_await session_mgr.Start(session);
    } catch (const std::exception& e) {
        LOG_ERROR("Session error: {}", e.what());
    }
}

void Server::Serve(const ServeMux& mux) {
    std::jthread thread{[this] { context_.Run(); }};
    stdexec::sync_wait([this, &mux]() -> exec::task<void> {
        while (true) {
            auto socket = co_await fuchsia::AsyncAccept(acceptor_);
            auto session = std::make_shared<Session>(std::move(socket), session_mgr_, mux);
            async_scope_.spawn(
                stdexec::on(context_.GetScheduler(), StartSession(session_mgr_, session)));
        }
    }());
}

}  // namespace fuchsia::http
