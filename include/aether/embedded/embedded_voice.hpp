#pragma once

#include "../math/fixed_point.hpp"
#include "../math/fixed_oscillator.hpp"
#include "../math/fixed_biquad.hpp"
#include "../math/fixed_envelope.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Autonomous Zero-Heap Single-Voice Embedded Synth Core.
 * Combines FixedSineOscillator, FixedBiquadFilter, and FixedADSREnvelope
 * for microcontrollers (Daisy Seed, Cortex-M4/M7, RP2040) running at < 1% CPU.
 */
class EmbeddedVoice {
public:
    EmbeddedVoice() noexcept {
        init();
    }

    void init() noexcept {
        osc_.set_normalized_frequency(440.0f / 48000.0f);
        filter_.configure_lowpass(1200.0f / 48000.0f, 0.707f);
        env_.configure(480, 4800, math::FixedQ15::from_float(0.6f), 9600);
    }

    void note_on(float freq_hz, float sample_rate = 48000.0f) noexcept {
        osc_.set_normalized_frequency(freq_hz / sample_rate);
        env_.gate(true);
    }

    void note_off() noexcept {
        env_.gate(false);
    }

    [[nodiscard]] inline math::FixedQ15 process() noexcept {
        const math::FixedQ15 raw = osc_.step();
        const math::FixedQ15 env = env_.process();
        const math::FixedQ15 shaped = raw * env;
        return filter_.process(shaped);
    }

    [[nodiscard]] constexpr bool is_active() const noexcept {
        return env_.is_active();
    }

private:
    math::FixedSineOscillator osc_;
    math::FixedBiquadFilter filter_;
    math::FixedADSREnvelope env_;
};

} // namespace aether::embedded
