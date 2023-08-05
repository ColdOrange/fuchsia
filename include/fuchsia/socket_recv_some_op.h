//
// Created by wenjuxu on 2023/6/24.
//

#pragma once

#include "fuchsia/buffer.h"
#include "fuchsia/epoll_context.h"
#include "fuchsia/net/socket.h"
#include "fuchsia/socket_op_base.h"

namespace fuchsia {

template <typename Receiver, typename Protocol>
class EpollContext::SocketRecvSomeOperation : public SocketOperationBase<Receiver, Protocol> {
public:
    using SocketType = typename Protocol::Socket;
    using BaseType = SocketOperationBase<Receiver, Protocol>;

    SocketRecvSomeOperation(Receiver receiver, SocketType& socket, MutableBuffer buffer)
        : BaseType(std::move(receiver), socket, vtable_, BaseType::OperationType::Read),
          buffer_(buffer),
          bytes_transferred_(0) {}

private:
    static void Start(BaseType* base) noexcept {
        auto self = static_cast<SocketRecvSomeOperation*>(base);
        auto res = self->socket_.Recv(self->buffer_.Data(), self->buffer_.Size(), base->ec_);
        if (!res.has_value()) {
            return;
        }
        self->bytes_transferred_ += res.value();
    }

    static void Complete(BaseType* base) noexcept {
        if (base->ec_ == std::errc::operation_canceled) {
            stdexec::set_stopped(std::move(base->receiver_));
        } else if (base->ec_) {
            stdexec::set_error(std::move(base->receiver_), base->ec_);
        } else {
            auto self = static_cast<SocketRecvSomeOperation*>(base);
            stdexec::set_value(std::move(base->receiver_), self->bytes_transferred_);
        }
    }

    static constexpr typename BaseType::Vtable vtable_{&Start, &Complete};

    MutableBuffer buffer_;
    size_t bytes_transferred_;
};

template <typename Protocol>
class SocketRecvSomeSender {
public:
    template <typename Receiver>
    using OperationType = EpollContext::SocketRecvSomeOperation<Receiver, Protocol>;
    using SocketType = typename Protocol::Socket;

    explicit SocketRecvSomeSender(SocketType& socket, MutableBuffer buffer) noexcept
        : socket_(socket), buffer_(buffer) {}

    using is_sender = void;
    using completion_sigs = stdexec::completion_signatures<stdexec::set_value_t(size_t),
                                                           stdexec::set_error_t(std::error_code),
                                                           stdexec::set_stopped_t()>;

    template <typename Env>
    friend completion_sigs tag_invoke(stdexec::get_completion_signatures_t,
                                      const SocketRecvSomeSender&, Env) noexcept {
        return {};
    }

    friend stdexec::empty_env tag_invoke(stdexec::get_env_t,
                                         const SocketRecvSomeSender& sender) noexcept {
        return {};
    }

    template <stdexec::__decays_to<SocketRecvSomeSender> Sender,
              stdexec::receiver_of<completion_sigs> Receiver>
    friend OperationType<std::remove_cvref_t<Receiver>> tag_invoke(stdexec::connect_t,
                                                                   Sender&& sender,
                                                                   Receiver receiver) noexcept {
        return {std::move(receiver), sender.socket_, sender.buffer_};
    }

private:
    SocketType& socket_;
    MutableBuffer buffer_;
};

namespace cpo {

struct AsyncRecvSome {
    template <typename Protocol>
    constexpr auto operator()(net::Socket<Protocol>& socket, MutableBuffer buffer) const noexcept
        -> SocketRecvSomeSender<Protocol> {
        return SocketRecvSomeSender<Protocol>{socket, buffer};
    }
};

}  // namespace cpo

inline constexpr cpo::AsyncRecvSome AsyncRecvSome;

}  // namespace fuchsia
