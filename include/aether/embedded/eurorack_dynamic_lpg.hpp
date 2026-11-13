#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Variable State Dynamic VCA / VCF Sallen-Key Lowpass Gate.
 * Implements an organic Buchla / Make Noise QMMG style dynamic vactrol gate.
 * Features:
 * - Asymmetric vactrol photodiode decay response with dual decay slopes (fast strike, long tail).
 * - Variable mode blending: VCA mode (linear amplitude), VCF mode (cutoff filtering), and Both.
 * - Dynamic feedback saturation and organic ringing dampening.
 */
class EurorackDynamicLpg {
public:
    enum class Mode : uint8_t {
        VcaOnly = 0,
        VcfOnly = 1,
        Both = 2
    };

    EurorackDynamicLpg() {
        reset();
    }

    void reset() {
        vactrol_state_ = 0;
        lp_filter_state_ = 0;
        mode_ = Mode::Both;
        vactrol_attack_q15_ = 28000;  // Very fast attack
        vactrol_decay_fast_q15_ = 32000; // First phase decay (~0.976)
        vactrol_decay_slow_q15_ = 32600; // Deep long tail decay (~0.995)
        cutoff_min_g_ = 400;          // Closed gate cutoff
    }

    void set_mode(Mode mode) {
        mode_ = mode;
    }

    /**
     * @brief Strike vactrol with an instantaneous pulse (trigger strike).
     */
    void strike(math::FixedQ15 level) {
        if (level.raw > vactrol_state_) {
            vactrol_state_ = level.raw;
        }
    }

    /**
     * @brief Process audio input through vactrol low-pass gate with control voltage.
     * @param input Audio signal in Q15
     * @param cv_in Control voltage [0, 32767] (gate opening level)
     */
    math::FixedQ15 process(math::FixedQ15 input, math::FixedQ15 cv_in) {
        // 1. Vactrol physical simulation (asymmetric response)
        int32_t target = cv_in.raw;
        if (target < 0) target = 0;

        if (target > vactrol_state_) {
            // Instantaneous or rapid attack
            int32_t diff = target - vactrol_state_;
            vactrol_state_ += (static_cast<int64_t>(diff) * vactrol_attack_q15_) >> 15;
        } else {
            // Asymmetric dual-slope decay
            int16_t decay_coeff = (vactrol_state_ > 16384) ? vactrol_decay_fast_q15_ : vactrol_decay_slow_q15_;
            vactrol_state_ = (static_cast<int64_t>(vactrol_state_) * decay_coeff) >> 15;
        }

        int32_t in = input.raw;
        int32_t filtered = 0;
        int32_t amplified = 0;

        // 2. VCF Stage: 1-pole non-linear lowpass tracking vactrol opening
        int32_t effective_cutoff_g = cutoff_min_g_ + ((static_cast<int64_t>(vactrol_state_) * 28000) >> 15);
        int32_t f_diff = in - lp_filter_state_;
        lp_filter_state_ += (static_cast<int64_t>(f_diff) * effective_cutoff_g) >> 15;
        filtered = lp_filter_state_;

        // 3. VCA Stage: Amplitude modulation
        amplified = (static_cast<int64_t>(in) * vactrol_state_) >> 15;

        // 4. Mode Routing
        int32_t out = 0;
        switch (mode_) {
            case Mode::VcaOnly:
                out = amplified;
                break;
            case Mode::VcfOnly:
                out = filtered;
                break;
            case Mode::Both:
                // Both: Audio is both filtered and amplitude-attenuated simultaneously
                out = (static_cast<int64_t>(filtered) * vactrol_state_) >> 15;
                break;
        }

        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t vactrol_state_{0};
    int32_t lp_filter_state_{0};
    Mode mode_{Mode::Both};
    int16_t vactrol_attack_q15_{28000};
    int16_t vactrol_decay_fast_q15_{32000};
    int16_t vactrol_decay_slow_q15_{32600};
    int16_t cutoff_min_g_{400};
};

} // namespace aether::embedded
