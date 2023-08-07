//
// Created by wenjuxu on 2023/7/30.
//

#pragma once

#include <fmt/format.h>

#include "fuchsia/http/common.h"
#include "fuchsia/http/parser.h"

namespace fuchsia::http {

class Request : public Parser<MessageType::Request> {
public:
    using Parser::Body;
    using Parser::Header;
    using Parser::Headers;
    using Parser::Method;
    using Parser::Url;
    using Parser::Version;

    Request() = default;

    // TODO: client side methods
};

class Response : public Parser<MessageType::Response> {
public:
    using Parser::Body;
    using Parser::Header;
    using Parser::Headers;
    using Parser::StatusCode;
    using Parser::Version;

    Response() = default;

    void Reset() override {
        Parser::Reset();
        keep_alive_ = false;
    }

    void SetStatusCode(fuchsia::http::StatusCode status_code) { status_code_ = status_code; }

    bool KeepAlive() const { return keep_alive_; }
    void SetKeepAlive(bool on) { keep_alive_ = on; }

    void AddHeader(const std::string& key, const std::string& value) {
        headers_.push_back({key, value});
    }

    void WriteBody(std::string_view data) { body_.append(data); }

    std::vector<fuchsia::ConstBuffer> ToBuffers() {
        header_buffer_.clear();
        fmt::format_to(std::back_inserter(header_buffer_), "HTTP/1.1 {} {}\r\n",
                       static_cast<int>(status_code_), StatusCodeToString(status_code_));
        bool has_content_type = false;
        for (const auto& header : headers_) {
            fmt::format_to(std::back_inserter(header_buffer_), "{}: {}\r\n", header.key,
                           header.value);
            if (header.key == "Content-Type") {
                has_content_type = true;
            }
        }
        if (!body_.empty()) {
            fmt::format_to(std::back_inserter(header_buffer_), "Content-Length: {}\r\n",
                           body_.size());
            if (!has_content_type) {
                fmt::format_to(std::back_inserter(header_buffer_), "Content-Type: text/plain\r\n");
            }
        }
        if (keep_alive_) {
            fmt::format_to(std::back_inserter(header_buffer_), "Connection: keep-alive\r\n");
        } else {
            fmt::format_to(std::back_inserter(header_buffer_), "Connection: close\r\n");
        }
        fmt::format_to(std::back_inserter(header_buffer_), "\r\n");

        return {fuchsia::Buffer(header_buffer_), fuchsia::Buffer(body_)};
    }

private:
    bool keep_alive_{false};
    std::string header_buffer_;
};

}  // namespace fuchsia::http
