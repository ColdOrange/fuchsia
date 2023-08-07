//
// Created by wenjuxu on 2023/7/30.
//

#include "fuchsia/http/server.h"
#include "spdlog/spdlog.h"

exec::task<void> HandleHello(const fuchsia::http::Request& req, fuchsia::http::Response& resp) {
    resp.WriteBody("Hello, world!");
    co_return;
}

exec::task<void> HandleHelloKeepAlive(const fuchsia::http::Request& req,
                                      fuchsia::http::Response& resp) {
    resp.WriteBody("Hello, world!");
    resp.SetKeepAlive(true);
    co_return;
}

exec::task<void> HandleJson(const fuchsia::http::Request& req, fuchsia::http::Response& resp) {
    resp.AddHeader("Content-Type", "application/json");
    resp.WriteBody(R"({"hello": "world"})");
    co_return;
}

int main() {
    spdlog::set_level(spdlog::level::trace);

    fuchsia::http::Server server("0.0.0.0", 8080);
    fuchsia::http::ServeMux mux;
    mux.HandleFunc("/hello", HandleHello);
    mux.HandleFunc("/hello-keep-alive", HandleHelloKeepAlive);  // for benchmark
    mux.HandleFunc("/json", HandleJson);
    server.Serve(mux);
}
