//
// Created by wenjuxu on 2023/5/14.
//

#pragma once

#include <atomic>
#include <cassert>
#include <type_traits>
#include <utility>

#include "fuchsia/intrusive_queue.h"

namespace fuchsia {

template <IntrusiveQueueNode T>
class AtomicIntrusiveQueue {
public:
    AtomicIntrusiveQueue() = default;

    AtomicIntrusiveQueue(const AtomicIntrusiveQueue&) = delete;
    AtomicIntrusiveQueue& operator=(const AtomicIntrusiveQueue&) = delete;
    AtomicIntrusiveQueue(AtomicIntrusiveQueue&& other) = delete;
    AtomicIntrusiveQueue& operator=(AtomicIntrusiveQueue&& other) = delete;

    ~AtomicIntrusiveQueue() { assert(Empty()); }

    bool Empty() const noexcept { return head_.load(std::memory_order_relaxed) == nullptr; }

    void PushFront(T* item) noexcept {
        T* old_head = head_.load(std::memory_order_relaxed);
        do {
            item->next = old_head;
        } while (!head_.compare_exchange_weak(old_head, item, std::memory_order_acq_rel));
    }

    IntrusiveQueue<T> PopAll() noexcept {
        T* old_head = head_.exchange(nullptr, std::memory_order_acq_rel);
        return IntrusiveQueue<T>::MakeReversed(old_head);
    }

private:
    std::atomic<T*> head_ = nullptr;
};

}  // namespace fuchsia
