#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Transient & Percussion Drum Exciter Core.
 * Models an analog 808/909 bridged T-network percussion voice.
 * Features:
 * - Damped 2-pole bridged-T bandpass resonator with fast initial click pitch dive.
 * - Dynamic pitch envelope (snappy transient punch).
 * - Variable decay length and resonance self-damping.
 * - Suitable for bass drums, toms, woodblocks, and analog congas.
 */
class EurorackDrumExciter {
public:
    EurorackDrumExciter() {
        reset();
    }

    void reset() {
        y1_ = 0;
        y2_ = 0;
        pitch_env_ = 0;
        decay_coeff_q15_ = 32000;
        pitch_decay_q15_ = 28000;
        base_freq_coeff_q14_ = 8000;
        pitch_amount_q15_ = 16384;
        click_counter_ = 0;
    }

    void set_pitch(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 20.0f) freq_hz = 20.0f;
        if (freq_hz > 1000.0f) freq_hz = 1000.0f;

        // 2 * cos(2 * pi * f / fs) in Q14
        double w = 2.0 * 3.141592653589793 * static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz);
        double c = 2.0 * std::cos(w);
        base_freq_coeff_q14_ = static_cast<int16_t>(c * 16384.0);
    }

    void set_decay(math::FixedQ15 decay) {
        if (decay.raw < 1000) decay.raw = 1000;
        if (decay.raw > 32700) decay.raw = 32700;
        decay_coeff_q15_ = decay.raw;
    }

    void set_punch(math::FixedQ15 punch) {
        if (punch.raw < 0) punch.raw = 0;
        pitch_amount_q15_ = punch.raw;
    }

    void trigger(math::FixedQ15 velocity) {
        if (velocity.raw <= 0) return;
        // Inject narrow impulse into resonator
        y1_ += velocity.raw;
        pitch_env_ = velocity.raw;
        click_counter_ = 16; // 16 samples click transient
    }

    math::FixedQ15 process() {
        // Step pitch dive envelope
        pitch_env_ = (static_cast<int64_t>(pitch_env_) * pitch_decay_q15_) >> 15;

        // Dynamic frequency modulation: base frequency shifted up by pitch envelope
        int32_t pitch_mod = (static_cast<int64_t>(pitch_env_) * pitch_amount_q15_) >> 17;
        int32_t dynamic_freq_coeff = base_freq_coeff_q14_ - pitch_mod;

        // Resonator filter step: y0 = 2*r*cos(w)*y1 - r^2*y2
        int32_t term1 = (static_cast<int64_t>(y1_) * dynamic_freq_coeff) >> 14;
        int32_t term2 = (static_cast<int64_t>(y2_) * decay_coeff_q15_) >> 15;

        int32_t y0 = term1 - term2;

        // Resonator decay
        y0 = (static_cast<int64_t>(y0) * decay_coeff_q15_) >> 15;

        y2_ = y1_;
        y1_ = y0;

        // Add transient click
        int32_t click = 0;
        if (click_counter_ > 0) {
            click = (click_counter_ & 1) ? 10000 : -10000;
            click_counter_--;
        }

        int32_t out = y0 + click;
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t y1_{0};
    int32_t y2_{0};
    int32_t pitch_env_{0};
    int16_t decay_coeff_q15_{32000};
    int16_t pitch_decay_q15_{28000};
    int16_t base_freq_coeff_q14_{8000};
    int16_t pitch_amount_q15_{16384};
    size_t click_counter_{0};
};

} // namespace aether::embedded
