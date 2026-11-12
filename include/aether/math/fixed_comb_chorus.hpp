#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedCombChorus
 * 6-Stage Diffuse Resonant Matrix Chorus / Ensemble in Q15:
 * - Emulates Solina String Ensemble / Roland Dimension D bucket-brigade device (BBD) chorus.
 * - 6 prime-numbered delay lines (from 137 to 431 samples)
 * - Independent multi-phase LFO modulation matrix (60-degree increments)
 * - Deep lush spatial dispersion with zero phase cancellation
 * - Zero dynamic heap allocation.
 */
class FixedCombChorus {
public:
    static constexpr size_t NUM_STAGES = 6;
    static constexpr size_t MAX_BUF = 512;
    static constexpr size_t MASK = MAX_BUF - 1;

    constexpr FixedCombChorus() noexcept {
        init_sin_lut();
    }

    /**
     * configure
     * rate_inc: 32-bit phase increment per sample (~0.6 Hz ensemble sweep)
     * depth: modulation depth in samples [1, 32]
     * mix: wet mix in Q15 [0, 32767]
     */
    void configure(uint32_t rate_inc, size_t depth, FixedQ15 mix) noexcept {
        rate_inc_ = rate_inc;
        depth_ = std::clamp<size_t>(depth, 1, 32);
        mix_ = mix;
    }

    void reset() noexcept {
        for (auto& buf : buffers_) buf.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
        lfo_phase_ = 0;
    }

    struct StereoEnsemble {
        FixedQ15 left;
        FixedQ15 right;
    };

    [[nodiscard]] StereoEnsemble process(FixedQ15 in_l, FixedQ15 in_r) noexcept {
        // Prime base delays for Dimension D / Solina matrix
        static constexpr std::array<size_t, NUM_STAGES> BASE_DELAYS = { 137, 179, 233, 283, 349, 431 };
        // 60-degree phase offsets: (2^32 / 6) = 715827882
        static constexpr uint32_t PHASE_STEP = 715827882u;

        lfo_phase_ += rate_inc_;

        // Write inputs to delay lines (3 left, 3 right)
        int32_t mono_in = (static_cast<int32_t>(in_l.raw) + in_r.raw) >> 1;
        for (size_t s = 0; s < NUM_STAGES; ++s) {
            buffers_[s][write_pos_] = FixedQ15::from_raw(static_cast<int16_t>(mono_in));
        }

        int32_t wet_l = 0;
        int32_t wet_r = 0;

        for (size_t s = 0; s < NUM_STAGES; ++s) {
            uint32_t phase = lfo_phase_ + static_cast<uint32_t>(s) * PHASE_STEP;
            uint32_t idx = (phase >> 24) & 0xFF;
            int32_t sin_val = (static_cast<int32_t>(sin_lut_[idx]) + 32768) >> 1; // 0 to 32767

            // Delay offset: base + (depth * sin_val) >> 15
            size_t delay = BASE_DELAYS[s] + ((depth_ * sin_val) >> 15);
            size_t read_pos = (write_pos_ + MAX_BUF - delay) & MASK;
            int32_t sample = buffers_[s][read_pos].raw;

            // Pan: even stages to Left, odd stages to Right
            if ((s % 2) == 0) {
                wet_l += sample;
            } else {
                wet_r += sample;
            }
        }

        write_pos_ = (write_pos_ + 1) & MASK;

        // Average 3 stages per channel
        wet_l = wet_l / 3;
        wet_r = wet_r / 3;

        // Dry / Wet mix
        int32_t dry_mul = 32767 - mix_.raw;
        int32_t out_l = ((static_cast<int32_t>(in_l.raw) * dry_mul) >> 15) + ((wet_l * mix_.raw) >> 15);
        int32_t out_r = ((static_cast<int32_t>(in_r.raw) * dry_mul) >> 15) + ((wet_r * mix_.raw) >> 15);

        out_l = std::clamp<int32_t>(out_l, -32768, 32767);
        out_r = std::clamp<int32_t>(out_r, -32768, 32767);

        return { FixedQ15::from_raw(static_cast<int16_t>(out_l)), FixedQ15::from_raw(static_cast<int16_t>(out_r)) };
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

    std::array<std::array<FixedQ15, MAX_BUF>, NUM_STAGES> buffers_{};
    size_t write_pos_{0};
    uint32_t lfo_phase_{0};
    uint32_t rate_inc_{1000000};
    size_t depth_{16};
    FixedQ15 mix_{FixedQ15::from_raw(16384)}; // 50%
    std::array<int16_t, 256> sin_lut_{};
};

} // namespace aether::math
