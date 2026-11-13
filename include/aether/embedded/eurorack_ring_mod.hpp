#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Ring Modulator & Carrier Generator
// Integrates an internal digital sine-carrier oscillator with analog four-quadrant
// diode multiplier emulation and carrier feedthrough rejection.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackRingModulator {
public:
    EurorackRingModulator() {
        reset();
    }

    void reset() noexcept {
        carrier_phase_ = 0;
        carrier_phase_inc_ = 1000000; // ~440 Hz
        mix_q15_ = math::FixedQ15::from_float(1.0f);
        bleed_q15_ = math::FixedQ15(0);
    }

    // Set carrier frequency: carrier_phase_inc = (freq_hz * 2^32) / sample_rate
    void set_carrier_freq(uint32_t phase_inc) noexcept {
        carrier_phase_inc_ = phase_inc;
    }

    // Set dry/wet mix (0.0 to 1.0 in Q15) and carrier bleed feedthrough (-1.0 to +1.0)
    void configure(math::FixedQ15 mix, math::FixedQ15 bleed = math::FixedQ15(0)) noexcept {
        mix_q15_ = mix;
        bleed_q15_ = bleed;
    }

    // Process a single audio sample (modulator) against internal carrier
    math::FixedQ15 process(math::FixedQ15 audio_in) noexcept {
        // Synthesize carrier sine via parabolic approximation
        carrier_phase_ += carrier_phase_inc_;
        uint32_t phase16 = carrier_phase_ >> 16;
        int32_t tri = 0;
        if (phase16 < 32768) {
            tri = static_cast<int32_t>(phase16) - 16384;
        } else {
            tri = 49152 - static_cast<int32_t>(phase16);
        }
        // Parabolic wave shaping to sine: y = 2 * tri - (tri * |tri|) / 16384
        int32_t abs_tri = (tri < 0) ? -tri : tri;
        int32_t carrier_raw = (2 * tri) - ((tri * abs_tri) >> 14);
        if (carrier_raw > 32767) carrier_raw = 32767;
        if (carrier_raw < -32768) carrier_raw = -32768;

        // Four-quadrant multiplication: (audio_in * carrier) >> 15
        int64_t ring_prod = (static_cast<int64_t>(audio_in.raw) * carrier_raw) >> 15;

        // Add carrier bleed feedthrough
        ring_prod += (static_cast<int64_t>(bleed_q15_.raw) * carrier_raw) >> 15;

        // Dry/Wet interpolation: out = (1 - mix) * dry + mix * ring
        int64_t dry_part = (static_cast<int64_t>(audio_in.raw) * (32767 - mix_q15_.raw)) >> 15;
        int64_t wet_part = (ring_prod * mix_q15_.raw) >> 15;
        int64_t final_out = dry_part + wet_part;

        if (final_out > 32767) final_out = 32767;
        if (final_out < -32768) final_out = -32768;

        return math::FixedQ15(static_cast<int16_t>(final_out));
    }

private:
    uint32_t carrier_phase_{0};
    uint32_t carrier_phase_inc_{1000000};
    math::FixedQ15 mix_q15_{math::FixedQ15::from_float(1.0f)};
    math::FixedQ15 bleed_q15_{0};
};

} // namespace aether::embedded
