//
// Created by wenjuxu on 2023/7/30.
//

#include "fuchsia/http/session.h"

#include <sstream>

#include "fuchsia/logging.h"
#include "fuchsia/socket_recv_some_op.h"
#include "fuchsia/socket_send_some_op.h"

namespace fuchsia::http {

exec::task<void> Session::Start() {
    while (true) {
        auto size = co_await fuchsia::AsyncRecvSome(socket_, fuchsia::Buffer(buffer_));
        auto result = request_.Parse(buffer_, size);
        if (result == ParseResult::Incomplete) {
            continue;
        }

        LOG_TRACE("Session {} recv request: {} {}", id_, request_.Method(), request_.Url());
        if (result == ParseResult::Error) {
            response_.SetStatusCode(StatusCode::BadRequest);
        } else {  // ParseResult::Ok
            auto handler = mux_.Match(request_.Url());
            if (handler == nullptr) {
                response_.SetStatusCode(StatusCode::NotFound);
            } else {
                co_await handler->operator()(request_, response_);
            }
        }

        LOG_TRACE("Session {} send response: {}", id_, response_.StatusCode());
        co_await fuchsia::AsyncSendSome(socket_, response_.ToBuffers());
        if (response_.KeepAlive()) {
            request_.Reset();
            response_.Reset();
        } else {
            socket_.Shutdown(fuchsia::net::ShutdownMode::Both);
            session_mgr_.Stop(shared_from_this());
            break;
        }
    }
}

void Session::Stop() { socket_.Close(); }

uint64_t Session::GenID() {
    static uint64_t id = 0;
    return ++id;
}

exec::task<void> SessionMgr::Start(const std::shared_ptr<Session>& session) {
    sessions_.insert(session);
    co_await session->Start();
}

void SessionMgr::Stop(const std::shared_ptr<Session>& session) {
    sessions_.erase(session);
    session->Stop();
}

void SessionMgr::StopAll() {
    for (const auto& session : sessions_) {
        session->Stop();
    }
    sessions_.clear();
}

}  // namespace fuchsia::http
