//
// Created by wenjuxu on 2023/5/14.
//

#include "fuchsia/epoll_context.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include "fmt/std.h"
#include "fuchsia/logging.h"
#include "fuchsia/scope_guard.h"

namespace fuchsia {

static thread_local EpollContext* current_context = nullptr;

EpollContext::EpollContext() {
    epoll_fd_ = epoll_create(1);
    if (epoll_fd_ < 0) {
        int err = errno;
        LOG_ERROR("epoll_create failed: {}", strerror(err));
        throw std::system_error{err, std::system_category(), "epoll_create failed"};
    }

    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd_ < 0) {
        int err = errno;
        LOG_ERROR("timerfd_create failed: {}", strerror(err));
        throw std::system_error{err, std::system_category(), "timerfd_create failed"};
    }

    struct epoll_event event {};
    event.data.fd = timer_fd_;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &event) < 0) {
        int err = errno;
        LOG_ERROR("epoll add timer fd failed: {}", strerror(err));
        throw std::system_error{err, std::system_category(), "epoll add timer fd failed"};
    }

    wakeup_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_fd_ < 0) {
        int err = errno;
        LOG_ERROR("eventfd create failed: {}", strerror(err));
        throw std::system_error{err, std::system_category(), "eventfd create failed"};
    }

    event.data.fd = wakeup_fd_;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wakeup_fd_, &event) < 0) {
        int err = errno;
        LOG_ERROR("epoll add wakeup fd failed: {}", strerror(err));
        throw std::system_error{err, std::system_category(), "epoll add wakeup fd failed"};
    }

    LOG_TRACE("epoll context created");
}

EpollContext::~EpollContext() {
    if (epoll_fd_ > 0) close(epoll_fd_);
    if (timer_fd_ > 0) close(timer_fd_);
    if (wakeup_fd_ > 0) close(wakeup_fd_);
    LOG_TRACE("epoll context destroyed");
}

void EpollContext::Run() {
    LOG_TRACE("epoll context started running on thread: {}", std::this_thread::get_id());
    current_context = this;
    ScopeGuard _{[&]() noexcept { current_context = nullptr; }};

    while (true) {
        ProcessLocalOperations();

        bool has_remote_operations = ProcessRemoteOperations();
        if (has_remote_operations) {
            continue;
        }

        if (stop_source_.stop_requested()) {
            LOG_TRACE("epoll context stopped running");
            return;
        }

        BlockingWaitEvents();
    }
}

void EpollContext::Stop() noexcept {
    LOG_TRACE("epoll context stop requested");
    stop_source_.request_stop();
    Wakeup();
}

void EpollContext::Schedule(OperationBase* op) noexcept {
    assert(op->execute != nullptr);
    if (IsRunningOnIOThread()) {
        ScheduleLocal(op);
    } else {
        ScheduleRemote(op);
    }
}

void EpollContext::ScheduleLocal(OperationBase* op) noexcept {
    LOG_TRACE("schedule local operation: {}", op->uuid);
    local_operation_queue_.PushBack(op);
}

void EpollContext::ScheduleRemote(OperationBase* op) noexcept {
    LOG_TRACE("schedule remote operation: {} from thread: {}", op->uuid,
              std::this_thread::get_id());
    remote_operation_queue_.PushFront(op);
    Wakeup();
}

void EpollContext::ScheduleAt(EpollContext::TimerOperation* op) noexcept {
    LOG_TRACE("schedule timer operation: {}", op->uuid);
    assert(op->execute != nullptr);
    assert(IsRunningOnIOThread());
    timer_queue_.Push(op);
    if (timer_queue_.Top() == op) {
        UpdateNextExpirationTime();
    }
}

void EpollContext::RemoveTimer(EpollContext::TimerOperation* op) noexcept {
    LOG_TRACE("remove timer operation: {}", op->uuid);
    assert(IsRunningOnIOThread());
    bool need_update = timer_queue_.Top() == op;
    timer_queue_.Remove(op);
    if (need_update) {
        UpdateNextExpirationTime();
    }
}

void EpollContext::ProcessLocalOperations() noexcept {
    if (local_operation_queue_.Empty()) {
        LOG_TRACE("local operation queue is empty");
        return;
    }

    LOG_TRACE("processing local operations");

    size_t count = 0;
    auto pending_queue = std::move(local_operation_queue_);
    while (!pending_queue.Empty()) {
        auto op = pending_queue.PopFront();
        op->execute.load()(op);
        ++count;
    }

    LOG_TRACE("processed {} local operations", count);
}

bool EpollContext::ProcessRemoteOperations() noexcept {
    if (remote_operation_queue_.Empty()) {
        LOG_TRACE("remote operation queue is empty");
        return false;
    }

    LOG_TRACE("processing remote operations");

    size_t count = 0;
    auto pending_queue = remote_operation_queue_.PopAll();
    while (!pending_queue.Empty()) {
        auto op = pending_queue.PopFront();
        ScheduleLocal(op);
        ++count;
    }

    LOG_TRACE("processed {} remote operations", count);
    return true;
}

void EpollContext::ProcessTimers() noexcept {
    LOG_TRACE("processing timers");

    auto now = TimePoint::clock::now();
    while (!timer_queue_.Empty() && timer_queue_.Top()->expiration <= now) {
        auto op = static_cast<TimerOperation*>(timer_queue_.Pop());
        auto old_state = op->state.fetch_or(OperationFlags::Expired, std::memory_order_acq_rel);
        if (old_state & OperationFlags::Cancelled) {
            LOG_TRACE("timer operation {} already cancelled", op->uuid);
            continue;
        }

        LOG_TRACE("timer operation {} elapsed, schedule it now", op->uuid);
        ScheduleLocal(op);
    }

    next_expiration_time_.reset();
    UpdateNextExpirationTime();
}

void EpollContext::UpdateNextExpirationTime() noexcept {
    if (timer_queue_.Empty()) {
        if (next_expiration_time_) {
            LOG_TRACE("no more timer operations, cancelling timer");
            struct itimerspec spec {};  // zero value means disarm the timer
            timerfd_settime(timer_fd_, 0, &spec, nullptr);
            next_expiration_time_.reset();
        }
    } else {
        auto earliest_expiration = timer_queue_.Top()->expiration;
        auto duration = earliest_expiration - TimePoint::clock::now();
        LOG_TRACE("next timer operation will expire in {} ms",
                  std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());

        // tweak already elapsed timers for timerfd_settime
        if (duration < std::chrono::nanoseconds::zero()) {
            duration = std::chrono::nanoseconds{1};
        }

        static constexpr auto threshold = std::chrono::milliseconds{1};
        if (!next_expiration_time_ || earliest_expiration < *next_expiration_time_ - threshold) {
            struct itimerspec spec {};
            spec.it_value.tv_sec =
                std::chrono::duration_cast<std::chrono::seconds>(duration).count();
            spec.it_value.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        duration - std::chrono::seconds{spec.it_value.tv_sec})
                                        .count();
            timerfd_settime(timer_fd_, 0, &spec, nullptr);
            LOG_TRACE("update next expiration time to <{}s, {}ns>", spec.it_value.tv_sec,
                      spec.it_value.tv_nsec);
            next_expiration_time_ = earliest_expiration;
        }
    }
}

void EpollContext::BlockingWaitEvents() {
    LOG_TRACE("blocking wait events");

    static constexpr size_t kMaxEventsPerLoop = 128;
    struct epoll_event events[kMaxEventsPerLoop];
    int num_events =
        epoll_wait(epoll_fd_, events, kMaxEventsPerLoop, local_operation_queue_.Empty() ? -1 : 0);
    if (num_events < 0) {
        int err = errno;
        if (err != EINTR) {
            LOG_ERROR("epoll_wait failed: {}", strerror(err));
            throw std::system_error{err, std::system_category(), "epoll_wait failed"};
        }
    }

    LOG_TRACE("blocking wait finished, {} events received", num_events);

    for (int i = 0; i < num_events; ++i) {
        if (events[i].data.fd == timer_fd_) {
            LOG_TRACE("timer event received");
            Drain(timer_fd_);
            ProcessTimers();
        } else if (events[i].data.fd == wakeup_fd_) {
            LOG_TRACE("wakeup event received");
            Drain(wakeup_fd_);
            // Do nothing
        } else {
            auto op = reinterpret_cast<OperationBase*>(events[i].data.ptr);
            LOG_TRACE("event received for operation {}", op->uuid);
            ScheduleLocal(op);
        }
    }
}

void EpollContext::Wakeup() {
    uint64_t value = 1;
    ssize_t n = ::write(wakeup_fd_, &value, sizeof(value));
    if (n < 0) {
        int err = errno;
        LOG_ERROR("write wakeup fd failed: {}", strerror(err));

        // It's fundamentally broken anyway if we cannot wake up the loop,
        // and we also want to use noexcept on functions that call Wakeup().
        std::terminate();
    }
}

void EpollContext::Drain(int fd) {
    uint64_t value;
    ssize_t n = ::read(fd, &value, sizeof(value));
    if (n < 0) {
        int err = errno;
        LOG_ERROR("read fd {} failed: {}", fd, strerror(err));
        throw std::system_error{err, std::system_category(), "read fd failed"};
    }
}

bool EpollContext::IsRunningOnIOThread() const noexcept { return current_context == this; }

}  // namespace fuchsia
