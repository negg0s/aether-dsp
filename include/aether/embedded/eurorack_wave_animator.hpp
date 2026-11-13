#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Wave Animator / Harmonic Spatial Pulser.
 * Models an analog Serge / Intellijel wave animator module.
 * Features:
 * - Generates 3 delayed, phase-modulated cross-modulated replicas of an input waveform.
 * - Dynamic pulse width / threshold slicing on positive and negative half-waves.
 * - Internal triangle sub-LFOs for sweeping modulation nodes.
 * - Turns static saw or triangle waves into massive, lush animated pads.
 */
class EurorackWaveAnimator {
public:
    EurorackWaveAnimator() {
        reset();
    }

    void reset() {
        lfo_phase1_ = 0;
        lfo_phase2_ = 1431655765; // 120 deg
        lfo_phase3_ = 2863311530; // 240 deg
        lfo_inc_ = 100000;         // ~1.1 Hz at 48kHz
        depth_q15_ = 16384;        // 0.5 depth
        threshold_q15_ = 8192;     // 0.25 threshold
    }

    void set_speed(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 0.05f) freq_hz = 0.05f;
        if (freq_hz > 50.0f) freq_hz = 50.0f;

        double inc = (static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        lfo_inc_ = static_cast<uint32_t>(inc);
    }

    void set_depth(math::FixedQ15 depth) {
        if (depth.raw < 0) depth.raw = 0;
        depth_q15_ = depth.raw;
    }

    void set_threshold(math::FixedQ15 thres) {
        threshold_q15_ = thres.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        // Step sub-LFOs
        lfo_phase1_ += lfo_inc_;
        lfo_phase2_ += lfo_inc_;
        lfo_phase3_ += lfo_inc_;

        int32_t lfo1 = compute_tri(lfo_phase1_);
        int32_t lfo2 = compute_tri(lfo_phase2_);
        int32_t lfo3 = compute_tri(lfo_phase3_);

        // Three dynamic comparator slicing stages (Serge wave animator core)
        int32_t mod1 = threshold_q15_ + ((static_cast<int64_t>(lfo1) * depth_q15_) >> 15);
        int32_t mod2 = -threshold_q15_ + ((static_cast<int64_t>(lfo2) * depth_q15_) >> 15);
        int32_t mod3 = ((static_cast<int64_t>(lfo3) * depth_q15_) >> 15);

        int32_t in = input.raw;

        // Stage 1: positive fold / clip
        int32_t diff1 = in - mod1;
        int32_t slice1 = (diff1 > 0) ? -diff1 : diff1;

        // Stage 2: negative fold / clip
        int32_t diff2 = in - mod2;
        int32_t slice2 = (diff2 < 0) ? -diff2 : diff2;

        // Stage 3: center cross-fold
        int32_t diff3 = in - mod3;
        int32_t slice3 = (diff3 > 0) ? (32767 - diff3) : (-32768 - diff3);

        // Mix input with animated pulse slices
        int32_t out = in + ((slice1 + slice2 + slice3) / 3);
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    uint32_t lfo_phase1_{0};
    uint32_t lfo_phase2_{0};
    uint32_t lfo_phase3_{0};
    uint32_t lfo_inc_{100000};
    int16_t depth_q15_{16384};
    int16_t threshold_q15_{8192};

    // Fast triangle LFO in range [-32768, 32767]
    inline int32_t compute_tri(uint32_t phase) {
        uint32_t p = phase >> 16;
        if (p < 32768) {
            return static_cast<int32_t>(p) * 2 - 32768;
        } else {
            return 32767 - static_cast<int32_t>(p - 32768) * 2;
        }
    }
};

} // namespace aether::embedded
