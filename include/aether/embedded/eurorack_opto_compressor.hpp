#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Ring Dynamic Envelope Shaper & Optical Compressor / Limiter.
 * Models an electro-luminescent optical compressor (Teletronix LA-2A style).
 * Features:
 * - Two-stage non-linear release curve modeling luminescent phosphor decay lag (fast first 50%, sluggish final release).
 * - Variable peak reduction threshold and makeup gain.
 * - Soft program-dependent compression ratio.
 * - Pure integer arithmetic in Q15 format, zero heap allocation.
 */
class EurorackOptoCompressor {
public:
    EurorackOptoCompressor() {
        reset();
    }

    void reset() {
        luminescence_ = 0;
        threshold_q15_ = 12000;
        makeup_gain_q15_ = 32767; // 1.0x gain
        gain_reduction_ = 32767;
    }

    void set_peak_reduction(math::FixedQ15 reduction) {
        if (reduction.raw < 1000) reduction.raw = 1000;
        threshold_q15_ = 32767 - reduction.raw;
    }

    void set_makeup_gain(math::FixedQ15 gain) {
        makeup_gain_q15_ = gain.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        int32_t in = input.raw;
        int32_t abs_in = (in >= 0) ? in : -in;

        // 1. Optical cell charging (fast attack ~10 ms)
        if (abs_in > luminescence_) {
            luminescence_ += ((static_cast<int64_t>(abs_in - luminescence_) * 3500) >> 15);
        } else {
            // Phosphor decay: Two-stage release lag
            // Fast decay when bright, sluggish slow release when dim
            int16_t decay_coeff = (luminescence_ > 8000) ? 32400 : 32710;
            luminescence_ = (static_cast<int64_t>(luminescence_) * decay_coeff) >> 15;
        }

        // 2. Gain reduction curve: soft optical knee
        int32_t target_gain = 32767;
        if (luminescence_ > threshold_q15_) {
            int32_t excess = luminescence_ - threshold_q15_;
            // Soft compression curve: 1.0 / (1.0 + excess / threshold)
            int32_t denom = threshold_q15_ + (excess >> 1);
            if (denom <= 0) denom = 1;
            target_gain = static_cast<int32_t>((static_cast<int64_t>(threshold_q15_) * 32767) / denom);
        }

        // Smooth gain reduction
        gain_reduction_ += ((static_cast<int64_t>(target_gain - gain_reduction_) * 8000) >> 15);

        // 3. Apply gain reduction and makeup gain
        int32_t compressed = (static_cast<int64_t>(in) * gain_reduction_) >> 15;
        int32_t out = (static_cast<int64_t>(compressed) * makeup_gain_q15_) >> 14; // up to 2x gain

        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t luminescence_{0};
    int16_t threshold_q15_{12000};
    int16_t makeup_gain_q15_{32767};
    int32_t gain_reduction_{32767};
};

} // namespace aether::embedded
