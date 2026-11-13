#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedCombAllpassPhasor
 * Dual-stage Barber-Pole Shepard-Risset infinitely rising/falling comb phaser in Q15:
 * - 4 rotating fractional delay taps spaced at 90-degree phase intervals.
 * - Gaussian / cosine bell-curve amplitude crossfading to ensure smooth infinite sweeping.
 * - Zero FPU, zero heap allocation.
 */
class FixedCombAllpassPhasor {
public:
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;
    static constexpr size_t NUM_TAPS = 4;

    constexpr FixedCombAllpassPhasor() noexcept {
        init_sin_lut();
    }

    /**
     * configure
     * rate_inc: signed 32-bit phase increment (positive = infinitely rising, negative = infinitely falling)
     * feedback: Q15 resonance intensity [0, 30000]
     * mix: wet level in Q15 [0, 32767]
     */
    void configure(int32_t rate_inc, FixedQ15 feedback, FixedQ15 mix) noexcept {
        rate_inc_ = rate_inc;
        feedback_ = std::clamp<int16_t>(feedback.raw, 0, 30000);
        mix_ = mix;
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
        phase_ = 0;
        last_out_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        phase_ += rate_inc_;

        // Recirculate feedback
        int32_t fb_val = (last_out_ * feedback_) >> 15;
        int32_t to_write = static_cast<int32_t>(input.raw) + fb_val;
        to_write = std::clamp<int32_t>(to_write, -32768, 32767);
        buffer_[write_pos_] = FixedQ15::from_raw(static_cast<int16_t>(to_write));

        int32_t wet_accum = 0;
        int32_t total_weight = 0;

        // 4 Taps spaced by 90 degrees (0x40000000)
        for (size_t i = 0; i < NUM_TAPS; ++i) {
            uint32_t tap_phase = phase_ + static_cast<uint32_t>(i * 0x40000000u);

            // Delay sweep: range 64 to 800 samples
            // Normalized phase 0..32767
            int32_t norm_p = static_cast<int32_t>(tap_phase >> 17); // 15 bits
            size_t delay = 64 + ((norm_p * 736) >> 15);
            size_t read_pos = (write_pos_ + BUFFER_SIZE - delay) & BUFFER_MASK;
            int32_t s = buffer_[read_pos].raw;

            // Hanning / bell window for tap amplitude: w = 0.5 * (1 - cos(2*pi*phase))
            // cos lookup: (tap_phase >> 24) & 0xFF
            uint32_t cos_idx = ((tap_phase + 0x40000000u) >> 24) & 0xFF;
            int32_t cos_val = sin_lut_[cos_idx]; // -32768 to 32767
            int32_t weight = (32767 - cos_val) >> 1; // 0 to 32767

            wet_accum += (s * weight) >> 15;
            total_weight += weight;
        }

        write_pos_ = (write_pos_ + 1) & BUFFER_MASK;

        // Normalize wet output
        if (total_weight > 0) {
            wet_accum = (static_accum_div(wet_accum, total_weight) * 32767) >> 15;
        }
        wet_accum = std::clamp<int32_t>(wet_accum, -32768, 32767);
        last_out_ = wet_accum;

        // Dry / Wet mix
        int32_t dry_mul = 32767 - mix_.raw;
        int32_t out = ((static_cast<int32_t>(input.raw) * dry_mul) >> 15) + ((wet_accum * mix_.raw) >> 15);
        out = std::clamp<int32_t>(out, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    [[nodiscard]] static constexpr int32_t static_accum_div(int32_t val, int32_t weight) noexcept {
        return static_cast<int32_t>((static_cast<int64_t>(val) << 15) / (weight > 0 ? weight : 1));
    }

    constexpr void init_sin_lut() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            int32_t x = static_cast<int32_t>(i);
            int32_t q = 0;
            if (x < 64) {
                q = (x * 32767) / 64;
            } else if (x < 128) {
                q = ((128 - x) * 32767) / 64;
            } else if (x < 192) {
                q = -(((x - 128) * 32767) / 64);
            } else {
                q = -(((256 - x) * 32767) / 64);
            }
            sin_lut_[i] = static_cast<int16_t>(q);
        }
    }

    std::array<FixedQ15, BUFFER_SIZE> buffer_{};
    size_t write_pos_{0};
    uint32_t phase_{0};
    int32_t rate_inc_{1000000};
    int16_t feedback_{16000};
    FixedQ15 mix_{FixedQ15::from_raw(16384)}; // 50%
    int32_t last_out_{0};
    std::array<int16_t, 256> sin_lut_{};
};

} // namespace aether::math
