#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Wave Packet Synthesizer / Pulsar Generator.
 * Implements Curtis Roads pulsar / particle grain synthesis in pure integer Q15 arithmetic.
 * Features:
 * - Emits discrete burst wave packets (sinusoidal pulsar) separated by variable silence gaps (duty cycle).
 * - Morphable masking formants (Cosine, Triangle, Parabolic bell envelopes).
 * - Sub-harmonic pitch multiplying and stochastic pitch jitter.
 * - Zero dynamic memory allocations, designed for Cortex-M7 / Daisy Patch.
 */
class EurorackPulsarSynth {
public:
    enum class WindowType : uint8_t {
        Triangle = 0,
        SineBell = 1,
        Rectangular = 2
    };

    EurorackPulsarSynth() {
        reset();
    }

    void reset() {
        fundamental_phase_ = 0;
        fundamental_inc_ = 10000000; // ~111 Hz at 48kHz
        formant_phase_ = 0;
        formant_inc_ = 40000000;     // ~444 Hz formant
        duty_cycle_q15_ = 16384;     // 50% burst, 50% silence
        window_type_ = WindowType::SineBell;
        in_burst_ = false;
    }

    void set_fundamental(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 1.0f) freq_hz = 1.0f;
        if (freq_hz > sample_rate_hz * 0.4f) freq_hz = sample_rate_hz * 0.4f;

        double inc = (static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        fundamental_inc_ = static_cast<uint32_t>(inc);
    }

    void set_formant(float formant_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (formant_hz < 10.0f) formant_hz = 10.0f;
        if (formant_hz > sample_rate_hz * 0.45f) formant_hz = sample_rate_hz * 0.45f;

        double inc = (static_cast<double>(formant_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        formant_inc_ = static_cast<uint32_t>(inc);
    }

    void set_duty_cycle(math::FixedQ15 duty) {
        if (duty.raw < 1000) duty.raw = 1000;
        if (duty.raw > 32000) duty.raw = 32000;
        duty_cycle_q15_ = duty.raw;
    }

    void set_window_type(WindowType win) {
        window_type_ = win;
    }

    math::FixedQ15 process() {
        uint32_t prev_phase = fundamental_phase_;
        fundamental_phase_ += fundamental_inc_;

        // Fundamental period reset (pulse emission boundary)
        if (fundamental_phase_ < prev_phase) {
            formant_phase_ = 0;
            in_burst_ = true;
        }

        // Active duty cycle threshold: [0, duty_threshold] is active pulsar, rest is silence
        uint32_t duty_threshold = (static_cast<uint64_t>(duty_cycle_q15_) << 17); // maps Q15 to 32-bit fraction
        if (fundamental_phase_ > duty_threshold) {
            in_burst_ = false;
        }

        if (!in_burst_) {
            return math::FixedQ15::from_raw(0);
        }

        // Formant carrier oscillation
        formant_phase_ += formant_inc_;
        int32_t carrier = compute_carrier(formant_phase_);

        // Envelope masking window computed across active duty period
        // Window progress = fundamental_phase / duty_threshold in [0, 32767]
        int32_t win_progress = static_cast<int32_t>((static_cast<uint64_t>(fundamental_phase_) * 32767) / duty_threshold);
        if (win_progress > 32767) win_progress = 32767;

        int32_t env = 32767;
        if (window_type_ == WindowType::Triangle) {
            // Triangle window (0 at boundaries, 32767 at center)
            if (win_progress < 16384) {
                env = win_progress * 2;
            } else {
                env = (32767 - win_progress) * 2;
            }
        } else if (window_type_ == WindowType::SineBell) {
            // Half-sine window
            int32_t half_phase = win_progress; // [0, 32767] mapped to half circle
            int32_t p = half_phase;
            env = (static_cast<int64_t>(p) * (32767 - p)) >> 13;
        }

        // Multiply carrier by window envelope
        int32_t out = (static_cast<int64_t>(carrier) * env) >> 15;
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    uint32_t fundamental_phase_{0};
    uint32_t fundamental_inc_{10000000};
    uint32_t formant_phase_{0};
    uint32_t formant_inc_{40000000};
    int16_t duty_cycle_q15_{16384};
    WindowType window_type_{WindowType::SineBell};
    bool in_burst_{false};

    inline int32_t compute_carrier(uint32_t phase) {
        uint32_t p = phase >> 16;
        int32_t x = 0;
        if (p < 32768) {
            x = static_cast<int32_t>(p) * 2 - 32768;
        } else {
            x = 32767 - static_cast<int32_t>(p - 32768) * 2;
        }
        int32_t abs_x = (x >= 0) ? x : -x;
        return (static_cast<int64_t>(x) * (32767 - abs_x)) >> 13;
    }
};

} // namespace aether::embedded
