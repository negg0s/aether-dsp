#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Ring Dynamic Envelope Shaper & Diode Rectifier Fuzz.
 * Models an analog germanium/silicon crossover octave fuzz (Shin-Ei Companion / Univox Super-Fuzz).
 * Features:
 * - Dual complementary full-wave differential rectifiers producing octave-up gnarl.
 * - Dynamic mid-scoop tone filter (active passive notch at 1 kHz).
 * - Soft clipping saturation rails in Q15 format.
 */
class EurorackOctaveFuzz {
public:
    EurorackOctaveFuzz() {
        reset();
    }

    void reset() {
        drive_q15_ = 28000;
        tone_scoop_ = true;
        lp_state_ = 0;
        hp_state_ = 0;
    }

    void set_drive(math::FixedQ15 drive) {
        if (drive.raw < 1000) drive.raw = 1000;
        drive_q15_ = drive.raw;
    }

    void set_tone_scoop(bool scoop) {
        tone_scoop_ = scoop;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        int32_t in = input.raw;

        // 1. Pre-gain drive
        int32_t driven = (static_cast<int64_t>(in) * drive_q15_) >> 12; // up to 8x gain

        // 2. Differential pair rectifier (Super-Fuzz octave core):
        // |x| with soft diode knee
        int32_t abs_x = (driven >= 0) ? driven : -driven;
        int32_t rect = abs_x - (driven >> 2); // Asymmetrical blend for rich even+odd harmonics

        // 3. Diode clipping rails
        if (rect > 28000) rect = 28000 + ((rect - 28000) >> 2);
        if (rect < -28000) rect = -28000 + ((rect + 28000) >> 2);

        // 4. Tone stage: Mid-scoop filter (lowpass + highpass recombined with mid notch)
        int32_t out = rect;
        if (tone_scoop_) {
            // Lowpass at 350 Hz
            lp_state_ += ((static_cast<int64_t>(rect - lp_state_) * 1500) >> 15);
            // Highpass at 3000 Hz
            hp_state_ += ((static_cast<int64_t>(rect - hp_state_) * 12000) >> 15);
            int32_t hp_sig = rect - hp_state_;

            out = (lp_state_ + hp_sig) >> 1;
        }

        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int16_t drive_q15_{28000};
    bool tone_scoop_{true};
    int32_t lp_state_{0};
    int32_t hp_state_{0};
};

} // namespace aether::embedded
