#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedAllpassPhaser
 * Multi-stage analog phaser processor in Q15:
 * - 4-stage cascaded 1st-order allpass filter ladder
 * - Sinusoidal LFO sweep of allpass pole frequency
 * - Resonant feedback for deep comb sweeping notches
 * - Zero heap, zero FPU, ultra-fast.
 */
class FixedAllpassPhaser {
public:
    constexpr FixedAllpassPhaser() noexcept {
        init_sin_lut();
    }

    /**
     * configure
     * rate_inc: LFO 32-bit phase increment per sample
     * min_pole: minimum allpass pole coefficient in Q15 [1000, 20000]
     * max_pole: maximum allpass pole coefficient in Q15 [min_pole, 31000]
     * feedback: resonance feedback in Q15 [-28000, 28000]
     * mix: wet mix in Q15 [0, 32767]
     */
    void configure(uint32_t rate_inc, FixedQ15 min_pole, FixedQ15 max_pole, FixedQ15 feedback, FixedQ15 mix) noexcept {
        rate_inc_ = rate_inc;
        min_pole_ = std::clamp<int16_t>(min_pole.raw, 1000, 25000);
        max_pole_ = std::clamp<int16_t>(max_pole.raw, min_pole_, 31000);
        feedback_ = std::clamp<int16_t>(feedback.raw, -28000, 28000);
        mix_ = std::clamp<int16_t>(mix.raw, 0, 32767);
    }

    void reset() noexcept {
        ap_states_.fill(0);
        lfo_phase_ = 0;
        last_out_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        // Step LFO: sine in range [0, 32767]
        lfo_phase_ += rate_inc_;
        uint32_t idx = (lfo_phase_ >> 24) & 0xFF;
        int32_t sin_val = (static_cast<int32_t>(sin_lut_[idx]) + 32768) >> 1; // 0 to 32767

        // Interpolate pole coefficient: coeff = min + sin_val * (max - min)
        int32_t range = max_pole_ - min_pole_;
        int32_t coeff = min_pole_ + ((range * sin_val) >> 15);
        coeff = std::clamp<int32_t>(coeff, 1000, 31000);

        // Feedback from output
        int32_t fb_val = (last_out_ * feedback_) >> 15;
        int32_t stage_in = static_cast<int32_t>(input.raw) + fb_val;
        stage_in = std::clamp<int32_t>(stage_in, -32768, 32767);

        // 4 cascaded 1st-order allpass filters:
        // y[n] = -coeff * x[n] + x[n-1] + coeff * y[n-1]
        // or Direct Form 1: y = state - coeff * (x - y)
        // Canonical: y = -c * in + state; state = in + c * y
        int32_t current = stage_in;
        for (size_t i = 0; i < 4; ++i) {
            int32_t c_in = (current * coeff) >> 15;
            int32_t y = ap_states_[i] - c_in;
            y = std::clamp<int32_t>(y, -32768, 32767);

            int32_t c_y = (y * coeff) >> 15;
            ap_states_[i] = std::clamp<int32_t>(current + c_y, -32768, 32767);
            current = y;
        }

        last_out_ = current;

        // Mix dry / wet
        int32_t dry_mul = 32767 - mix_;
        int32_t wet_mul = mix_;
        int32_t out = ((static_cast<int32_t>(input.raw) * dry_mul) >> 15) + ((current * wet_mul) >> 15);
        out = std::clamp<int32_t>(out, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
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

    uint32_t lfo_phase_{0};
    uint32_t rate_inc_{1000000};
    int16_t min_pole_{4000};
    int16_t max_pole_{24000};
    int16_t feedback_{16384};
    int16_t mix_{16384}; // 50%
    int32_t last_out_{0};
    std::array<int32_t, 4> ap_states_{};
    std::array<int16_t, 256> sin_lut_{};
};

} // namespace aether::math
