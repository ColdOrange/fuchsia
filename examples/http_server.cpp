//
// Created by wenjuxu on 2023/7/30.
//

#include "fuchsia/http/server.h"
#include "spdlog/spdlog.h"

void HandleHello(const fuchsia::http::Request& req, fuchsia::http::Response& resp) {
    resp.WriteBody("Hello, world!");
}

void HandleJson(const fuchsia::http::Request& req, fuchsia::http::Response& resp) {
    resp.AddHeader("Content-Type", "application/json");
    resp.WriteBody(R"({"hello": "world"})");
}

int main() {
    spdlog::set_level(spdlog::level::trace);

    fuchsia::http::Server server("0.0.0.0", 8080);
    fuchsia::http::ServeMux mux;
    mux.HandleFunc("/hello", HandleHello);
    mux.HandleFunc("/json", HandleJson);
    server.Serve(mux);
}
