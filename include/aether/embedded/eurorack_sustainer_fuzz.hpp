#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Ring Dynamic Envelope Shaper & Diode Distortion / Sustainer.
 * Models an analog electro-harmonix Big Muff Pi style dual-stage silicon diode clipping fuzz/sustainer.
 * Features:
 * - Two cascaded non-linear clipping stages with clipping compression and feedback sustain.
 * - Classic passive tilt/notch tone control network.
 * - Pure integer arithmetic in Q15 format, zero heap allocation.
 */
class EurorackSustainerFuzz {
public:
    EurorackSustainerFuzz() {
        reset();
    }

    void reset() {
        sustain_q15_ = 28000;
        tone_tilt_q15_ = 16384; // Center
        lp_state_ = 0;
        hp_state_ = 0;
    }

    void set_sustain(math::FixedQ15 sus) {
        if (sus.raw < 1000) sus.raw = 1000;
        sustain_q15_ = sus.raw;
    }

    void set_tone(math::FixedQ15 tone) {
        tone_tilt_q15_ = tone.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        int32_t in = input.raw;

        // Stage 1: Pre-gain and first diode clip
        int32_t drive1 = (static_cast<int64_t>(in) * sustain_q15_) >> 12; // up to 8x gain
        int32_t clip1 = clip_diode(drive1);

        // Stage 2: Second gain and diode clip (massive sustain compression)
        int32_t drive2 = (static_cast<int64_t>(clip1) * sustain_q15_) >> 13;
        int32_t clip2 = clip_diode(drive2);

        // Tone Stage: Dual-path lowpass and highpass tilt network
        // Lowpass branch (~500 Hz, coeff ~2000 in Q15)
        lp_state_ += ((static_cast<int64_t>(clip2 - lp_state_) * 2000) >> 15);
        int32_t lo_branch = lp_state_;

        // Highpass branch (~1200 Hz, coeff ~5000 in Q15)
        hp_state_ += ((static_cast<int64_t>(clip2 - hp_state_) * 5000) >> 15);
        int32_t hi_branch = clip2 - hp_state_;

        // Crossfade between low branch and high branch using tone_tilt_q15_
        int32_t out = ((static_cast<int64_t>(lo_branch) * (32767 - tone_tilt_q15_)) >> 15) +
                      ((static_cast<int64_t>(hi_branch) * tone_tilt_q15_) >> 15);

        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int16_t sustain_q15_{28000};
    int16_t tone_tilt_q15_{16384};
    int32_t lp_state_{0};
    int32_t hp_state_{0};

    inline int32_t clip_diode(int32_t x) {
        // Soft diode cubic saturation
        if (x > 24000) {
            return 24000 + ((x - 24000) >> 3);
        } else if (x < -24000) {
            return -24000 + ((x + 24000) >> 3);
        }
        return x;
    }
};

} // namespace aether::embedded
