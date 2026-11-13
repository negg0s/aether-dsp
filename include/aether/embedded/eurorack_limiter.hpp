#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Peak Limiter & Clipper
// Fast lookahead peak limiter with transparent brickwall ceiling clamp
// and adaptive release envelope to prevent digital DAC clipping in Eurorack modules.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackBrickwallLimiter {
public:
    static constexpr size_t LookaheadBuffer = 32;

    EurorackBrickwallLimiter() {
        reset();
    }

    void reset() noexcept {
        buffer_.fill(0);
        write_idx_ = 0;
        envelope_ = 32767; // 1.0 gain in Q15
        ceiling_ = 32000;  // -0.2 dBFS ceiling clamp
        release_rate_ = 8; // gentle gain recovery
    }

    void set_ceiling(int16_t ceiling_raw) noexcept {
        ceiling_ = (ceiling_raw < 1000) ? 1000 : ceiling_raw;
    }

    void set_release(uint16_t release_rate) noexcept {
        release_rate_ = (release_rate == 0) ? 1 : release_rate;
    }

    // Process a single audio sample through lookahead brickwall limiter
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        // Store incoming sample in circular delay line
        buffer_[write_idx_] = input.raw;
        size_t read_idx = (write_idx_ + 1) % LookaheadBuffer;
        int16_t delayed_sample = buffer_[read_idx];
        write_idx_ = (write_idx_ + 1) % LookaheadBuffer;

        // Peak detector on current incoming sample
        int32_t peak = (input.raw < 0) ? -input.raw : input.raw;

        // If prospective peak exceeds ceiling, compute required attenuation gain
        if (peak > ceiling_) {
            // target_gain = (ceiling << 15) / peak
            int32_t target_gain = (static_cast<int32_t>(ceiling_) << 15) / peak;
            if (target_gain < envelope_) {
                envelope_ = target_gain; // Instant fast attack
            }
        } else {
            // Adaptive release recovery towards unity gain (32767)
            if (envelope_ < 32767) {
                envelope_ += release_rate_;
                if (envelope_ > 32767) envelope_ = 32767;
            }
        }

        // Apply gain reduction envelope to delayed sample
        int64_t limited = (static_cast<int64_t>(delayed_sample) * envelope_) >> 15;

        // Final brickwall safety clamp
        if (limited > ceiling_) limited = ceiling_;
        if (limited < -ceiling_) limited = -ceiling_;

        return math::FixedQ15(static_cast<int16_t>(limited));
    }

    [[nodiscard]] math::FixedQ15 gain_reduction() const noexcept {
        return math::FixedQ15(static_cast<int16_t>(envelope_));
    }

private:
    std::array<int16_t, LookaheadBuffer> buffer_{};
    size_t write_idx_{0};
    int32_t envelope_{32767};
    int16_t ceiling_{32000};
    uint16_t release_rate_{8};
};

} // namespace aether::embedded
