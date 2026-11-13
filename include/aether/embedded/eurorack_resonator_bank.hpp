#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Spectral Resonator & Comb Bank (4 parallel tuned tuned resonators).
 * Implements a 4-channel modal acoustic resonator bank (4 tuned bandpass comb filters).
 * Creates rich polyphonic metallic, wooden, and glass acoustic resonances from clicks or noise.
 * Pure integer arithmetic, zero dynamic allocation, designed for STM32 / Daisy Patch Eurorack.
 */
class EurorackResonatorBank {
public:
    static constexpr size_t kNumModes = 4;

    EurorackResonatorBank() {
        reset();
    }

    void reset() {
        for (size_t i = 0; i < kNumModes; ++i) {
            y1_[i] = 0;
            y2_[i] = 0;
            freq_coeff_[i] = 16000;
        }
        decay_coeff_q15_ = 32000; // ~0.976 decay
        structure_q15_ = 8192;    // Harmonic overtone spread
        set_base_frequency(220.0f, 48000.0f);
    }

    /**
     * @brief Set base fundamental tuning frequency [20 Hz .. 2000 Hz].
     */
    void set_base_frequency(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 20.0f) freq_hz = 20.0f;
        if (freq_hz > 2500.0f) freq_hz = 2500.0f;

        base_freq_hz_ = freq_hz;
        sample_rate_hz_ = sample_rate_hz;
        update_modes();
    }

    /**
     * @brief Set resonance decay / damping [0, 32767].
     */
    void set_decay(math::FixedQ15 decay) {
        if (decay.raw < 1000) decay.raw = 1000;
        if (decay.raw > 32700) decay.raw = 32700;
        decay_coeff_q15_ = decay.raw;
    }

    /**
     * @brief Structure morphs mode ratios from harmonic (string) to inharmonic (metal/bell) [0, 32767].
     */
    void set_structure(math::FixedQ15 structure) {
        structure_q15_ = structure.raw;
        update_modes();
    }

    math::FixedQ15 process(math::FixedQ15 excitation) {
        int32_t out_accum = 0;

        for (size_t i = 0; i < kNumModes; ++i) {
            // 2-pole resonant bandpass filter (Goertzel/Biquad resonator):
            // y[n] = 2*r*cos(w)*y[n-1] - r^2*y[n-2] + x[n]
            // coeff = 2*cos(w) in Q14
            int32_t c = freq_coeff_[i]; // Q14
            int32_t term1 = (static_cast<int64_t>(y1_[i]) * c) >> 14;
            int32_t term2 = (static_cast<int64_t>(y2_[i]) * decay_coeff_q15_) >> 15;

            int32_t y0 = excitation.raw + term1 - term2;

            // Decay attenuation applied to feedback
            y0 = (static_cast<int64_t>(y0) * decay_coeff_q15_) >> 15;

            y2_[i] = y1_[i];
            y1_[i] = y0;

            out_accum += y0;
        }

        // Scale output sum of 4 modes
        out_accum >>= 2;

        if (out_accum > 32767) out_accum = 32767;
        if (out_accum < -32768) out_accum = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out_accum));
    }

private:
    int32_t y1_[kNumModes];
    int32_t y2_[kNumModes];
    int16_t freq_coeff_[kNumModes];
    int16_t decay_coeff_q15_{32000};
    int16_t structure_q15_{8192};
    float base_freq_hz_{220.0f};
    float sample_rate_hz_{48000.0f};

    void update_modes() {
        // Mode ratios interpolate between harmonic (1, 2, 3, 4) and inharmonic (1, 2.76, 5.4, 8.9)
        float s = static_cast<float>(structure_q15_) / 32767.0f;
        float ratios[kNumModes] = {
            1.0f,
            2.0f + s * 0.76f,
            3.0f + s * 2.4f,
            4.0f + s * 4.9f
        };

        for (size_t i = 0; i < kNumModes; ++i) {
            float f = base_freq_hz_ * ratios[i];
            if (f > sample_rate_hz_ * 0.45f) f = sample_rate_hz_ * 0.45f;

            // 2 * cos(2 * pi * f / fs) in Q14 (scale 16384)
            double w = 2.0 * 3.141592653589793 * static_cast<double>(f) / static_cast<double>(sample_rate_hz_);
            double c = 2.0 * std::cos(w);
            freq_coeff_[i] = static_cast<int16_t>(c * 16384.0);
        }
    }
};

} // namespace aether::embedded
