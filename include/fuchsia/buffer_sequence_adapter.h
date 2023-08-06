//
// Created by wenjuxu on 2023/8/6.
//

#pragma once

#include "fuchsia/buffer.h"
#include "sys/uio.h"

namespace fuchsia {

class BufferSequenceAdapterBase {
public:
    // The maximum number of buffers to support in a single operation.
    static constexpr int MaxBuffers = 64;

protected:
    using NativeBufferType = iovec;

    static constexpr void InitIovBase(void*& base, void* addr) { base = addr; }

    template <typename T>
    static constexpr void InitIovBase(T& base, void* addr) {
        base = static_cast<T>(addr);
    }

    static constexpr void InitNativeBuffer(iovec& iov, const MutableBuffer& buf) {
        InitIovBase(iov.iov_base, buf.Data());
        iov.iov_len = buf.Size();
    }

    static constexpr void InitNativeBuffer(iovec& iov, const ConstBuffer& buf) {
        InitIovBase(iov.iov_base, const_cast<void*>(buf.Data()));
        iov.iov_len = buf.Size();
    }
};

template <typename Buffer, typename BufferSequence>
class BufferSequenceAdapter : private BufferSequenceAdapterBase {
public:
    static constexpr bool IsSingleBuffer = false;

    explicit constexpr BufferSequenceAdapter(const BufferSequence& sequence) noexcept
        : count_(0), total_buffer_size_(0) {
        BufferSequenceAdapter::Init(BufferSequenceBegin(sequence), BufferSequenceEnd(sequence));
    }

    constexpr NativeBufferType* Buffers() noexcept { return buffers_; }

    constexpr std::size_t Count() const noexcept { return count_; }

    constexpr std::size_t TotalSize() const noexcept { return total_buffer_size_; }

    constexpr bool AllEmpty() const noexcept { return total_buffer_size_ == 0; }

    static constexpr bool AllEmpty(const BufferSequence& sequence) noexcept {
        return BufferSequenceAdapter::AllEmpty(BufferSequenceBegin(sequence),
                                               BufferSequenceEnd(sequence));
    }

    static constexpr Buffer First(const BufferSequence& sequence) noexcept {
        return BufferSequenceAdapter::First(BufferSequenceBegin(sequence),
                                            BufferSequenceEnd(sequence));
    }

private:
    template <typename Iterator>
    void Init(Iterator begin, Iterator end) {
        for (Iterator iter = begin; iter != end && count_ < MaxBuffers; ++iter, ++count_) {
            Buffer buffer{*iter};
            InitNativeBuffer(buffers_[count_], buffer);
            total_buffer_size_ += buffer.Size();
        }
    }

    template <typename Iterator>
    static bool AllEmpty(Iterator begin, Iterator end) {
        std::size_t i = 0;
        for (Iterator iter = begin; iter != end && i < MaxBuffers; ++iter, ++i) {
            if (Buffer(*iter).Size() > 0) {
                return false;
            }
        }
        return true;
    }

    template <typename Iterator>
    static Buffer First(Iterator begin, Iterator end) {
        for (Iterator iter = begin; iter != end; ++iter) {
            Buffer buffer{*iter};
            if (buffer.Size() != 0) {
                return buffer;
            }
        }
        return Buffer{};
    }

    NativeBufferType buffers_[MaxBuffers];
    std::size_t count_;
    std::size_t total_buffer_size_;
};

template <typename Buffer>
class BufferSequenceAdapter<Buffer, MutableBuffer> : BufferSequenceAdapterBase {
public:
    static constexpr bool IsSingleBuffer = true;

    explicit constexpr BufferSequenceAdapter(const MutableBuffer& sequence) {
        InitNativeBuffer(buffer_, Buffer(sequence));
        total_buffer_size_ = sequence.Size();
    }

    constexpr NativeBufferType* Buffers() noexcept { return &buffer_; }

    constexpr std::size_t Count() const noexcept { return 1; }

    constexpr std::size_t TotalSize() const noexcept { return total_buffer_size_; }

    constexpr bool AllEmpty() const noexcept { return total_buffer_size_ == 0; }

    static constexpr bool AllEmpty(const MutableBuffer& sequence) noexcept {
        return sequence.Size() == 0;
    }

    static constexpr Buffer First(const MutableBuffer& sequence) noexcept {
        return Buffer(sequence);
    }

private:
    NativeBufferType buffer_;
    std::size_t total_buffer_size_;
};

template <typename Buffer>
class BufferSequenceAdapter<Buffer, ConstBuffer> : BufferSequenceAdapterBase {
public:
    static constexpr bool IsSingleBuffer = true;

    explicit constexpr BufferSequenceAdapter(const ConstBuffer& sequence) {
        InitNativeBuffer(buffer_, Buffer(sequence));
        total_buffer_size_ = sequence.Size();
    }

    constexpr NativeBufferType* Buffers() noexcept { return &buffer_; }

    constexpr std::size_t Count() const noexcept { return 1; }

    constexpr std::size_t TotalSize() const noexcept { return total_buffer_size_; }

    constexpr bool AllEmpty() const noexcept { return total_buffer_size_ == 0; }

    static constexpr bool AllEmpty(const ConstBuffer& sequence) noexcept {
        return sequence.Size() == 0;
    }

    static constexpr Buffer First(const ConstBuffer& sequence) noexcept { return Buffer(sequence); }

private:
    NativeBufferType buffer_;
    std::size_t total_buffer_size_;
};

}  // namespace fuchsia
