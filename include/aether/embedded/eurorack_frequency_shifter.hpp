#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Variable Frequency Pitch Shifter & Frequency Shifter (Bode Style).
 * Implements real-time single-sideband (SSB) modulation with Hilbert transform 90-degree phase shift network.
 * Features:
 * - 4-stage cascaded allpass network computing analytic in-phase (I) and quadrature (Q) signals.
 * - Internal quadrature carrier generator (sine & cosine in Q15).
 * - Upper sideband (USB) and lower sideband (LSB) outputs.
 * - Non-harmonic pitch shifting for bell tones, metallic rings, and acoustic dissonance.
 */
class EurorackFrequencyShifter {
public:
    EurorackFrequencyShifter() {
        reset();
    }

    void reset() {
        for (int i = 0; i < 4; ++i) {
            ap_i_[i] = 0;
            ap_q_[i] = 0;
        }
        carrier_phase_ = 0;
        carrier_inc_ = 1000000; // default ~11 Hz shift
    }

    /**
     * @brief Set frequency shift in Hz (-500 Hz to +500 Hz).
     */
    void set_shift_frequency(float shift_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (shift_hz < -2000.0f) shift_hz = -2000.0f;
        if (shift_hz > 2000.0f) shift_hz = 2000.0f;

        double inc = (static_cast<double>(shift_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        carrier_inc_ = static_cast<uint32_t>(inc);
    }

    struct Output {
        math::FixedQ15 upper_sideband; // Up-shifted
        math::FixedQ15 lower_sideband; // Down-shifted
    };

    Output process(math::FixedQ15 input) {
        // 1. Advance carrier phase
        carrier_phase_ += carrier_inc_;

        // Quadrature carrier: Cosine and Sine
        int32_t carrier_cos = compute_sin(carrier_phase_ + 1073741824u); // +90 deg
        int32_t carrier_sin = compute_sin(carrier_phase_);

        // 2. Hilbert Transform Phase-Splitter Network (I and Q branches)
        // 4 cascaded allpass filters with pole values designed for 90-deg difference
        // I branch coefficients: k = {0.161758, 0.733029, 0.94535, 0.990598}
        // Q branch coefficients: k = {0.479401, 0.876244, 0.976599, 0.997500}
        const int16_t coeff_i[4] = { 5300, 24018, 30977, 32460 };
        const int16_t coeff_q[4] = { 15709, 28712, 32001, 32686 };

        int32_t sig_i = input.raw;
        int32_t sig_q = input.raw;

        for (int i = 0; i < 4; ++i) {
            sig_i = step_allpass(sig_i, ap_i_[i], coeff_i[i]);
            sig_q = step_allpass(sig_q, ap_q_[i], coeff_q[i]);
        }

        // 3. Single-Sideband Modulation (Bode Frequency Shifter equation)
        // USB = I * cos(wc*t) - Q * sin(wc*t)
        // LSB = I * cos(wc*t) + Q * sin(wc*t)
        int32_t i_cos = (static_cast<int64_t>(sig_i) * carrier_cos) >> 15;
        int32_t q_sin = (static_cast<int64_t>(sig_q) * carrier_sin) >> 15;

        int32_t usb = i_cos - q_sin;
        int32_t lsb = i_cos + q_sin;

        if (usb > 32767) usb = 32767;
        if (usb < -32768) usb = -32768;
        if (lsb > 32767) lsb = 32767;
        if (lsb < -32768) lsb = -32768;

        Output out;
        out.upper_sideband = math::FixedQ15::from_raw(static_cast<int16_t>(usb));
        out.lower_sideband = math::FixedQ15::from_raw(static_cast<int16_t>(lsb));
        return out;
    }

private:
    int32_t ap_i_[4];
    int32_t ap_q_[4];
    uint32_t carrier_phase_{0};
    uint32_t carrier_inc_{1000000};

    // 1st-order allpass filter step: y[n] = c * x[n] + x[n-1] - c * y[n-1]
    inline int32_t step_allpass(int32_t in, int32_t& state, int16_t coeff) {
        int32_t v = in - ((static_cast<int64_t>(coeff) * state) >> 15);
        int32_t out = state + ((static_cast<int64_t>(coeff) * v) >> 15);
        state = in;
        return out;
    }

    // Fast parabolic sine approximation in Q15
    inline int32_t compute_sin(uint32_t phase) {
        uint32_t p = phase >> 16;
        int32_t x = 0;
        if (p < 32768) {
            x = static_cast<int32_t>(p) * 2 - 32768;
        } else {
            x = 32767 - static_cast<int32_t>(p - 32768) * 2;
        }
        // Parabolic curve: 4*x*(1 - |x|)
        int32_t abs_x = (x >= 0) ? x : -x;
        int32_t curve = (static_cast<int64_t>(x) * (32767 - abs_x)) >> 13;
        return curve;
    }
};

} // namespace aether::embedded
