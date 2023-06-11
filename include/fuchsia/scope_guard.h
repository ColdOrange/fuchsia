//
// Created by wenjuxu on 2023/6/6.
//

#pragma once

#include "stdexec/execution.hpp"

namespace fuchsia {

template <class Fn, class... Ts>
requires stdexec::__nothrow_callable<Fn, Ts...>
struct ScopeGuard {
    stdexec::__scope_guard<Fn, Ts...> guard;

    void Dismiss() noexcept { guard.__dismiss(); }
};

template <class Fn, class... Ts>
ScopeGuard(Fn, Ts...) -> ScopeGuard<Fn, Ts...>;

}  // namespace fuchsia
