#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * @brief Zero-heap embedded Comb Delay Filter in FixedQ15 format.
 * Implements prime-length circular delay buffer with feedback and damping.
 */
template <size_t DELAY_LENGTH>
class FixedCombFilter {
public:
    FixedCombFilter() noexcept {
        reset();
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15(0));
        idx_ = 0;
        filter_state_ = FixedQ15(0);
    }

    void configure(FixedQ15 feedback, FixedQ15 damping) noexcept {
        feedback_ = feedback;
        damping_ = damping;
    }

    [[nodiscard]] inline FixedQ15 process(FixedQ15 in) noexcept {
        const FixedQ15 out = buffer_[idx_];

        // Lowpass damping: state = state * (1 - damping) + out * damping
        const FixedQ15 damp_term = out * damping_;
        const FixedQ15 one_minus_damp = FixedQ15::from_float(1.0f) - damping_;
        filter_state_ = (filter_state_ * one_minus_damp) + damp_term;

        // Feedback
        buffer_[idx_] = in + (filter_state_ * feedback_);

        idx_++;
        if (idx_ >= DELAY_LENGTH) {
            idx_ = 0;
        }

        return out;
    }

private:
    std::array<FixedQ15, DELAY_LENGTH> buffer_{};
    size_t idx_{0};
    FixedQ15 feedback_{FixedQ15::from_float(0.7f)};
    FixedQ15 damping_{FixedQ15::from_float(0.25f)};
    FixedQ15 filter_state_{FixedQ15(0)};
};

/**
 * @brief Zero-heap embedded Allpass Filter in FixedQ15 format.
 */
template <size_t DELAY_LENGTH>
class FixedAllpassFilter {
public:
    FixedAllpassFilter() noexcept {
        reset();
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15(0));
        idx_ = 0;
    }

    void configure(FixedQ15 feedback) noexcept {
        feedback_ = feedback;
    }

    [[nodiscard]] inline FixedQ15 process(FixedQ15 in) noexcept {
        const FixedQ15 buf_out = buffer_[idx_];
        const FixedQ15 out = -in + buf_out;
        buffer_[idx_] = in + (buf_out * feedback_);

        idx_++;
        if (idx_ >= DELAY_LENGTH) {
            idx_ = 0;
        }

        return out;
    }

private:
    std::array<FixedQ15, DELAY_LENGTH> buffer_{};
    size_t idx_{0};
    FixedQ15 feedback_{FixedQ15::from_float(0.5f)};
};

/**
 * @brief Complete Fixed-point Q15 Freeverb-architecture algorithmic reverb
 * for ultra-low power ARM microcontrollers (Daisy Seed, Cortex-M4/M7).
 * Zero heap allocation, pure integer DSP.
 */
class FixedReverb {
public:
    FixedReverb() noexcept {
        reset();
    }

    void reset() noexcept {
        comb1_.reset();
        comb2_.reset();
        comb3_.reset();
        comb4_.reset();
        ap1_.reset();
        ap2_.reset();
    }

    void configure(float feedback_norm = 0.75f, float damp_norm = 0.2f, float wet_mix = 0.35f) noexcept {
        feedback_ = FixedQ15::from_float(feedback_norm);
        damping_ = FixedQ15::from_float(damp_norm);
        wet_mix_ = FixedQ15::from_float(wet_mix);

        comb1_.configure(feedback_, damping_);
        comb2_.configure(feedback_, damping_);
        comb3_.configure(feedback_, damping_);
        comb4_.configure(feedback_, damping_);
        ap1_.configure(FixedQ15::from_float(0.5f));
        ap2_.configure(FixedQ15::from_float(0.5f));
    }

    [[nodiscard]] inline FixedQ15 process(FixedQ15 in) noexcept {
        // Parallel Comb filters
        const FixedQ15 c1 = comb1_.process(in);
        const FixedQ15 c2 = comb2_.process(in);
        const FixedQ15 c3 = comb3_.process(in);
        const FixedQ15 c4 = comb4_.process(in);

        // Sum scaled down to avoid overflow
        const int32_t sum = (static_cast<int32_t>(c1.raw) + c2.raw + c3.raw + c4.raw) >> 2;
        const FixedQ15 comb_sum(static_cast<int16_t>(sum));

        // Series Allpass filters for diffusion
        const FixedQ15 diffused1 = ap1_.process(comb_sum);
        const FixedQ15 diffused2 = ap2_.process(diffused1);

        // Dry/Wet mixing
        const FixedQ15 one_minus_wet = FixedQ15::from_float(1.0f) - wet_mix_;
        return (in * one_minus_wet) + (diffused2 * wet_mix_);
    }

private:
    // Mutually prime delay line sizes
    FixedCombFilter<1116> comb1_;
    FixedCombFilter<1188> comb2_;
    FixedCombFilter<1277> comb3_;
    FixedCombFilter<1356> comb4_;
    FixedAllpassFilter<225> ap1_;
    FixedAllpassFilter<341> ap2_;

    FixedQ15 feedback_{FixedQ15::from_float(0.75f)};
    FixedQ15 damping_{FixedQ15::from_float(0.2f)};
    FixedQ15 wet_mix_{FixedQ15::from_float(0.35f)};
};

} // namespace aether::math
