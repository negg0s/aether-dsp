#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Ring Dynamic Envelope Shaper & VCA Pumper.
 * Features:
 * - Sidechain audio ducking / pumping (ducking compressor curve).
 * - Variable trigger recovery time (sidechain release).
 * - Pure integer arithmetic in Q15 format, zero heap allocation.
 */
class EurorackDuckingVca {
public:
    EurorackDuckingVca() {
        reset();
    }

    void reset() {
        duck_amount_q15_ = 24000; // ~73% ducking depth
        envelope_ = 0;
        release_coeff_q15_ = 32000; // ~0.976 release
    }

    void set_duck_depth(math::FixedQ15 depth) {
        if (depth.raw < 0) depth.raw = 0;
        duck_amount_q15_ = depth.raw;
    }

    void set_release(math::FixedQ15 rel) {
        if (rel.raw < 1000) rel.raw = 1000;
        if (rel.raw > 32700) rel.raw = 32700;
        release_coeff_q15_ = rel.raw;
    }

    void trigger_sidechain(math::FixedQ15 trig) {
        if (trig.raw > envelope_) {
            envelope_ = trig.raw;
        }
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        // Step release envelope
        envelope_ = (static_cast<int64_t>(envelope_) * release_coeff_q15_) >> 15;

        // Duck gain = 1.0 - (envelope * duck_amount)
        int32_t attenuation = (static_cast<int64_t>(envelope_) * duck_amount_q15_) >> 15;
        int32_t gain = 32767 - attenuation;
        if (gain < 0) gain = 0;

        int32_t out = (static_cast<int64_t>(input.raw) * gain) >> 15;
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int16_t duck_amount_q15_{24000};
    int32_t envelope_{0};
    int16_t release_coeff_q15_{32000};
};

} // namespace aether::embedded
