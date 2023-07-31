//
// Created by wenjuxu on 2023/7/30.
//

#pragma once

#include <string>

#include "exec/async_scope.hpp"
#include "exec/task.hpp"
#include "fuchsia/epoll_context.h"
#include "fuchsia/http/mux.h"
#include "fuchsia/http/session.h"
#include "fuchsia/net/tcp.h"

namespace fuchsia::http {

class Server {
public:
    Server(const std::string& address, int port);

    Server(const Server&) = delete;

    ~Server();

    void Serve(const ServeMux& mux);

private:
    fuchsia::EpollContext context_;
    fuchsia::net::Tcp::Acceptor acceptor_;
    SessionMgr session_mgr_;
    exec::async_scope async_scope_;
};

}  // namespace fuchsia::http
