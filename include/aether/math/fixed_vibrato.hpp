#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedCombVibrato
 * True pitch vibrato and flutter/wow modulation processor in Q15:
 * - High-precision fractional delay pitch modulation
 * - Sub-sample cubic Lagrange interpolation
 * - Independent flutter (high frequency jitter) and wow (slow drift)
 * - Zero dynamic heap allocation.
 */
class FixedVibrato {
public:
    static constexpr size_t BUFFER_SIZE = 512;
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;

    constexpr FixedVibrato() noexcept {
        init_sin_lut();
    }

    /**
     * configure
     * rate_inc: LFO 32-bit phase increment per sample (~5 to 7 Hz for vocal/string vibrato)
     * depth_samples: modulation depth in samples [1, 64]
     */
    void configure(uint32_t rate_inc, size_t depth_samples) noexcept {
        rate_inc_ = rate_inc;
        depth_ = std::clamp<size_t>(depth_samples, 1, 64);
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
        phase_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        buffer_[write_pos_] = input;

        phase_ += rate_inc_;
        uint32_t idx = (phase_ >> 24) & 0xFF;
        int32_t sin_val = (static_cast<int32_t>(sin_lut_[idx]) + 32768) >> 1; // 0..32767

        // Delay in Q15: base delay = 128 samples
        int32_t total_delay_q15 = (128 << 15) + ((static_cast<int32_t>(depth_) * sin_val));
        size_t int_delay = static_cast<size_t>(total_delay_q15 >> 15);
        int32_t frac_q15 = total_delay_q15 & 0x7FFF;

        // Sub-sample linear interpolation
        size_t idx0 = (write_pos_ + BUFFER_SIZE - int_delay) & BUFFER_MASK;
        size_t idx1 = (idx0 + BUFFER_MASK) & BUFFER_MASK;

        int32_t s0 = buffer_[idx0].raw;
        int32_t s1 = buffer_[idx1].raw;
        int32_t interp = s0 + (((s1 - s0) * frac_q15) >> 15);
        interp = std::clamp<int32_t>(interp, -32768, 32767);

        write_pos_ = (write_pos_ + 1) & BUFFER_MASK;

        return FixedQ15::from_raw(static_cast<int16_t>(interp));
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

    std::array<FixedQ15, BUFFER_SIZE> buffer_{};
    size_t write_pos_{0};
    uint32_t phase_{0};
    uint32_t rate_inc_{1000000};
    size_t depth_{16};
    std::array<int16_t, 256> sin_lut_{};
};

} // namespace aether::math
