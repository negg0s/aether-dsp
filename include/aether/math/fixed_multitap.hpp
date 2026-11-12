#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedMultiTapDelay
 * 4-Tap rhythmic delay processor in Q15:
 * - 4 independent, tempo-synchronized or fractional taps
 * - Per-tap gain, panning, and feedback recirculation
 * - Buffer length: 4096 samples (approx 85.3 ms at 48 kHz, suitable for rhythmic slapback/diffuse reflections)
 * - Zero dynamic heap allocation.
 */
class FixedMultiTapDelay {
public:
    static constexpr size_t BUFFER_SIZE = 4096;
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;

    struct TapConfig {
        size_t delay_samples{256};
        int16_t gain_q15{16384}; // 0.5x
        int16_t pan_q15{0};      // -32768 (hard left) to +32767 (hard right)
    };

    constexpr FixedMultiTapDelay() noexcept {
        // Default tap times: 256, 512, 768, 1024 samples
        taps_[0] = { 256,  24000, -20000 };
        taps_[1] = { 512,  18000,  20000 };
        taps_[2] = { 768,  14000, -10000 };
        taps_[3] = { 1024, 10000,  10000 };
    }

    void set_tap(size_t index, size_t delay_samples, FixedQ15 gain, int16_t pan) noexcept {
        if (index < taps_.size()) {
            taps_[index].delay_samples = std::clamp<size_t>(delay_samples, 1, BUFFER_SIZE - 1);
            taps_[index].gain_q15 = gain.raw;
            taps_[index].pan_q15 = std::clamp<int16_t>(pan, -32768, 32767);
        }
    }

    void set_feedback(FixedQ15 fb) noexcept {
        feedback_ = std::clamp<int16_t>(fb.raw, 0, 31000);
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
    }

    struct StereoOutput {
        FixedQ15 left;
        FixedQ15 right;
    };

    [[nodiscard]] StereoOutput process(FixedQ15 input) noexcept {
        int32_t out_l = 0;
        int32_t out_r = 0;
        int32_t last_tap_val = 0;

        for (const auto& tap : taps_) {
            size_t read_pos = (write_pos_ + BUFFER_SIZE - tap.delay_samples) & BUFFER_MASK;
            int32_t s = buffer_[read_pos].raw;
            int32_t tapped = (s * tap.gain_q15) >> 15;

            // Pan calculation in Q15:
            // pan in [-32768, 32767] -> Left gain = (32767 - pan) / 2, Right gain = (32767 + pan) / 2
            int32_t pan_shifted = static_cast<int32_t>(tap.pan_q15);
            int32_t gain_l = (32767 - pan_shifted) >> 1;
            int32_t gain_r = (32767 + pan_shifted) >> 1;

            out_l += (tapped * gain_l) >> 15;
            out_r += (tapped * gain_r) >> 15;

            last_tap_val = tapped;
        }

        // Recirculate feedback from longest tap
        int32_t fb_contrib = (last_tap_val * feedback_) >> 15;
        int32_t to_write = static_cast<int32_t>(input.raw) + fb_contrib;
        to_write = std::clamp<int32_t>(to_write, -32768, 32767);

        buffer_[write_pos_] = FixedQ15::from_raw(static_cast<int16_t>(to_write));
        write_pos_ = (write_pos_ + 1) & BUFFER_MASK;

        out_l = std::clamp<int32_t>(out_l, -32768, 32767);
        out_r = std::clamp<int32_t>(out_r, -32768, 32767);

        return {FixedQ15::from_raw(static_cast<int16_t>(out_l)), FixedQ15::from_raw(static_cast<int16_t>(out_r))};
    }

private:
    std::array<FixedQ15, BUFFER_SIZE> buffer_{};
    std::array<TapConfig, 4> taps_{};
    size_t write_pos_{0};
    int16_t feedback_{8000}; // ~0.24
};

} // namespace aether::math
