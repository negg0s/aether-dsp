#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Envelope Follower & Transient Trigger
// Extracts audio envelope amplitude via full-wave rectification and dual-time-constant
// peak-decay RC filtering, plus high-pass transient spike detector for drum triggers.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackEnvelopeFollower {
public:
    EurorackEnvelopeFollower() {
        reset();
    }

    void reset() noexcept {
        envelope_q15_ = math::FixedQ15(0);
        attack_coeff_ = math::FixedQ15::from_float(0.8f);  // fast attack
        release_coeff_ = math::FixedQ15::from_float(0.01f); // smooth release
        prev_env_ = 0;
        transient_threshold_ = 3000;
        gate_hold_ = 0;
        gate_out_ = false;
    }

    // Set attack/release time constants in Q15 (0.001 to 0.999)
    void configure(math::FixedQ15 attack, math::FixedQ15 release, int16_t transient_thresh = 3000) noexcept {
        attack_coeff_ = attack;
        release_coeff_ = release;
        transient_threshold_ = transient_thresh;
    }

    struct Output {
        math::FixedQ15 envelope{0};
        bool transient_trigger{false};
    };

    Output process(math::FixedQ15 audio_in) noexcept {
        // Full wave rectification: |audio_in|
        int32_t rectified = (audio_in.raw < 0) ? -audio_in.raw : audio_in.raw;
        if (rectified > 32767) rectified = 32767;

        int32_t current = envelope_q15_.raw;
        int32_t delta = rectified - current;

        // Peak detector with asymmetrical smoothing
        if (delta > 0) {
            // Attack phase
            int32_t step = (delta * attack_coeff_.raw) >> 15;
            current += step;
        } else {
            // Release phase
            int32_t step = (delta * release_coeff_.raw) >> 15;
            current += step;
        }

        if (current > 32767) current = 32767;
        if (current < 0) current = 0;
        envelope_q15_ = math::FixedQ15(static_cast<int16_t>(current));

        // High-pass transient detector (d(env)/dt)
        int32_t diff = current - prev_env_;
        prev_env_ = current;

        if (diff > transient_threshold_) {
            gate_hold_ = 24; // hold trigger pulse for 24 samples
        }

        if (gate_hold_ > 0) {
            gate_out_ = true;
            gate_hold_--;
        } else {
            gate_out_ = false;
        }

        return {envelope_q15_, gate_out_};
    }

    [[nodiscard]] math::FixedQ15 envelope() const noexcept {
        return envelope_q15_;
    }

private:
    math::FixedQ15 envelope_q15_{0};
    math::FixedQ15 attack_coeff_{math::FixedQ15::from_float(0.8f)};
    math::FixedQ15 release_coeff_{math::FixedQ15::from_float(0.01f)};
    int32_t prev_env_{0};
    int16_t transient_threshold_{3000};
    uint16_t gate_hold_{0};
    bool gate_out_{false};
};

} // namespace aether::embedded
