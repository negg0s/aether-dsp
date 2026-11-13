#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Ring Frequency Doubler & Rectifier.
 * Models an analog full-wave precision rectifier and transformer-coupled octave doubler (Mu-Tron Octave Divider / Green Ringer).
 * Features:
 * - Full-wave absolute value rectification with DC offset compensation filter.
 * - Dynamic crossover distortion knee (diode forward drop simulation).
 * - Generates clean second harmonic (1 octave up) and metallic intermodulation artifacts on chords.
 */
class EurorackOctaveDoubler {
public:
    EurorackOctaveDoubler() {
        reset();
    }

    void reset() {
        dc_blocker_x_ = 0;
        dc_blocker_y_ = 0;
        knee_q15_ = 2000; // Small forward diode threshold
        mix_q15_ = 32767; // 100% octave up
    }

    void set_knee(math::FixedQ15 knee) {
        if (knee.raw < 0) knee.raw = 0;
        if (knee.raw > 16384) knee.raw = 16384;
        knee_q15_ = knee.raw;
    }

    void set_mix(math::FixedQ15 mix) {
        if (mix.raw < 0) mix.raw = 0;
        mix_q15_ = mix.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        int32_t in = input.raw;

        // 1. Diode rectification with crossover knee
        int32_t abs_in = (in >= 0) ? in : -in;
        int32_t rectified = 0;
        if (abs_in > knee_q15_) {
            rectified = (abs_in - knee_q15_) * 2;
        }

        // 2. DC-Blocking highpass filter on rectified signal:
        // y[n] = x[n] - x[n-1] + R * y[n-1], where R = 0.992 (~32500 in Q15)
        int32_t diff = rectified - dc_blocker_x_;
        int32_t filtered = diff + ((static_cast<int64_t>(dc_blocker_y_) * 32500) >> 15);
        dc_blocker_x_ = rectified;
        dc_blocker_y_ = filtered;

        // 3. Dry / Octave-Up Wet Crossfade
        int32_t wet = (static_cast<int64_t>(filtered) * mix_q15_) >> 15;
        int32_t dry = (static_cast<int64_t>(in) * (32767 - mix_q15_)) >> 15;
        int32_t out = dry + wet;

        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t dc_blocker_x_{0};
    int32_t dc_blocker_y_{0};
    int16_t knee_q15_{2000};
    int16_t mix_q15_{32767};
};

} // namespace aether::embedded
