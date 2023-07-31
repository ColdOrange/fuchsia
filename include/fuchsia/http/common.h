//
// Created by wenjuxu on 2023/7/30.
//

#pragma once

namespace fuchsia::http {

struct HttpVersion {
    int major;
    int minor;
};

struct Header {
    std::string key{};
    std::string value{};
};

using Headers = std::vector<Header>;

#define HTTP_STATUS_MAP(XX)                                               \
    XX(100, Continue, Continue)                                           \
    XX(101, SwitchingProtocols, Switching Protocols)                      \
    XX(102, Processing, Processing)                                       \
    XX(103, EarlyHints, Early Hints)                                      \
    XX(200, Ok, OK)                                                       \
    XX(201, Created, Created)                                             \
    XX(202, Accepted, Accepted)                                           \
    XX(203, NonAuthoritativeInformation, Non - Authoritative Information) \
    XX(204, NoContent, No Content)                                        \
    XX(205, ResetContent, Reset Content)                                  \
    XX(206, PartialContent, Partial Content)                              \
    XX(207, MultiStatus, Multi - Status)                                  \
    XX(208, AlreadyReported, Already Reported)                            \
    XX(226, ImUsed, IM Used)                                              \
    XX(300, MultipleChoices, Multiple Choices)                            \
    XX(301, MovedPermanently, Moved Permanently)                          \
    XX(302, Found, Found)                                                 \
    XX(303, SeeOther, See Other)                                          \
    XX(304, NotModified, Not Modified)                                    \
    XX(305, UseProxy, Use Proxy)                                          \
    XX(306, Unused, Unused)                                               \
    XX(307, TemporaryRedirect, Temporary Redirect)                        \
    XX(308, PermanentRedirect, Permanent Redirect)                        \
    XX(400, BadRequest, Bad Request)                                      \
    XX(401, Unauthorized, Unauthorized)                                   \
    XX(402, PaymentRequired, Payment Required)                            \
    XX(403, Forbidden, Forbidden)                                         \
    XX(404, NotFound, Not Found)                                          \
    XX(405, MethodNotAllowed, Method Not Allowed)                         \
    XX(406, NotAcceptable, Not Acceptable)                                \
    XX(407, ProxyAuthenticationRequired, Proxy Authentication Required)   \
    XX(408, RequestTimeout, Request Timeout)                              \
    XX(409, Conflict, Conflict)                                           \
    XX(410, Gone, Gone)                                                   \
    XX(411, LengthRequired, Length Required)                              \
    XX(412, PreconditionFailed, Precondition Failed)                      \
    XX(413, PayloadTooLarge, Payload Too Large)                           \
    XX(414, UriTooLong, URI Too Long)                                     \
    XX(415, UnsupportedMediaType, Unsupported Media Type)                 \
    XX(416, RangeNotSatisfiable, Range Not Satisfiable)                   \
    XX(417, ExpectationFailed, Expectation Failed)                        \
    XX(421, MisdirectedRequest, Misdirected Request)                      \
    XX(422, UnprocessableEntity, Unprocessable Entity)                    \
    XX(423, Locked, Locked)                                               \
    XX(424, FailedDependency, Failed Dependency)                          \
    XX(425, TooEarly, Too Early)                                          \
    XX(426, UpgradeRequired, Upgrade Required)                            \
    XX(428, PreconditionRequired, Precondition Required)                  \
    XX(429, TooManyRequests, Too Many Requests)                           \
    XX(431, RequestHeaderFieldsTooLarge, Request Header Fields Too Large) \
    XX(451, UnavailableForLegalReasons, Unavailable For Legal Reasons)    \
    XX(500, InternalServerError, Internal Server Error)                   \
    XX(501, NotImplemented, Not Implemented)                              \
    XX(502, BadGateway, Bad Gateway)                                      \
    XX(503, ServiceUnavailable, Service Unavailable)                      \
    XX(504, GatewayTimeout, Gateway Timeout)                              \
    XX(505, HttpVersionNotSupported, HTTP Version Not Supported)          \
    XX(506, VariantAlsoNegotiates, Variant Also Negotiates)               \
    XX(507, InsufficientStorage, Insufficient Storage)                    \
    XX(508, LoopDetected, Loop Detected)                                  \
    XX(510, NotExtended, Not Extended)                                    \
    XX(511, NetworkAuthenticationRequired, Network Authentication Required)

enum StatusCode {
#define XX(num, name, string) name = (num),
    HTTP_STATUS_MAP(XX)
#undef XX
};

inline std::string_view StatusCodeToString(StatusCode status_code) {
    switch (status_code) {
#define XX(num, name, string) \
    case name:                \
        return #string;
        HTTP_STATUS_MAP(XX)
#undef XX
    }
}

}  // namespace fuchsia::http
