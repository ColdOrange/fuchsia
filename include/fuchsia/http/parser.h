//
// Created by wenjuxu on 2023/7/30.
//

#pragma once

#include <span>

#include "fuchsia/buffer.h"
#include "fuchsia/http/common.h"
#include "llhttp.h"

namespace fuchsia::http {

enum class MessageType { Request, Response };

enum class ParseResult { Ok, Error, Incomplete };

template <MessageType Type>
class Parser {
public:
    Parser() {
        if constexpr (Type == MessageType::Request) {
            llhttp_init(&parser_, HTTP_REQUEST, &settings_);
        } else {
            llhttp_init(&parser_, HTTP_RESPONSE, &settings_);
        }
        parser_.data = this;
    }

    virtual void Reset() {
        llhttp_reset(&parser_);
        state_ = ParserState::None;
        method_.clear();
        url_.clear();
        status_code_ = fuchsia::http::StatusCode::Ok;
        header_field_.clear();
        headers_.clear();
        body_.clear();
    }

    ParseResult Parse(const char* data, size_t len) {
        auto ret = llhttp_execute(&parser_, data, len);
        if (ret != HPE_OK) {
            return ParseResult::Error;
        } else if (state_ == ParserState::OnMessageComplete) {
            return ParseResult::Ok;
        } else {
            return ParseResult::Incomplete;
        }
    }

    HttpVersion Version() const { return version_; }

    std::string Method() const { return method_; }

    std::string Url() const { return url_; }

    fuchsia::http::StatusCode StatusCode() const { return status_code_; }

    const fuchsia::http::Headers& Headers() const { return headers_; }

    std::string_view Body() const { return body_; }

    std::string_view Header(std::string_view key) const {
        for (const auto& header : headers_) {
            if (header.key == key) {
                return header.value;
            }
        }
        return {};
    }

protected:
    static int OnMessageBegin(llhttp_t* parser) {
        auto self = static_cast<Parser*>(parser->data);
        self->state_ = ParserState::OnMessageBegin;
        return 0;
    }

    static int OnUrl(llhttp_t* parser, const char* data, size_t len) {
        auto self = static_cast<Parser*>(parser->data);
        self->url_.append(data, len);
        self->state_ = ParserState::OnUrl;
        return 0;
    }

    static int OnStatus(llhttp_t* parser, const char* data, size_t len) {
        auto self = static_cast<Parser*>(parser->data);
        self->status_code_ = static_cast<fuchsia::http::StatusCode>(parser->status_code);
        self->state_ = ParserState::OnStatus;
        return 0;
    }

    static int OnMethod(llhttp_t* parser, const char* data, size_t len) {
        auto self = static_cast<Parser*>(parser->data);
        self->method_.append(data, len);
        self->state_ = ParserState::OnMethod;
        return 0;
    }

    static int OnVersionComplete(llhttp_t* parser) {
        auto self = static_cast<Parser*>(parser->data);
        self->version_ = HttpVersion{parser->http_major, parser->http_minor};
        self->state_ = ParserState::OnVersionComplete;
        return 0;
    }

    static int OnHeaderField(llhttp_t* parser, const char* data, size_t len) {
        auto self = static_cast<Parser*>(parser->data);

        self->header_field_.append(data, len);
        self->state_ = ParserState::OnHeaderField;
        return 0;
    }

    static int OnHeaderValue(llhttp_t* parser, const char* data, size_t len) {
        auto self = static_cast<Parser*>(parser->data);
        if (self->state_ == ParserState::OnHeaderField) {
            self->headers_.emplace_back(self->header_field_, std::string(data, len));
            self->header_field_.clear();
        } else if (self->state_ == ParserState::OnHeaderValue) {
            self->headers_.back().value.append(data, len);
        }
        self->state_ = ParserState::OnHeaderValue;
        return 0;
    }

    static int OnHeadersComplete(llhttp_t* parser) {
        auto self = static_cast<Parser*>(parser->data);
        self->state_ = ParserState::OnHeadersComplete;
        return 0;
    }

    static int OnBody(llhttp_t* parser, const char* data, size_t len) {
        auto self = static_cast<Parser*>(parser->data);
        self->body_.append(data, len);
        self->state_ = ParserState::OnBody;
        return 0;
    }

    static int OnMessageComplete(llhttp_t* parser) {
        auto self = static_cast<Parser*>(parser->data);
        self->state_ = ParserState::OnMessageComplete;
        return 0;
    }

    static int OnChunkHeader(llhttp_t* parser) {
        auto self = static_cast<Parser*>(parser->data);
        self->state_ = ParserState::OnChunkHeader;
        return 0;
    }

    static int OnChunkComplete(llhttp_t* parser) {
        auto self = static_cast<Parser*>(parser->data);
        self->state_ = ParserState::OnChunkComplete;
        return 0;
    }

    inline static llhttp_settings_t settings_ = {
        .on_message_begin = OnMessageBegin,
        .on_url = OnUrl,
        .on_status = OnStatus,
        .on_method = OnMethod,
        .on_header_field = OnHeaderField,
        .on_header_value = OnHeaderValue,
        .on_headers_complete = OnHeadersComplete,
        .on_body = OnBody,
        .on_message_complete = OnMessageComplete,
        .on_version_complete = OnVersionComplete,
        .on_chunk_header = OnChunkHeader,
        .on_chunk_complete = OnChunkComplete,
    };

    enum class ParserState {
        None,
        OnMessageBegin,
        OnUrl,
        OnStatus,
        OnMethod,
        OnHeaderField,
        OnHeaderValue,
        OnHeadersComplete,
        OnBody,
        OnMessageComplete,
        OnVersionComplete,
        OnChunkHeader,
        OnChunkComplete,
    };

    llhttp_t parser_;
    ParserState state_ = ParserState::None;
    HttpVersion version_;
    std::string method_;
    std::string url_;
    fuchsia::http::StatusCode status_code_ = fuchsia::http::StatusCode::Ok;
    std::string header_field_;
    fuchsia::http::Headers headers_;
    std::string body_;
};

}  // namespace fuchsia::http
