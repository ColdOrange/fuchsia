//
// Created by wenjuxu on 2023/7/30.
//

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "exec/task.hpp"
#include "fuchsia/http/message.h"

namespace fuchsia::http {

class ServeMux {
public:
    using Handler = std::function<exec::task<void>(const Request& req, Response& resp)>;

    ServeMux() = default;
    ~ServeMux() = default;

    void HandleFunc(const std::string& pattern, Handler handler);

    const Handler* Match(const std::string& path) const;

private:
    std::map<std::string, Handler> handlers_;
};

ServeMux DefaultServeMux();

}  // namespace fuchsia::http
