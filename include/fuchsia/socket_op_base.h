//
// Created by wenjuxu on 2023/6/24.
//

#pragma once

#include <sys/epoll.h>

#include "fuchsia/epoll_context.h"

namespace fuchsia {

template <typename Receiver, typename Protocol>
class EpollContext::SocketOperationBase : OperationBase {
public:
    using SocketType = typename Protocol::Socket;
    using ContextType = typename SocketType::ContextType;
    using StopTokenType = stdexec::stop_token_of_t<stdexec::env_of_t<Receiver>>;

    struct Vtable {
        void (*start)(SocketOperationBase*) noexcept = nullptr;
        void (*complete)(SocketOperationBase*) noexcept = nullptr;
    };

    enum class OperationType { Read, Write };

    SocketOperationBase(Receiver receiver, SocketType& socket, const Vtable& vtable,
                        OperationType op_type)
        : receiver_(std::move(receiver)),
          socket_(socket),
          context_(&socket.Context()),
          vtable_(vtable),
          state_(0),
          stop_callback_(stdexec::get_stop_token(stdexec::get_env(receiver_)), *this),
          op_type_(op_type),
          ec_() {}

    friend void tag_invoke(stdexec::start_t, SocketOperationBase& self) noexcept { self.Start(); }

private:
    void Start() noexcept {
        if (context_->IsRunningOnIOThread()) {
            StartLocal();
        } else {
            StartRemote();
        }
    }

    void StartLocal() noexcept {
        vtable_.start(this);
        if (ec_ == std::errc::resource_unavailable_try_again ||
            ec_ == std::errc::operation_would_block) {
            ec_.clear();
            AddEpollEvent();
            execute = &ExecuteOnWakeup;
            return;
        }

        auto old_state = state_.fetch_or(OperationFlags::Completed, std::memory_order_acq_rel);
        if ((old_state & OperationFlags::Cancelled) != 0) {
            // io has been cancelled by a remote thread.
            // The other thread is responsible for enqueueing the operation completion.
            return;
        }

        vtable_.complete(this);
    }

    void StartRemote() noexcept {
        execute = &OnStartRemoteScheduled;
        context_->ScheduleRemote(this);
    }

    static void OnStartRemoteScheduled(OperationBase* op) noexcept {
        static_cast<SocketOperationBase*>(op)->StartLocal();
    }

    static void ExecuteAlreadyCanceled(OperationBase* op) noexcept {
        auto self = static_cast<SocketOperationBase*>(op);
        stdexec::set_stopped(std::move(self->receiver_));
    }

    static void ExecuteOnWakeup(OperationBase* op) noexcept {
        auto self = static_cast<SocketOperationBase*>(op);
        self->RemoveEpollEvent();

        auto old_state =
            self->state_.fetch_or(OperationFlags::Completed, std::memory_order_acq_rel);
        if ((old_state & OperationFlags::Cancelled) != 0) {
            // io has been cancelled by a remote thread.
            // The other thread is responsible for enqueueing the operation completion
            return;
        }

        // trigger start again to obtain the result of this operation.
        self->vtable_.start(self);
        self->vtable_.complete(self);
    }

    void RequestStop() noexcept {
        auto old_state = state_.fetch_or(OperationFlags::Cancelled, std::memory_order_acq_rel);
        if ((old_state & OperationFlags::Completed) == 0) {
            // io not completed yet
            RemoveEpollEvent();
            execute = &ExecuteAlreadyCanceled;
            context_->ScheduleRemote(this);
        } else {
            // io already completed thus cannot be canceled
        }
    }

    void AddEpollEvent() noexcept {
        struct epoll_event event {};
        if (op_type_ == OperationType::Read) {
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
        } else {
            event.events = EPOLLOUT | EPOLLRDHUP | EPOLLHUP;
        }
        event.data.ptr = this;
        if (::epoll_ctl(context_->epoll_fd_, EPOLL_CTL_ADD, socket_.Fd(), &event) != 0) {
            ec_ = std::make_error_code(std::errc(errno));
        }
    }

    void RemoveEpollEvent() noexcept {
        struct epoll_event event {};
        if (::epoll_ctl(context_->epoll_fd_, EPOLL_CTL_DEL, socket_.Fd(), &event) != 0) {
            ec_ = std::make_error_code(std::errc(errno));
        }
    }

    struct StopCallback {
        SocketOperationBase& operation;
        void operator()() noexcept { operation.RequestStop(); }
    };

public:
    Receiver receiver_;
    SocketType& socket_;
    ContextType* context_;
    const Vtable& vtable_;
    std::atomic<uint32_t> state_;
    typename StopTokenType::template callback_type<StopCallback> stop_callback_;
    OperationType op_type_;
    std::error_code ec_;
};

}  // namespace fuchsia
