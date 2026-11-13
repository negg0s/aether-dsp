#pragma once

#include <cstdint>
#include <cstddef>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Sample and Hold / Track and Hold module
// Models:
// 1. Classic Sample & Hold: captures input CV on rising clock edge and holds steady.
// 2. Track & Hold: passes input CV through while gate is HIGH; freezes CV when gate is LOW.
// 3. Analog droop simulation: tiny leakage over time (capacitor self-discharge).
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackSampleAndHold {
public:
    EurorackSampleAndHold() {
        reset();
    }

    void reset() noexcept {
        held_cv_mv_ = 0;
        prev_gate_ = false;
        droop_rate_mv_ = 0;
        droop_counter_ = 0;
    }

    // Set droop rate (millivolts lost every N samples due to analog capacitor leakage)
    void set_droop(uint16_t droop_rate_mv, uint32_t samples_per_leak = 1000) noexcept {
        droop_rate_mv_ = droop_rate_mv;
        samples_per_leak_ = (samples_per_leak == 0) ? 1000 : samples_per_leak;
    }

    // Process Sample & Hold (triggers on rising edge of clock_in)
    int16_t process_sample_and_hold(int16_t cv_in_mv, bool clock_in) noexcept {
        if (clock_in && !prev_gate_) {
            held_cv_mv_ = cv_in_mv;
            droop_counter_ = 0;
        } else if (droop_rate_mv_ > 0) {
            droop_counter_++;
            if (droop_counter_ >= samples_per_leak_) {
                droop_counter_ = 0;
                if (held_cv_mv_ > 0) {
                    held_cv_mv_ -= droop_rate_mv_;
                    if (held_cv_mv_ < 0) held_cv_mv_ = 0;
                } else if (held_cv_mv_ < 0) {
                    held_cv_mv_ += droop_rate_mv_;
                    if (held_cv_mv_ > 0) held_cv_mv_ = 0;
                }
            }
        }
        prev_gate_ = clock_in;
        return held_cv_mv_;
    }

    // Process Track & Hold: when gate_in is HIGH, tracks cv_in; freezes when LOW
    int16_t process_track_and_hold(int16_t cv_in_mv, bool gate_in) noexcept {
        if (gate_in) {
            held_cv_mv_ = cv_in_mv;
            droop_counter_ = 0;
        } else if (droop_rate_mv_ > 0) {
            droop_counter_++;
            if (droop_counter_ >= samples_per_leak_) {
                droop_counter_ = 0;
                if (held_cv_mv_ > 0) {
                    held_cv_mv_ -= droop_rate_mv_;
                    if (held_cv_mv_ < 0) held_cv_mv_ = 0;
                } else if (held_cv_mv_ < 0) {
                    held_cv_mv_ += droop_rate_mv_;
                    if (held_cv_mv_ > 0) held_cv_mv_ = 0;
                }
            }
        }
        return held_cv_mv_;
    }

    [[nodiscard]] int16_t held_value() const noexcept {
        return held_cv_mv_;
    }

private:
    int16_t held_cv_mv_{0};
    bool prev_gate_{false};
    uint16_t droop_rate_mv_{0};
    uint32_t samples_per_leak_{1000};
    uint32_t droop_counter_{0};
};

} // namespace aether::embedded
