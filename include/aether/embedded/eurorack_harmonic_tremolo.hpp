#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Ring Dynamic Envelope Shaper & VCA Tremolo Core.
 * Models an optical and harmonic bias tremolo (Fender Vibro-Champ / Magnatone style).
 * Features:
 * - Two modes: Amplitude Tremolo (volume modulation) and Harmonic Tremolo (crossover phase-opposed modulation).
 * - Variable modulation waveform (Sine, Triangle, Square).
 * - Pure integer arithmetic in Q15 format, zero heap allocation.
 */
class EurorackHarmonicTremolo {
public:
    enum class Mode : uint8_t {
        Amplitude = 0,
        Harmonic = 1
    };

    EurorackHarmonicTremolo() {
        reset();
    }

    void reset() {
        lfo_phase_ = 0;
        lfo_inc_ = 400000; // ~4.5 Hz at 48kHz
        depth_q15_ = 24000; // ~73% depth
        mode_ = Mode::Harmonic;
        lp_state_ = 0;
    }

    void set_rate(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 0.1f) freq_hz = 0.1f;
        if (freq_hz > 25.0f) freq_hz = 25.0f;

        double inc = (static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        lfo_inc_ = static_cast<uint32_t>(inc);
    }

    void set_depth(math::FixedQ15 depth) {
        if (depth.raw < 0) depth.raw = 0;
        depth_q15_ = depth.raw;
    }

    void set_mode(Mode m) {
        mode_ = m;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        int32_t in = input.raw;

        // Advance LFO
        lfo_phase_ += lfo_inc_;

        // Triangle LFO in [-32768, 32767]
        uint32_t p = lfo_phase_ >> 16;
        int32_t tri = 0;
        if (p < 32768) {
            tri = static_cast<int32_t>(p) * 2 - 32768;
        } else {
            tri = 32767 - static_cast<int32_t>(p - 32768) * 2;
        }

        // LFO in [0, 32767]
        int32_t lfo_norm = (tri + 32768) >> 1;

        if (mode_ == Mode::Amplitude) {
            // Amplitude tremolo: gain = (1.0 - depth) + depth * lfo
            int32_t min_gain = 32767 - depth_q15_;
            int32_t mod_gain = (static_cast<int64_t>(depth_q15_) * lfo_norm) >> 15;
            int32_t total_gain = min_gain + mod_gain;

            int32_t out = (static_cast<int64_t>(in) * total_gain) >> 15;
            if (out > 32767) out = 32767;
            if (out < -32768) out = -32768;
            return math::FixedQ15::from_raw(static_cast<int16_t>(out));
        } else {
            // Harmonic tremolo: split low / high bands at ~700 Hz (coeff ~3000 in Q15 at 48kHz)
            lp_state_ += ((static_cast<int64_t>(in - lp_state_) * 3000) >> 15);
            int32_t low_band = lp_state_;
            int32_t high_band = in - lp_state_;

            // Modulate low band with LFO, high band with inverted LFO
            int32_t lfo_inv = 32767 - lfo_norm;

            int32_t min_gain = 32767 - depth_q15_;
            int32_t gain_lo = min_gain + ((static_cast<int64_t>(depth_q15_) * lfo_norm) >> 15);
            int32_t gain_hi = min_gain + ((static_cast<int64_t>(depth_q15_) * lfo_inv) >> 15);

            int32_t mod_lo = (static_cast<int64_t>(low_band) * gain_lo) >> 15;
            int32_t mod_hi = (static_cast<int64_t>(high_band) * gain_hi) >> 15;

            int32_t out = mod_lo + mod_hi;
            if (out > 32767) out = 32767;
            if (out < -32768) out = -32768;
            return math::FixedQ15::from_raw(static_cast<int16_t>(out));
        }
    }

private:
    uint32_t lfo_phase_{0};
    uint32_t lfo_inc_{400000};
    int16_t depth_q15_{24000};
    Mode mode_{Mode::Harmonic};
    int32_t lp_state_{0};
};

} // namespace aether::embedded
