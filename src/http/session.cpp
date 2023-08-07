//
// Created by wenjuxu on 2023/7/30.
//

#include "fuchsia/http/session.h"

#include <sstream>

#include "fuchsia/logging.h"
#include "fuchsia/socket_recv_some_op.h"
#include "fuchsia/socket_send_some_op.h"

namespace fuchsia::http {

exec::task<void> Session::Start() { co_await AsyncRecvRequest(); }

void Session::Stop() { socket_.Close(); }

exec::task<void> Session::AsyncRecvRequest() {
    while (true) {
        auto size = co_await fuchsia::AsyncRecvSome(socket_, fuchsia::Buffer(buffer_));
        auto result = request_.Parse(buffer_, size);
        if (result == ParseResult::Incomplete) {
            continue;
        }

        co_await AsyncHandleRequest(result);
        bool shutdown = co_await AsyncSendResponse();
        if (shutdown) {
            break;
        }
    }
}

exec::task<void> Session::AsyncHandleRequest(ParseResult parse_result) {
    LOG_TRACE("Session {} recv request: {} {}", id_, request_.Method(), request_.Url());
    if (parse_result == ParseResult::Error) {
        response_.SetStatusCode(StatusCode::BadRequest);
    } else {
        auto handler = mux_.Match(request_.Url());
        if (handler == nullptr) {
            response_.SetStatusCode(StatusCode::NotFound);
        } else {
            co_await handler->operator()(request_, response_);
        }
    }
}

static std::string BuildHeader(Response& response) {
    if (!response.Body().empty()) {
        response.AddHeader("Content-Length", std::to_string(response.Body().size()));
        if (response.Header("Content-Type").empty()) {
            response.AddHeader("Content-Type", "text/plain");
        }
    }

    std::stringstream ss;
    ss << "HTTP/1.1 " << static_cast<int>(response.StatusCode()) << " "
       << StatusCodeToString(response.StatusCode()) << "\r\n";
    for (const auto& header : response.Headers()) {
        ss << header.key << ": " << header.value << "\r\n";
    }
    ss << "Connection: " << (response.KeepAlive() ? "keep-alive" : "close") << "\r\n";
    ss << "\r\n";
    return ss.str();
}

exec::task<bool> Session::AsyncSendResponse() {
    const auto& header = BuildHeader(response_);
    LOG_TRACE("Session {} send response: {}{}", id_, header, response_.Body());
    std::vector<fuchsia::ConstBuffer> buffers{
        fuchsia::ConstBuffer(header.data(), header.size()),
        fuchsia::ConstBuffer(response_.Body().data(), response_.Body().size()),
    };
    co_await fuchsia::AsyncSendSome(socket_, buffers);
    if (response_.KeepAlive()) {
        request_.Reset();
        response_.Reset();
        co_return false;
    } else {
        socket_.Shutdown(fuchsia::net::ShutdownMode::Both);
        session_mgr_.Stop(shared_from_this());
        co_return true;
    }
}

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
