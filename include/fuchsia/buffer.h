//
// Created by wenjuxu on 2023/6/23.
//

#pragma once

#include <cstddef>
#include <memory>

// Modified from asio/buffer.hpp

namespace fuchsia {

class MutableBuffer {
public:
    constexpr MutableBuffer() noexcept = default;

    constexpr MutableBuffer(void* data, size_t size) noexcept : data_(data), size_(size) {}

    constexpr void* Data() const noexcept { return data_; }

    constexpr size_t Size() const noexcept { return size_; }

    constexpr MutableBuffer& operator+=(size_t n) noexcept {
        size_t offset = n < size_ ? n : size_;
        data_ = static_cast<char*>(data_) + offset;
        size_ -= offset;
        return *this;
    }

private:
    void* data_ = nullptr;
    size_t size_ = 0;
};

class ConstBuffer {
public:
    constexpr ConstBuffer() noexcept = default;

    constexpr ConstBuffer(const void* data, size_t size) noexcept : data_(data), size_(size) {}

    constexpr ConstBuffer(const MutableBuffer& b) noexcept : data_(b.Data()), size_(b.Size()) {}

    constexpr const void* Data() const noexcept { return data_; }

    constexpr size_t Size() const noexcept { return size_; }

    constexpr ConstBuffer& operator+=(size_t n) noexcept {
        size_t offset = n < size_ ? n : size_;
        data_ = static_cast<const char*>(data_) + offset;
        size_ -= offset;
        return *this;
    }

private:
    const void* data_ = nullptr;
    size_t size_ = 0;
};

template <typename T>
requires std::convertible_to<const T*, const MutableBuffer*>
inline constexpr const MutableBuffer* BufferSequenceBegin(const T& b) noexcept {
    return static_cast<const MutableBuffer*>(std::addressof(b));
}

template <typename T>
requires std::convertible_to<const T*, const ConstBuffer*>
inline constexpr const ConstBuffer* BufferSequenceBegin(const T& b) noexcept {
    return static_cast<const ConstBuffer*>(std::addressof(b));
}

template <typename T>
requires std::convertible_to<const T*, const MutableBuffer*>
inline constexpr const MutableBuffer* BufferSequenceEnd(const T& b) noexcept {
    return static_cast<const MutableBuffer*>(std::addressof(b)) + 1;
}

template <typename T>
requires std::convertible_to<const T*, const ConstBuffer*>
inline constexpr const ConstBuffer* BufferSequenceEnd(const T& b) noexcept {
    return static_cast<const ConstBuffer*>(std::addressof(b)) + 1;
}

// clang-format off
template <typename C>
requires (!std::convertible_to<const C*, const MutableBuffer*> &&
          !std::convertible_to<const C*, const ConstBuffer*>)
inline constexpr auto BufferSequenceBegin(C& c) noexcept -> decltype(c.begin()) {
    return c.begin();
}

template <typename C>
requires (!std::convertible_to<const C*, const MutableBuffer*> &&
          !std::convertible_to<const C*, const ConstBuffer*>)
inline constexpr auto BufferSequenceBegin(const C& c) noexcept -> decltype(c.begin()) {
    return c.begin();
}

template <typename C>
requires (!std::convertible_to<const C*, const MutableBuffer*> &&
          !std::convertible_to<const C*, const ConstBuffer*>)
inline constexpr auto BufferSequenceEnd(C& c) noexcept -> decltype(c.end()) {
    return c.end();
}

template <typename C>
requires (!std::convertible_to<const C*, const MutableBuffer*> &&
          !std::convertible_to<const C*, const ConstBuffer*>)
inline constexpr auto BufferSequenceEnd(const C& c) noexcept -> decltype(c.end()) {
    return c.end();
}
// clang-format on

template <typename BufferSequence>
inline constexpr size_t BufferSize(const BufferSequence& b) noexcept {
    if constexpr (std::same_as<BufferSequence, ConstBuffer> ||
                  std::same_as<BufferSequence, MutableBuffer>) {
        return b.Size();
    } else {
        size_t size = 0;
        for (auto it = BufferSequenceBegin(b); it != BufferSequenceEnd(b); ++it) {
            size += it->Size();
        }
        return size;
    }
}

inline constexpr MutableBuffer operator+(const MutableBuffer& b, size_t n) noexcept {
    size_t offset = n < b.Size() ? n : b.Size();
    char* new_data = static_cast<char*>(b.Data()) + offset;
    size_t new_size = b.Size() - offset;
    return MutableBuffer{new_data, new_size};
}

inline constexpr ConstBuffer operator+(const ConstBuffer& b, size_t n) noexcept {
    size_t offset = n < b.Size() ? n : b.Size();
    const char* new_data = static_cast<const char*>(b.Data()) + offset;
    size_t new_size = b.Size() - offset;
    return ConstBuffer{new_data, new_size};
}

inline constexpr MutableBuffer Buffer(const MutableBuffer& b) noexcept { return b; }

inline constexpr MutableBuffer Buffer(const MutableBuffer& b, size_t max_size) noexcept {
    return MutableBuffer{b.Data(), b.Size() < max_size ? b.Size() : max_size};
}

inline constexpr ConstBuffer Buffer(const ConstBuffer& b) noexcept { return b; }

inline constexpr ConstBuffer Buffer(const ConstBuffer& b, size_t max_size) noexcept {
    return ConstBuffer{b.Data(), b.Size() < max_size ? b.Size() : max_size};
}

inline constexpr MutableBuffer Buffer(void* data, size_t size_in_bytes) noexcept {
    return MutableBuffer{data, size_in_bytes};
}

inline constexpr ConstBuffer Buffer(const void* data, size_t size_in_bytes) noexcept {
    return ConstBuffer{data, size_in_bytes};
}

template <typename T, size_t N>
inline constexpr MutableBuffer Buffer(T (&data)[N]) noexcept {
    return MutableBuffer{data, N * sizeof(T)};
}

template <typename T, size_t N>
inline constexpr ConstBuffer Buffer(const T (&data)[N]) noexcept {
    return ConstBuffer{data, N * sizeof(T)};
}

template <typename T, size_t N>
inline constexpr MutableBuffer Buffer(std::array<T, N>& data) noexcept {
    return MutableBuffer{data.data(), N * sizeof(T)};
}

template <typename T, size_t N>
inline constexpr ConstBuffer Buffer(std::array<const T, N>& data) noexcept {
    return ConstBuffer{data.data(), N * sizeof(T)};
}

template <typename T, size_t N>
inline constexpr ConstBuffer Buffer(const std::array<T, N>& data) noexcept {
    return ConstBuffer{data.data(), N * sizeof(T)};
}

template <typename T, typename Allocator>
inline MutableBuffer Buffer(std::vector<T, Allocator>& data) noexcept {
    return MutableBuffer{data.data(), data.size() * sizeof(T)};
}

template <typename T, typename Allocator>
inline ConstBuffer Buffer(const std::vector<T, Allocator>& data) noexcept {
    return ConstBuffer{data.data(), data.size() * sizeof(T)};
}

template <typename T, typename Traits, typename Allocator>
inline MutableBuffer Buffer(std::basic_string<T, Traits, Allocator>& data) noexcept {
    return MutableBuffer{data.data(), data.size() * sizeof(T)};
}

template <typename T, typename Traits, typename Allocator>
inline ConstBuffer Buffer(const std::basic_string<T, Traits, Allocator>& data) noexcept {
    return ConstBuffer{data.data(), data.size() * sizeof(T)};
}

template <typename T, typename Traits>
inline MutableBuffer Buffer(std::basic_string_view<T, Traits>& data) noexcept {
    return MutableBuffer{data.data(), data.size() * sizeof(T)};
}

template <typename T, typename Traits>
inline ConstBuffer Buffer(const std::basic_string_view<T, Traits>& data) noexcept {
    return ConstBuffer{data.data(), data.size() * sizeof(T)};
}

// clang-format off
template <typename T>
concept MutableContiguousContainer =
    std::contiguous_iterator<typename T::iterator> &&
    !std::convertible_to<T, MutableBuffer> &&
    !std::convertible_to<T, ConstBuffer> &&
    !std::is_const<typename std::remove_reference<
        typename std::iterator_traits<typename T::iterator>::reference>::type>::value;

template <typename T>
concept ConstContiguousContainer =
    std::contiguous_iterator<typename T::const_iterator> &&
    !std::convertible_to<T, MutableBuffer> &&
    !std::convertible_to<T, ConstBuffer> &&
    std::is_const<typename std::remove_reference<
        typename std::iterator_traits<typename T::const_iterator>::reference>::type>::value;
// clang-format on

template <MutableContiguousContainer C>
inline MutableBuffer Buffer(C& c) noexcept {
    return MutableBuffer{c.data(), c.size() * sizeof(typename C::value_type)};
}

template <ConstContiguousContainer C>
inline ConstBuffer Buffer(const C& c) noexcept {
    return ConstBuffer{c.data(), c.size() * sizeof(typename C::value_type)};
}

template <typename T>
concept MutableBufferSequence = requires(const T& t) {
    BufferSequenceBegin(t);
    BufferSequenceEnd(t);
    requires std::convertible_to<decltype(*BufferSequenceBegin(t)), MutableBuffer>;
};

template <typename T>
concept ConstBufferSequence = requires(const T& t) {
    BufferSequenceBegin(t);
    BufferSequenceEnd(t);
    requires std::convertible_to<decltype(*gBufferSequenceBegin(t)), ConstBuffer>;
};

}  // namespace fuchsia
