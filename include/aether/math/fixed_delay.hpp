#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedDelay
 * Fixed-point audio delay line in Q15 with feedback and cross-feed.
 * Length: 2048 samples (approx 42.6 ms at 48 kHz, suitable for flanger, chorus, slapback).
 * Zero heap allocation, static ring buffer.
 */
class FixedDelay {
public:
    static constexpr size_t MAX_DELAY = 2048;
    static constexpr size_t DELAY_MASK = MAX_DELAY - 1;

    constexpr FixedDelay() noexcept = default;

    void configure(size_t delay_samples, FixedQ15 feedback, FixedQ15 mix) noexcept {
        delay_len_ = std::clamp<size_t>(delay_samples, 1, MAX_DELAY - 1);
        feedback_ = feedback;
        mix_ = mix;
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        size_t read_pos = (write_pos_ + MAX_DELAY - delay_len_) & DELAY_MASK;
        FixedQ15 delayed = buffer_[read_pos];

        // Recirculating feedback: fb = delayed * feedback
        int32_t fb_raw = (static_cast<int32_t>(delayed.raw) * feedback_.raw) >> 15;
        int32_t to_write = static_cast<int32_t>(input.raw) + fb_raw;
        to_write = std::clamp<int32_t>(to_write, -32768, 32767);
        buffer_[write_pos_] = FixedQ15::from_raw(static_cast<int16_t>(to_write));

        write_pos_ = (write_pos_ + 1) & DELAY_MASK;

        // Mix: out = dry * (1 - mix) + wet * mix
        int32_t dry_contrib = (static_cast<int32_t>(input.raw) * (32767 - mix_.raw)) >> 15;
        int32_t wet_contrib = (static_cast<int32_t>(delayed.raw) * mix_.raw) >> 15;
        int32_t mixed = dry_contrib + wet_contrib;
        mixed = std::clamp<int32_t>(mixed, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(mixed));
    }

private:
    std::array<FixedQ15, MAX_DELAY> buffer_{};
    size_t write_pos_{0};
    size_t delay_len_{256};
    FixedQ15 feedback_{FixedQ15::from_raw(0)};
    FixedQ15 mix_{FixedQ15::from_raw(16384)}; // 50% wet
};

} // namespace aether::math
