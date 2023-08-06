//
// Created by wenjuxu on 2023/6/24.
//

#pragma once

#include "fuchsia/buffer_sequence_adapter.h"
#include "fuchsia/epoll_context.h"
#include "fuchsia/net/socket.h"
#include "fuchsia/socket_op_base.h"

namespace fuchsia {

template <typename Receiver, typename Protocol, typename Buffers>
class EpollContext::SocketSendSomeOperation : public SocketOperationBase<Receiver, Protocol> {
public:
    using SocketType = typename Protocol::Socket;
    using BaseType = SocketOperationBase<Receiver, Protocol>;
    using BuffersType = BufferSequenceAdapter<ConstBuffer, Buffers>;

    SocketSendSomeOperation(Receiver receiver, SocketType& socket, Buffers buffers)
        : BaseType(std::move(receiver), socket, vtable_, BaseType::OperationType::Write),
          buffers_(buffers),
          bytes_transferred_(0) {}

private:
    static void Start(BaseType* base) noexcept {
        auto self = static_cast<SocketSendSomeOperation*>(base);
        if constexpr (BuffersType::IsSingleBuffer) {
            auto&& single_buffer = BuffersType::First(self->buffers_);
            auto res = self->socket_.Send(single_buffer.Data(), single_buffer.Size(), base->ec_);
            if (!res.has_value()) {
                return;
            }
            self->bytes_transferred_ += res.value();
        } else {
            BuffersType buffers(self->buffers_);
            auto res = self->socket_.SendMsg(buffers.Buffers(), buffers.Count(), base->ec_);
            if (!res.has_value()) {
                return;
            }
            self->bytes_transferred_ += res.value();
        }
    }

    static void Complete(BaseType* base) noexcept {
        if (base->ec_ == std::errc::operation_canceled) {
            stdexec::set_stopped(std::move(base->receiver_));
        } else if (base->ec_) {
            stdexec::set_error(std::move(base->receiver_), base->ec_);
        } else {
            auto self = static_cast<SocketSendSomeOperation*>(base);
            stdexec::set_value(std::move(base->receiver_), self->bytes_transferred_);
        }
    }

    static constexpr typename BaseType::Vtable vtable_{&Start, &Complete};

    Buffers buffers_;
    size_t bytes_transferred_;
};

template <typename Protocol, typename Buffers>
class SocketSendSomeSender {
public:
    template <typename Receiver>
    using OperationType = EpollContext::SocketSendSomeOperation<Receiver, Protocol, Buffers>;
    using SocketType = typename Protocol::Socket;

    SocketSendSomeSender(SocketType& socket, Buffers buffers) noexcept
        : socket_(socket), buffers_(buffers) {}

    using is_sender = void;
    using completion_sigs = stdexec::completion_signatures<stdexec::set_value_t(size_t),
                                                           stdexec::set_error_t(std::error_code),
                                                           stdexec::set_stopped_t()>;

    template <typename Env>
    friend completion_sigs tag_invoke(stdexec::get_completion_signatures_t,
                                      const SocketSendSomeSender&, Env) noexcept {
        return {};
    }

    friend stdexec::empty_env tag_invoke(stdexec::get_env_t,
                                         const SocketSendSomeSender& sender) noexcept {
        return {};
    }

    template <stdexec::__decays_to<SocketSendSomeSender> Sender,
              stdexec::receiver_of<completion_sigs> Receiver>
    friend OperationType<std::remove_cvref_t<Receiver>> tag_invoke(stdexec::connect_t,
                                                                   Sender&& sender,
                                                                   Receiver receiver) noexcept {
        return {std::move(receiver), sender.socket_, sender.buffers_};
    }

private:
    SocketType& socket_;
    Buffers buffers_;
};

namespace cpo {

struct AsyncSendSome {
    template <typename Protocol, ConstBufferSequence Buffers>
    constexpr auto operator()(net::Socket<Protocol>& socket, Buffers buffers) const noexcept
        -> SocketSendSomeSender<Protocol, Buffers> {
        return {socket, buffers};
    }
};

}  // namespace cpo

inline constexpr cpo::AsyncSendSome AsyncSendSome;

}  // namespace fuchsia
