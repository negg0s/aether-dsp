#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Clock Multiplier & Phase-Locked Loop
// Locks onto incoming analog or digital clock pulses, tracks BPM changes with jitter damping,
// and synthesizes integer multiplied clock rates (x1, x2, x3, x4, x6, x8, x12, x16)
// Zero-heap, zero-allocation bare-metal MCU PLL implementation (similar to 4ms SCM).
class EurorackClockMultiplier {
public:
    EurorackClockMultiplier() {
        reset();
    }

    void reset() noexcept {
        period_samples_ = 48000; // 1 Hz default (60 BPM)
        counter_samples_ = 0;
        multiplier_ = 2; // x2 default
        sub_counter_ = 0;
        sub_period_ = 24000;
        prev_clock_ = false;
        pll_locked_ = false;
        gate_out_ = false;
        pulse_width_samples_ = 48; // 1 ms trigger pulse at 48kHz
    }

    void set_multiplier(uint8_t mult) noexcept {
        multiplier_ = (mult == 0) ? 1 : mult;
        recalculate_sub_period();
    }

    struct Output {
        bool clock_out{false};
        bool pll_locked{false};
        uint32_t period_samples{0};
    };

    Output process(bool clock_in) noexcept {
        bool pulse_fired = false;

        // Detect rising edge of incoming master clock
        if (clock_in && !prev_clock_) {
            if (counter_samples_ > 480) { // debounce minimum period (100 Hz max clock)
                // Filter period with 1-pole EMA low-pass for PLL jitter damping:
                // smoothed_period = (3 * old + new) / 4
                if (pll_locked_) {
                    period_samples_ = (3 * period_samples_ + counter_samples_) >> 2;
                } else {
                    period_samples_ = counter_samples_;
                    pll_locked_ = true;
                }
                recalculate_sub_period();
            }
            counter_samples_ = 0;
            sub_counter_ = 0;
            pulse_fired = true;
        } else {
            counter_samples_++;
            sub_counter_++;

            // Multiply: check if sub-counter hit multiplied period
            if (sub_counter_ >= sub_period_) {
                sub_counter_ = 0;
                pulse_fired = true;
            }
        }
        prev_clock_ = clock_in;

        if (pulse_fired) {
            pulse_duration_left_ = pulse_width_samples_;
        }

        if (pulse_duration_left_ > 0) {
            gate_out_ = true;
            pulse_duration_left_--;
        } else {
            gate_out_ = false;
        }

        return {gate_out_, pll_locked_, period_samples_};
    }

    [[nodiscard]] bool is_locked() const noexcept {
        return pll_locked_;
    }

private:
    void recalculate_sub_period() noexcept {
        if (multiplier_ > 0) {
            sub_period_ = period_samples_ / multiplier_;
            if (sub_period_ < 1) sub_period_ = 1;
        }
    }

    uint32_t period_samples_{48000};
    uint32_t counter_samples_{0};
    uint8_t multiplier_{2};
    uint32_t sub_counter_{0};
    uint32_t sub_period_{24000};
    uint32_t pulse_width_samples_{48};
    uint32_t pulse_duration_left_{0};
    bool prev_clock_{false};
    bool pll_locked_{false};
    bool gate_out_{false};
};

} // namespace aether::embedded
