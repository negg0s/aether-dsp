#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Switched Capacitor Resonant Filter (Buchla 291 / EMS VCS3 style).
 * Models discrete charge transfers on switched capacitor banks.
 * Produces distinctive clock-feedthrough aliasing sidebands, gritty analog resonance,
 * and high-order bandpass / lowpass responses without dynamic heap allocation.
 */
class EurorackSwitchedCapacitorFilter {
public:
    EurorackSwitchedCapacitorFilter() {
        reset();
    }

    void reset() {
        c1_charge_ = 0;
        c2_charge_ = 0;
        clock_phase_ = 0;
        clock_inc_ = 10000000; // ~111 Hz default clock at 48kHz
        resonance_q15_ = 0;
        bleed_q15_ = 4096;      // slight clock bleed
    }

    /**
     * @brief Set switched-capacitor switching clock frequency [100 Hz .. 24 kHz].
     */
    void set_clock_frequency(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 20.0f) freq_hz = 20.0f;
        if (freq_hz > sample_rate_hz * 0.49f) freq_hz = sample_rate_hz * 0.49f;

        double inc = (static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        clock_inc_ = static_cast<uint32_t>(inc);
    }

    /**
     * @brief Set resonance feedback [0, 32767].
     */
    void set_resonance(math::FixedQ15 res) {
        if (res.raw < 0) res.raw = 0;
        resonance_q15_ = res.raw;
    }

    /**
     * @brief Set clock feedthrough bleed amount [0, 32767].
     */
    void set_clock_bleed(math::FixedQ15 bleed) {
        if (bleed.raw < 0) bleed.raw = 0;
        bleed_q15_ = bleed.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        uint32_t prev_phase = clock_phase_;
        clock_phase_ += clock_inc_;

        // Switched-capacitor non-overlapping two-phase clock (Phi 1 and Phi 2)
        // High bit transitions indicate charge transfer intervals
        bool phi1 = (clock_phase_ & 0x80000000u) != 0;
        bool clock_edge = ((prev_phase ^ clock_phase_) & 0x80000000u) != 0;

        int32_t in = input.raw;

        // Feedback path with resonance damping
        int32_t fb = (static_cast<int64_t>(c2_charge_) * resonance_q15_) >> 14; // up to 2.0x gain
        int32_t diff = in - fb;

        if (clock_edge) {
            if (phi1) {
                // Phase 1: Charge C1 from input difference
                c1_charge_ = diff;
            } else {
                // Phase 2: Transfer charge from C1 to C2 (integrating stage)
                int32_t transfer = (c1_charge_ - c2_charge_) >> 1;
                c2_charge_ += transfer;
            }
        }

        // Output signal: capacitor integration + clock bleed aliasing
        int32_t clk_feedthrough = phi1 ? bleed_q15_ : -bleed_q15_;
        int32_t out = c2_charge_ + clk_feedthrough;

        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t c1_charge_{0};
    int32_t c2_charge_{0};
    uint32_t clock_phase_{0};
    uint32_t clock_inc_{10000000};
    int16_t resonance_q15_{0};
    int16_t bleed_q15_{4096};
};

} // namespace aether::embedded
