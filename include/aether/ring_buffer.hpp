#pragma once

#include "types.hpp"
#include <atomic>
#include <array>
#include <new>

namespace aether {

constexpr size_t CACHE_LINE_SIZE = 64;

/**
 * @brief Lock-free, Wait-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * Cache-line padded to prevent false sharing between audio callback thread
 * and UI/control producer thread.
 */
template <typename T, size_t Capacity>
requires ((Capacity & (Capacity - 1)) == 0) // Capacity must be a power of 2
class SPSCRingBuffer {
public:
    static constexpr size_t MASK = Capacity - 1;

    constexpr SPSCRingBuffer() noexcept : head_(0), tail_(0) {}

    ~SPSCRingBuffer() noexcept = default;

    // Non-copyable, non-movable for thread-safety guarantees
    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;

    /**
     * @brief Push item to buffer. Only call from producer thread.
     */
    [[nodiscard]] bool push(const T& item) noexcept {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t current_tail = tail_.load(std::memory_order_acquire);

        if ((current_head - current_tail) >= Capacity) {
            return false; // Buffer full
        }

        buffer_[current_head & MASK] = item;
        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pop item from buffer. Only call from consumer thread.
     */
    [[nodiscard]] bool pop(T& item) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t current_head = head_.load(std::memory_order_acquire);

        if (current_tail == current_head) {
            return false; // Buffer empty
        }

        item = buffer_[current_tail & MASK];
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] size_t size() const noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_relaxed);
        return (head >= tail) ? (head - tail) : 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] constexpr size_t capacity() const noexcept {
        return Capacity;
    }

    void reset() noexcept {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

private:
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_{0};
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_{0};
    alignas(CACHE_LINE_SIZE) std::array<T, Capacity> buffer_{};
};

} // namespace aether
