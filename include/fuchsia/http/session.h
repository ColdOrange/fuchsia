//
// Created by wenjuxu on 2023/7/30.
//

#pragma once

#include <set>
#include <string>

#include "exec/task.hpp"
#include "fuchsia/http/message.h"
#include "fuchsia/http/mux.h"
#include "fuchsia/net/tcp.h"

namespace fuchsia::http {

class SessionMgr;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(fuchsia::net::Tcp::Socket socket, SessionMgr& session_mgr, const ServeMux& mux)
        : id_(GenID()), socket_{std::move(socket)}, session_mgr_{session_mgr}, mux_{mux} {}

    Session(const Session&) = delete;

    exec::task<void> Start();
    void Stop();

private:
    static uint64_t GenID();

    uint64_t id_;
    fuchsia::net::Tcp::Socket socket_;
    SessionMgr& session_mgr_;
    const ServeMux& mux_;
    Request request_;
    Response response_;
    char buffer_[8192]{};
};

class SessionMgr {
public:
    SessionMgr() = default;

    SessionMgr(const SessionMgr&) = delete;

    exec::task<void> Start(const std::shared_ptr<Session>& session);

    void Stop(const std::shared_ptr<Session>& session);

    void StopAll();

private:
    std::set<std::shared_ptr<Session>> sessions_;
};

}  // namespace fuchsia::http
