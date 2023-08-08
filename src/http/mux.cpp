//
// Created by wenjuxu on 2023/7/30.
//

#include "fuchsia/http/mux.h"

#include <utility>

namespace fuchsia::http {

void ServeMux::HandleFunc(const std::string& pattern, ServeMux::Handler handler) {
    if (handlers_.find(pattern) != handlers_.end()) {
        throw std::runtime_error("pattern already exists");
    }
    handlers_[pattern] = std::move(handler);
}

const ServeMux::Handler* ServeMux::Match(const std::string& path) const {
    for (const auto& [p, handler] : handlers_) {
        if (p == path) {
            return &handler;
        }
        if (p.back() == '/' && path.size() > p.size() && path.substr(0, p.size()) == p) {
            return &handler;
        }
    }
    return nullptr;
}

ServeMux DefaultServeMux() {
    static ServeMux mux;
    return mux;
}

}  // namespace fuchsia::http
