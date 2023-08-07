# fuchsia

Experimental asynchronous networking library built on top of [stdexec](https://github.com/NVIDIA/stdexec) (reference implementation of the [P2300 - std::execution](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2300r7.html) proposal).

# Get started

```bash
mkdir build && cd build
conan install .. (conan 1.x required)
cmake ..
cmake --build .
```

# Example

A lightweight HTTP server is implemented for demonstration, see [examples](examples) for more usage.

```cpp
#include "fuchsia/http/server.h"

exec::task<void> HandleHello(const fuchsia::http::Request& req, fuchsia::http::Response& resp) {
    resp.WriteBody("Hello, world!");
    co_return;
}

exec::task<void> HandleJson(const fuchsia::http::Request& req, fuchsia::http::Response& resp) {
    resp.AddHeader("Content-Type", "application/json");
    resp.WriteBody(R"({"hello": "world"})");
    co_return;
}

int main() {
    fuchsia::http::Server server("0.0.0.0", 8080);
    fuchsia::http::ServeMux mux;
    mux.HandleFunc("/hello", HandleHello);
    mux.HandleFunc("/json", HandleJson);
    server.Serve(mux);
}
```

# Benchmark

```bash
wrk -t12 -c400 -d30s http://127.0.0.1:8080/hello
```

Benchmark with [wrk](https://github.com/wg/wrk) for 30 seconds, using 12 threads, and keeping 400 HTTP connections open:

| Library | Requests/sec | Transfer/sec | Latency avg | Latency stdev | Latency max |
|---------|--------------|--------------|-------------|---------------|-------------|
| fuchsia | 133764       | 13.72MB      | 3.10ms      | 8.16ms        | 373.84ms    |
| muduo   | 132967       | 12.66MB      | 3.05ms      | 2.42ms        | 211.32ms    |
| asio    | 115201       | 11.21MB      | 3.67ms      | 7.22ms        | 340.45ms    |

* Benchmark of [muduo](https://github.com/chenshuo/muduo/tree/2d5c2593b991af1ed9a24d9383f26c2868e3abf1) is based on cpp17 branch, slightly modified (comment out the std::cout log) for best performance.
* Benchmark of [asio](https://github.com/chriskohlhoff/asio/tree/89b0a4138a92883ae2514be68018a6c837a5b65f) is a modified version based on the official http server example of cpp11, changed to pure hello response, and added keep-alive support.
