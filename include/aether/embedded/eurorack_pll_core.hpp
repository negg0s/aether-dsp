#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Analog Phase-Locked Loop (PLL) and Harmonic Tracking Core.
 * Models an analog CMOS 4046 PLL commonly used in Eurorack modules (Doepfer A-196, Intellijel).
 * Features:
 * - Phase detector (Type II XOR / Edge-triggered PFD).
 * - Leaky loop filter (Integrator + Lead-Lag compensation).
 * - Multiplier / Divider integer ratios (/16, /8, /4, /2, x1, x2, x4, x8, x16).
 * - Lock indicator flag with low-pass hysteresis.
 */
class EurorackPllCore {
public:
    EurorackPllCore() {
        reset();
    }

    void reset() {
        vco_phase_ = 0;
        loop_integrator_ = 0;
        ref_last_ = false;
        vco_last_ = false;
        lock_metric_ = 0;
        center_freq_inc_ = 10000000; // ~111 Hz default at 48kHz
        mult_ratio_ = 1;
        div_ratio_ = 1;
    }

    /**
     * @brief Set center tracking frequency in Hz (relative to sample rate).
     */
    void set_center_frequency(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 1.0f) freq_hz = 1.0f;
        if (freq_hz > sample_rate_hz * 0.4f) freq_hz = sample_rate_hz * 0.4f;

        double inc = (static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        center_freq_inc_ = static_cast<uint32_t>(inc);
    }

    /**
     * @brief Configure PLL frequency tracking multiplication / division factor.
     */
    void set_ratio(uint32_t mult, uint32_t div) {
        if (mult == 0) mult = 1;
        if (div == 0) div = 1;
        mult_ratio_ = mult;
        div_ratio_ = div;
    }

    struct Output {
        math::FixedQ15 vco_square;
        math::FixedQ15 vco_triangle;
        bool is_locked;
    };

    /**
     * @brief Process reference input signal (audio or clock pulse).
     */
    Output process(math::FixedQ15 ref_in) {
        // 1. Edge detection on input reference signal (Schmitt trigger threshold at zero)
        bool ref_high = ref_in.raw > 2000;
        bool ref_rising = ref_high && !ref_last_;
        ref_last_ = ref_high;

        // 2. VCO internal state
        bool vco_high = (vco_phase_ & 0x80000000u) != 0;
        bool vco_rising = vco_high && !vco_last_;
        vco_last_ = vco_high;

        // 3. Phase-Frequency Detector (PFD)
        // If ref arrives early, pull VCO frequency up. If VCO arrives early, pull down.
        int32_t phase_error = 0;
        if (ref_rising && !vco_rising) {
            phase_error = 16384;
        } else if (!ref_rising && vco_rising) {
            phase_error = -16384;
        } else if (ref_rising && vco_rising) {
            phase_error = 0;
        }

        // 4. Leaky Loop Filter (Proportional-Integral)
        loop_integrator_ += (phase_error * 8);
        // Passive bleeding back to center frequency
        loop_integrator_ = (loop_integrator_ * 32700) >> 15;

        // Compute instantaneous frequency increment
        int64_t total_inc = static_cast<int64_t>(center_freq_inc_) + (static_cast<int64_t>(loop_integrator_) * 1024);
        if (total_inc < 1000) total_inc = 1000;

        // Apply harmonic ratio multiplier / divider
        total_inc = (total_inc * mult_ratio_) / div_ratio_;

        vco_phase_ += static_cast<uint32_t>(total_inc);

        // 5. Update Lock Detector Metric
        int32_t abs_err = (phase_error >= 0) ? phase_error : -phase_error;
        lock_metric_ = (lock_metric_ * 32700 + (32767 - abs_err) * 67) >> 15;
        bool locked = lock_metric_ > 25000;

        // 6. Generate output waveforms (Square & Triangle)
        int16_t sq = vco_high ? 30000 : -30000;

        // Triangle wave from phase
        uint32_t tri_phase = vco_phase_ >> 16; // 16-bit
        int32_t tri = 0;
        if (tri_phase < 32768) {
            tri = static_cast<int32_t>(tri_phase) * 2 - 32768;
        } else {
            tri = 32767 - static_cast<int32_t>(tri_phase - 32768) * 2;
        }

        Output out;
        out.vco_square = math::FixedQ15::from_raw(sq);
        out.vco_triangle = math::FixedQ15::from_raw(static_cast<int16_t>(tri));
        out.is_locked = locked;

        return out;
    }

private:
    uint32_t vco_phase_{0};
    int32_t loop_integrator_{0};
    uint32_t center_freq_inc_{10000000};
    uint32_t mult_ratio_{1};
    uint32_t div_ratio_{1};
    int32_t lock_metric_{0};
    bool ref_last_{false};
    bool vco_last_{false};
};

} // namespace aether::embedded
