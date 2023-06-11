//
// Created by wenjuxu on 2023/5/13.
//

#pragma once

#include <atomic>
#include <chrono>
#include <optional>

#include "exec/timed_scheduler.hpp"
#include "fuchsia/atomic_intrusive_queue.h"
#include "fuchsia/intrusive_priority_queue.h"
#include "fuchsia/intrusive_queue.h"
#include "stdexec/execution.hpp"

namespace fuchsia {

class EpollContext {
public:
    EpollContext();
    ~EpollContext();

    class Scheduler;
    Scheduler GetScheduler() noexcept;

    void Run();
    void Stop() noexcept;

private:
    struct OperationBase {
#ifndef NDEBUG
        uint64_t uuid;
        static inline std::atomic<uint64_t> uuid_generator = 1;
        OperationBase() noexcept : uuid(uuid_generator.fetch_add(1, std::memory_order_acq_rel)) {}
#else
        OperationBase() noexcept = default;
#endif
        // Operations are neither copyable nor movable
        OperationBase(OperationBase&&) = delete;
        OperationBase(const OperationBase&) = delete;

        using ExecuteFunction = void(OperationBase*) noexcept;
        std::atomic<ExecuteFunction*> execute = nullptr;
        OperationBase* next = nullptr;
    };

    struct OperationFlags {
        static constexpr uint32_t None = 0;
        static constexpr uint32_t Cancelled = 1 << 0;
        static constexpr uint32_t Expired = 1 << 1;
        static constexpr uint32_t Completed = 1 << 2;
    };

    using TimePoint = std::chrono::steady_clock::time_point;

    struct TimerOperation : OperationBase {
        TimerOperation(EpollContext& context, TimePoint expiration) noexcept
            : context(context), expiration(expiration) {}

        bool operator<(const TimerOperation& other) const noexcept {
            return expiration < other.expiration;
        }

        EpollContext& context;
        TimePoint expiration;
        TimerOperation* prev = nullptr;
        TimerOperation* next = nullptr;
        std::atomic<uint32_t> state = 0;
    };

private:
    void Schedule(OperationBase* op) noexcept;
    void ScheduleLocal(OperationBase* op) noexcept;
    void ScheduleRemote(OperationBase* op) noexcept;
    void ScheduleAt(TimerOperation* op) noexcept;
    void RemoveTimer(TimerOperation* op) noexcept;

    bool IsRunningOnIOThread() const noexcept;
    void ProcessLocalOperations() noexcept;
    bool ProcessRemoteOperations() noexcept;
    void ProcessTimers() noexcept;
    void UpdateNextExpirationTime() noexcept;
    void BlockingWaitEvents();

    void Wakeup();
    static void Drain(int fd);

private:
    using OperationQueue = IntrusiveQueue<OperationBase>;
    using RemoteOperationQueue = AtomicIntrusiveQueue<OperationBase>;
    using TimerQueue = IntrusivePriorityQueue<TimerOperation>;

    int epoll_fd_ = -1;
    int timer_fd_ = -1;
    int wakeup_fd_ = -1;
    OperationQueue local_operation_queue_;
    RemoteOperationQueue remote_operation_queue_;
    TimerQueue timer_queue_;
    std::optional<TimePoint> next_expiration_time_;
    stdexec::in_place_stop_source stop_source_;
};

class EpollContext::Scheduler {
    struct ScheduleEnv {
        EpollContext* context;
        friend Scheduler tag_invoke(stdexec::get_completion_scheduler_t<stdexec::set_value_t>,
                                    const ScheduleEnv& env) noexcept {
            return Scheduler(env.context);
        }
    };

    class ScheduleSender {
        template <typename Receiver>
        class Operation : OperationBase {
        public:
            Operation(EpollContext& context, Receiver&& receiver) noexcept
                : context_(context), receiver_(std::move(receiver)) {
                execute = &Execute;
            }

            friend void tag_invoke(stdexec::start_t, Operation& op) noexcept { op.Start(); }

        private:
            void Start() noexcept { context_.Schedule(this); }

            static void Execute(OperationBase* op) noexcept {
                auto self = static_cast<Operation*>(op);
                if (stdexec::get_stop_token(self->receiver_).stop_requested()) {
                    stdexec::set_stopped(std::move(self->receiver_));
                    return;
                }
                try {
                    stdexec::set_value(std::move(self->receiver_));
                } catch (...) {
                    stdexec::set_error(std::move(self->receiver_), std::current_exception());
                }
            }

            EpollContext& context_;
            Receiver receiver_;
        };

    public:
        using is_sender = void;
        using completion_sigs =
            stdexec::completion_signatures<stdexec::set_value_t(),
                                           stdexec::set_error_t(std::exception_ptr),
                                           stdexec::set_stopped_t()>;

        template <typename Env>
        friend completion_sigs tag_invoke(stdexec::get_completion_signatures_t,
                                          const ScheduleSender&, Env) noexcept {
            return {};
        }

        friend ScheduleEnv tag_invoke(stdexec::get_env_t, const ScheduleSender& sender) noexcept {
            return sender.env_;
        }

        template <stdexec::receiver_of<completion_sigs> Receiver>
        friend Operation<std::remove_cvref_t<Receiver>> tag_invoke(stdexec::connect_t,
                                                                   const ScheduleSender& sender,
                                                                   Receiver&& receiver) noexcept {
            return {*sender.env_.context, std::forward<Receiver>(receiver)};
        }

    private:
        friend class Scheduler;
        explicit ScheduleSender(ScheduleEnv env) noexcept : env_(env) {}

        ScheduleEnv env_;
    };

    class ScheduleAtSender {
        template <typename Receiver>
        class Operation : TimerOperation {
        public:
            Operation(EpollContext& context, TimePoint expiration, Receiver&& receiver) noexcept
                : TimerOperation(context, expiration),
                  receiver_(std::move(receiver)),
                  stop_callback_(stdexec::get_stop_token(stdexec::get_env(receiver_)), *this) {
                // TODO: context stop callback
            }

            friend void tag_invoke(stdexec::start_t, Operation& op) noexcept { op.Start(); }

        private:
            void Start() noexcept {
                if (context.IsRunningOnIOThread()) {
                    StartLocal();
                } else {
                    StartRemote();
                }
            }

            void StartLocal() noexcept {
                if (stdexec::get_stop_token(receiver_).stop_requested()) {
                    execute = &ExecuteAlreadyCanceled;
                    context.ScheduleLocal(this);
                } else {
                    execute = &ExecuteMayComplete;
                    context.ScheduleAt(this);
                }
            }

            void StartRemote() noexcept {
                execute = &OnStartRemoteScheduled;
                context.ScheduleRemote(this);
            }

            static void OnStartRemoteScheduled(OperationBase* op) noexcept {
                static_cast<Operation*>(op)->StartLocal();
            }

            static void ExecuteAlreadyCanceled(OperationBase* op) noexcept {
                auto self = static_cast<Operation*>(op);
                stdexec::set_stopped(std::move(self->receiver_));
            }

            static void ExecuteMayComplete(OperationBase* op) noexcept {
                auto self = static_cast<Operation*>(op);
                if (stdexec::get_stop_token(self->receiver_).stop_requested()) {
                    ExecuteAlreadyCanceled(op);
                    return;
                }
                try {
                    stdexec::set_value(std::move(self->receiver_));
                } catch (...) {
                    stdexec::set_error(std::move(self->receiver_), std::current_exception());
                }
            }

            void RequestStop() noexcept {
                if (context.IsRunningOnIOThread()) {
                    RequestStopLocal();
                } else {
                    RequestStopRemote();
                }
            }

            void RequestStopLocal() noexcept {
                execute = &ExecuteAlreadyCanceled;
                auto state = this->state.load(std::memory_order_relaxed);  // no race
                if ((state & OperationFlags::Expired) == 0) {
                    // timer not expired yet
                    context.RemoveTimer(this);
                    context.ScheduleLocal(this);
                } else {
                    // timer already expired thus cannot be canceled
                }
            }

            void RequestStopRemote() noexcept {
                auto old_state = state.fetch_or(OperationFlags::Cancelled,
                                                std::memory_order_acq_rel);  // maybe race
                if ((old_state & OperationFlags::Expired) == 0) {
                    // timer not expired yet
                    execute = &OnRequestStopRemoteScheduled;
                    context.ScheduleRemote(this);
                } else {
                    // timer already expired thus cannot be canceled
                }
            }

            static void OnRequestStopRemoteScheduled(OperationBase* op) noexcept {
                static_cast<Operation*>(op)->RequestStopLocal();
            }

            struct StopCallback {
                Operation& operation;
                void operator()() noexcept { operation.RequestStop(); }
            };

            Receiver receiver_;
            stdexec::in_place_stop_callback<StopCallback> stop_callback_;
        };

    public:
        using is_sender = void;
        using completion_sigs =
            stdexec::completion_signatures<stdexec::set_value_t(),
                                           stdexec::set_error_t(std::exception_ptr),
                                           stdexec::set_stopped_t()>;

        template <typename Env>
        friend completion_sigs tag_invoke(stdexec::get_completion_signatures_t,
                                          const ScheduleAtSender&, Env) noexcept {
            return {};
        }

        friend ScheduleEnv tag_invoke(stdexec::get_env_t, const ScheduleAtSender& sender) noexcept {
            return sender.env_;
        }

        template <stdexec::receiver_of<completion_sigs> Receiver>
        friend Operation<std::remove_cvref_t<Receiver>> tag_invoke(stdexec::connect_t,
                                                                   const ScheduleAtSender& sender,
                                                                   Receiver&& receiver) noexcept {
            return {*sender.env_.context, sender.expiration_, std::forward<Receiver>(receiver)};
        }

    private:
        friend class Scheduler;
        ScheduleAtSender(ScheduleEnv env, TimePoint expiration) noexcept
            : env_(env), expiration_(expiration) {}

        ScheduleEnv env_;
        TimePoint expiration_;
    };

public:
    explicit Scheduler(EpollContext* context) noexcept : context_(context) {}

    friend ScheduleSender tag_invoke(stdexec::schedule_t, const Scheduler& scheduler) noexcept {
        return scheduler.Schedule();
    }

    friend ScheduleAtSender tag_invoke(exec::schedule_at_t, const Scheduler& scheduler,
                                       TimePoint expiration) noexcept {
        return scheduler.ScheduleAt(expiration);
    }

    friend ScheduleAtSender tag_invoke(exec::schedule_after_t, const Scheduler& scheduler,
                                       TimePoint::duration duration) noexcept {
        return scheduler.ScheduleAfter(duration);
    }

    friend TimePoint tag_invoke(exec::now_t, const Scheduler& scheduler) noexcept {
        return Scheduler::Now();
    }

    friend bool operator==(const Scheduler& a, const Scheduler& b) noexcept {
        return a.context_ == b.context_;
    }

private:
    ScheduleSender Schedule() const noexcept { return ScheduleSender{ScheduleEnv{context_}}; }

    ScheduleAtSender ScheduleAt(TimePoint expiration) const noexcept {
        return ScheduleAtSender{ScheduleEnv{context_}, expiration};
    }

    ScheduleAtSender ScheduleAfter(TimePoint::duration duration) const noexcept {
        return ScheduleAtSender{ScheduleEnv{context_}, Now() + duration};
    }

    static TimePoint Now() noexcept { return TimePoint::clock::now(); }

    EpollContext* context_;
};

inline EpollContext::Scheduler EpollContext::GetScheduler() noexcept { return Scheduler{this}; }

}  // namespace fuchsia
