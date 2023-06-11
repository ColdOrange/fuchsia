//
// Created by wenjuxu on 2023/5/14.
//

#pragma once

#include <cassert>
#include <type_traits>
#include <utility>

namespace fuchsia {

template <typename T>
concept IntrusivePriorityQueueNode = requires(T t, T u) {
    { t.prev } -> std::same_as<T*&>;
    { t.next } -> std::same_as<T*&>;
    { t < u } -> std::same_as<bool>;
};

template <IntrusivePriorityQueueNode T>
class IntrusivePriorityQueue {
public:
    IntrusivePriorityQueue() = default;

    IntrusivePriorityQueue(IntrusivePriorityQueue&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)) {}

    IntrusivePriorityQueue& operator=(IntrusivePriorityQueue&& other) noexcept {
        head_ = std::exchange(other.head_, nullptr);
        return *this;
    }

    ~IntrusivePriorityQueue() { assert(Empty()); }

    bool Empty() const noexcept { return head_ == nullptr; }

    void Push(T* item) noexcept {
        // TODO: optimize
        T* prev = nullptr;
        T* next = head_;
        while (next != nullptr && !(*item < *next)) {  // use `<=` to achieve FIFO when equal
            prev = next;
            next = next->next;
        }
        item->prev = prev;
        item->next = next;
        if (prev == nullptr) {
            head_ = item;
        } else {
            prev->next = item;
        }
        if (next != nullptr) {
            next->prev = item;
        }
    }

    T* Top() const noexcept {
        assert(!Empty());
        return head_;
    }

    T* Pop() noexcept {
        assert(!Empty());
        T* item = head_;
        head_ = item->next;
        if (head_ != nullptr) {
            head_->prev = nullptr;
        }
        return item;
    }

    void Remove(T* item) noexcept {
        if (item->prev == nullptr) {
            head_ = item->next;
        } else {
            item->prev->next = item->next;
        }
        if (item->next != nullptr) {
            item->next->prev = item->prev;
        }
    }

private:
    T* head_ = nullptr;
};

}  // namespace fuchsia
