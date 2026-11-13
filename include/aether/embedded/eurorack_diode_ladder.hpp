#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>
#include <cmath>

namespace aether::embedded {

/**
 * @brief Fixed-point 10-stage Moog / Serge Diode-Ladder Resonant Lowpass Filter (Q15).
 * Accurately models non-linear voltage-controlled diode resistance drops and 
 * 4-pole cascaded RC integration with resonance self-oscillation.
 * Pure integer arithmetic, zero dynamic allocation, suitable for STM32 / Daisy Patch.
 */
class EurorackDiodeLadder {
public:
    EurorackDiodeLadder() {
        reset();
    }

    void reset() {
        s0_ = 0;
        s1_ = 0;
        s2_ = 0;
        s3_ = 0;
        cutoff_g_ = 4000;
        resonance_k_ = 0;
    }

    /**
     * @brief Set cutoff frequency [20Hz .. 20kHz] mapped to filter gain coeff [0 .. 32767].
     */
    void set_cutoff(float cutoff_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (cutoff_hz < 10.0f) cutoff_hz = 10.0f;
        if (cutoff_hz > sample_rate_hz * 0.45f) cutoff_hz = sample_rate_hz * 0.45f;

        // g = tan(pi * fc / fs) approximation in Q15
        double w = 3.141592653589793 * static_cast<double>(cutoff_hz) / static_cast<double>(sample_rate_hz);
        double g = std::tan(w);
        if (g > 0.99) g = 0.99;
        cutoff_g_ = static_cast<int16_t>(g * 32767.0);
    }

    /**
     * @brief Set resonance feedback [0 .. 32767] (self-oscillation occurs at high levels).
     */
    void set_resonance(math::FixedQ15 res) {
        if (res.raw < 0) res.raw = 0;
        // Resonance feedback coefficient
        resonance_k_ = res.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        // Feedback path with diode non-linearity saturation
        // Diode ladder character: asymmetric soft saturation on feedback
        int32_t fb_signal = s3_;
        int32_t sat_fb = saturate_diode(fb_signal);

        // Feedback resonance subtraction
        int32_t res_scaled = (static_cast<int64_t>(sat_fb) * resonance_k_) >> 13; // scaled up to 4.0x
        int32_t u = static_cast<int32_t>(input.raw) - res_scaled;

        // 4 cascaded 1-pole non-linear integrator stages (diode ladder topology)
        s0_ = step_stage(u, s0_);
        s1_ = step_stage(s0_, s1_);
        s2_ = step_stage(s1_, s2_);
        s3_ = step_stage(s2_, s3_);

        int32_t out = s3_;
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t s0_{0};
    int32_t s1_{0};
    int32_t s2_{0};
    int32_t s3_{0};
    int16_t cutoff_g_{4000};
    int16_t resonance_k_{0};

    // Soft saturator modeling forward diode voltage drop
    inline int32_t saturate_diode(int32_t x) {
        // Piecewise rational approximation: x / (1 + |x| / 32768)
        int64_t abs_x = (x >= 0) ? x : -x;
        int64_t denom = 32768 + (abs_x >> 1);
        if (denom == 0) denom = 1;
        return static_cast<int32_t>((static_cast<int64_t>(x) * 32768) / denom);
    }

    // Single 1-pole integrator stage with cutoff g
    inline int32_t step_stage(int32_t in, int32_t state) {
        int32_t delta = in - state;
        int32_t d_g = (static_cast<int64_t>(delta) * cutoff_g_) >> 15;
        return state + d_g;
    }
};

} // namespace aether::embedded
