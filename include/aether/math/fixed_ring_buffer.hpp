#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>
#include <array>

namespace aether::math {

// Lock-free single-producer single-consumer (SPSC) ring buffer
// Designed for interrupt-service-routine (ISR) to audio callback streaming on bare-metal DSPs.
// Capacity must be a power of two.
template <typename T, size_t Capacity>
class FixedSpscQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert(Capacity >= 2, "Capacity must be at least 2");

public:
    FixedSpscQueue() : write_idx_(0), read_idx_(0) {}

    bool push(const T& item) noexcept {
        const size_t current_write = write_idx_.load(std::memory_order_relaxed);
        const size_t current_read = read_idx_.load(std::memory_order_acquire);

        if ((current_write - current_read) >= Capacity) {
            return false; // Queue is full
        }

        buffer_[current_write & BufferMask] = item;
        write_idx_.store(current_write + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) noexcept {
        const size_t current_read = read_idx_.load(std::memory_order_relaxed);
        const size_t current_write = write_idx_.load(std::memory_order_acquire);

        if (current_read == current_write) {
            return false; // Queue is empty
        }

        item = buffer_[current_read & BufferMask];
        read_idx_.store(current_read + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] size_t size() const noexcept {
        const size_t current_read = read_idx_.load(std::memory_order_relaxed);
        const size_t current_write = write_idx_.load(std::memory_order_relaxed);
        return (current_write >= current_read) ? (current_write - current_read) : 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        return read_idx_.load(std::memory_order_relaxed) == write_idx_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool full() const noexcept {
        return size() >= Capacity;
    }

    void reset() noexcept {
        read_idx_.store(0, std::memory_order_relaxed);
        write_idx_.store(0, std::memory_order_relaxed);
    }

    static constexpr size_t capacity() noexcept {
        return Capacity;
    }

private:
    static constexpr size_t BufferMask = Capacity - 1;
    std::array<T, Capacity> buffer_{};
    alignas(64) std::atomic<size_t> write_idx_;
    alignas(64) std::atomic<size_t> read_idx_;
};

} // namespace aether::math
