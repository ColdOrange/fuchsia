//
// Created by wenjuxu on 2023/7/30.
//

#pragma once

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

    void SetStatusCode(fuchsia::http::StatusCode status_code) { status_code_ = status_code; }

    void AddHeader(const std::string& key, const std::string& value) {
        headers_.push_back({key, value});
    }

    void WriteBody(std::string_view data) { body_.append(data); }
};

}  // namespace fuchsia::http
