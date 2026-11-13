#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 16-step CV Burst Generator & Ratchet Core.
 * Models specialized rhythmic bursting Eurorack modules (e.g. Qu-Bit Pulse, Make Noise Rene ratchet).
 * Features:
 * - Trigger / Gate input with Schmitt trigger conditioning.
 * - Burst count: 1 to 32 rapid pulses per trigger.
 * - Ratchet density: exponential decay, acceleration, or linear spacing.
 * - Pulse width duty cycle adjustment.
 * - End-of-burst (EOB) trigger output pulse.
 */
class EurorackBurstGenerator {
public:
    enum class SpacingMode {
        Linear = 0,
        Accelerating = 1,
        Decelerating = 2
    };

    EurorackBurstGenerator() {
        reset();
    }

    void reset() {
        active_ = false;
        trig_last_ = false;
        burst_count_ = 4;
        pulses_fired_ = 0;
        spacing_mode_ = SpacingMode::Linear;
        base_interval_samples_ = 1000;
        current_interval_samples_ = 1000;
        pulse_width_samples_ = 200;
        sample_counter_ = 0;
        eob_counter_ = 0;
    }

    void configure(size_t burst_count, size_t base_interval_samples, SpacingMode mode, size_t pulse_width_samples) {
        if (burst_count == 0) burst_count = 1;
        if (burst_count > 32) burst_count = 32;
        burst_count_ = burst_count;

        if (base_interval_samples < 50) base_interval_samples = 50;
        base_interval_samples_ = base_interval_samples;

        spacing_mode_ = mode;

        if (pulse_width_samples < 10) pulse_width_samples = 10;
        if (pulse_width_samples >= base_interval_samples) pulse_width_samples = base_interval_samples / 2;
        pulse_width_samples_ = pulse_width_samples;
    }

    struct Output {
        math::FixedQ15 burst_gate;
        math::FixedQ15 eob_pulse;
        bool is_active;
    };

    Output process(math::FixedQ15 trig_in) {
        // Schmitt trigger input detection
        bool trig_high = trig_in.raw > 2000;
        bool trig_rising = trig_high && !trig_last_;
        trig_last_ = trig_high;

        if (trig_rising) {
            // Trigger a fresh burst sequence
            active_ = true;
            pulses_fired_ = 0;
            sample_counter_ = 0;
            current_interval_samples_ = compute_interval(0);
        }

        int16_t gate_out = 0;
        int16_t eob_out = 0;

        if (eob_counter_ > 0) {
            eob_out = 32767;
            eob_counter_--;
        }

        if (active_) {
            // Is current sample in active gate high period?
            if (sample_counter_ < pulse_width_samples_) {
                gate_out = 32767;
            }

            sample_counter_++;
            if (sample_counter_ >= current_interval_samples_) {
                sample_counter_ = 0;
                pulses_fired_++;

                if (pulses_fired_ >= burst_count_) {
                    active_ = false;
                    eob_counter_ = 48; // 1 ms EOB pulse at 48kHz
                } else {
                    current_interval_samples_ = compute_interval(pulses_fired_);
                }
            }
        }

        Output out;
        out.burst_gate = math::FixedQ15::from_raw(gate_out);
        out.eob_pulse = math::FixedQ15::from_raw(eob_out);
        out.is_active = active_;
        return out;
    }

private:
    bool active_{false};
    bool trig_last_{false};
    size_t burst_count_{4};
    size_t pulses_fired_{0};
    SpacingMode spacing_mode_{SpacingMode::Linear};
    size_t base_interval_samples_{1000};
    size_t current_interval_samples_{1000};
    size_t pulse_width_samples_{200};
    size_t sample_counter_{0};
    size_t eob_counter_{0};

    size_t compute_interval(size_t pulse_idx) const {
        if (spacing_mode_ == SpacingMode::Linear) {
            return base_interval_samples_;
        } else if (spacing_mode_ == SpacingMode::Accelerating) {
            // Pulses get progressively closer together
            size_t factor = (burst_count_ - pulse_idx);
            size_t interval = (base_interval_samples_ * factor) / burst_count_;
            return (interval > (pulse_width_samples_ + 10)) ? interval : (pulse_width_samples_ + 10);
        } else {
            // Decelerating: pulses slow down
            size_t factor = pulse_idx + 1;
            size_t interval = (base_interval_samples_ * factor) / 2;
            return (interval > (pulse_width_samples_ + 10)) ? interval : (pulse_width_samples_ + 10);
        }
    }
};

} // namespace aether::embedded
