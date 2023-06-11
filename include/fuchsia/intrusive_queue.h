//
// Created by wenjuxu on 2023/5/14.
//

#pragma once

#include <cassert>
#include <type_traits>
#include <utility>

namespace fuchsia {

template <typename T>
concept IntrusiveQueueNode = requires(T t) {
    { t.next } -> std::same_as<T*&>;
};

template <IntrusiveQueueNode T>
class IntrusiveQueue {
public:
    IntrusiveQueue() = default;

    IntrusiveQueue(IntrusiveQueue&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)), tail_(std::exchange(other.tail_, nullptr)) {}

    IntrusiveQueue& operator=(IntrusiveQueue&& other) noexcept {
        head_ = std::exchange(other.head_, nullptr);
        tail_ = std::exchange(other.tail_, nullptr);
        return *this;
    }

    ~IntrusiveQueue() { assert(Empty()); }

    bool Empty() const noexcept { return head_ == nullptr; }

    void PushBack(T* item) noexcept {
        item->next = nullptr;
        if (tail_ == nullptr) {
            head_ = item;
        } else {
            tail_->next = item;
        }
        tail_ = item;
    }

    T* PopFront() noexcept {
        assert(!Empty());
        T* item = head_;
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        return item;
    }

    static IntrusiveQueue MakeReversed(T* list) noexcept {
        T* new_head = nullptr;
        T* new_tail = list;
        while (list != nullptr) {
            T* next = list->next;
            list->next = new_head;
            new_head = list;
            list = next;
        }

        IntrusiveQueue result;
        result.head_ = new_head;
        result.tail_ = new_tail;
        return result;
    }

private:
    T* head_ = nullptr;
    T* tail_ = nullptr;
};

}  // namespace fuchsia
