//
// Created by wenjuxu on 2023/6/24.
//

#pragma once

#include "fuchsia/epoll_context.h"
#include "fuchsia/socket_op_base.h"

namespace fuchsia {

template <typename Receiver, typename Protocol>
class EpollContext::SocketAcceptOperation : public SocketOperationBase<Receiver, Protocol> {
public:
    using AcceptorType = typename Protocol::Acceptor;
    using EndpointType = typename Protocol::Endpoint;
    using SocketType = typename Protocol::Socket;
    using BaseType = SocketOperationBase<Receiver, Protocol>;

    SocketAcceptOperation(Receiver receiver, AcceptorType& acceptor)
        : BaseType(std::move(receiver), acceptor, vtable_, BaseType::OperationType::Read),
          acceptor_(acceptor),
          conn_socket_(acceptor.Context()) {}

private:
    static void Start(BaseType* base) noexcept {
        auto self = static_cast<SocketAcceptOperation*>(base);
        auto res = self->acceptor_.Accept(base->ec_);
        if (!res.has_value()) {
            return;
        }
        std::tie(self->conn_socket_, self->peer_endpoint_) = std::move(res.value());
    }

    static void Complete(BaseType* base) noexcept {
        if (base->ec_ == std::errc::operation_canceled) {
            stdexec::set_stopped(std::move(base->receiver_));
        } else if (base->ec_) {
            stdexec::set_error(std::move(base->receiver_), base->ec_);
        } else {
            auto self = static_cast<SocketAcceptOperation*>(base);
            stdexec::set_value(std::move(base->receiver_), std::move(self->conn_socket_));
        }
    }

    static constexpr typename BaseType::Vtable vtable_{&Start, &Complete};

    AcceptorType& acceptor_;
    SocketType conn_socket_;
    EndpointType peer_endpoint_;
};

template <typename Protocol>
class SocketAcceptSender {
public:
    template <typename Receiver>
    using OperationType = EpollContext::SocketAcceptOperation<Receiver, Protocol>;
    using AcceptorType = typename Protocol::Acceptor;
    using EndpointType = typename Protocol::Endpoint;
    using SocketType = typename Protocol::Socket;

    explicit SocketAcceptSender(AcceptorType& acceptor) noexcept : acceptor_(acceptor) {}

    using is_sender = void;
    using completion_sigs = stdexec::completion_signatures<stdexec::set_value_t(SocketType&&),
                                                           stdexec::set_error_t(std::error_code),
                                                           stdexec::set_stopped_t()>;

    template <typename Env>
    friend completion_sigs tag_invoke(stdexec::get_completion_signatures_t,
                                      const SocketAcceptSender&, Env) noexcept {
        return {};
    }

    friend stdexec::empty_env tag_invoke(stdexec::get_env_t,
                                         const SocketAcceptSender& sender) noexcept {
        return {};
    }

    template <stdexec::__decays_to<SocketAcceptSender> Sender,
              stdexec::receiver_of<completion_sigs> Receiver>
    friend OperationType<std::remove_cvref_t<Receiver>> tag_invoke(stdexec::connect_t,
                                                                   Sender&& sender,
                                                                   Receiver receiver) noexcept {
        return {std::move(receiver), sender.acceptor_};
    }

private:
    AcceptorType& acceptor_;
};

namespace cpo {

struct AsyncAccept {
    template <typename Protocol>
    constexpr auto operator()(net::Acceptor<Protocol>& acceptor) const noexcept
        -> SocketAcceptSender<Protocol> {
        return SocketAcceptSender<Protocol>{acceptor};
    }
};

}  // namespace cpo

inline constexpr cpo::AsyncAccept AsyncAccept;

}  // namespace fuchsia
