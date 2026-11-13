#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Comparator & Window Windowing Module
// Compares input CV or audio signal against Upper and Lower thresholds:
// 1. High Gate: In > Upper
// 2. Low Gate: In < Lower
// 3. Inside Window: Lower <= In <= Upper
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackWindowComparator {
public:
    EurorackWindowComparator() {
        reset();
    }

    void reset() noexcept {
        upper_threshold_mv_ = 2500; // +2.5V default
        lower_threshold_mv_ = 1000; // +1.0V default
        hysteresis_mv_ = 50;
        high_gate_ = false;
        low_gate_ = false;
        inside_window_ = false;
    }

    void set_window(int16_t lower_mv, int16_t upper_mv, int16_t hysteresis_mv = 50) noexcept {
        if (lower_mv > upper_mv) {
            int16_t tmp = lower_mv;
            lower_mv = upper_mv;
            upper_mv = tmp;
        }
        lower_threshold_mv_ = lower_mv;
        upper_threshold_mv_ = upper_mv;
        hysteresis_mv_ = (hysteresis_mv < 0) ? 0 : hysteresis_mv;
    }

    struct Output {
        bool above_upper{false};
        bool below_lower{false};
        bool inside_window{false};
    };

    Output process(int16_t input_mv) noexcept {
        // Upper comparator with hysteresis
        if (input_mv > (upper_threshold_mv_ + hysteresis_mv_)) {
            high_gate_ = true;
        } else if (input_mv < (upper_threshold_mv_ - hysteresis_mv_)) {
            high_gate_ = false;
        }

        // Lower comparator with hysteresis
        if (input_mv < (lower_threshold_mv_ - hysteresis_mv_)) {
            low_gate_ = true;
        } else if (input_mv > (lower_threshold_mv_ + hysteresis_mv_)) {
            low_gate_ = false;
        }

        inside_window_ = (!high_gate_ && !low_gate_);

        return {high_gate_, low_gate_, inside_window_};
    }

    [[nodiscard]] bool is_inside() const noexcept {
        return inside_window_;
    }

private:
    int16_t upper_threshold_mv_{2500};
    int16_t lower_threshold_mv_{1000};
    int16_t hysteresis_mv_{50};
    bool high_gate_{false};
    bool low_gate_{false};
    bool inside_window_{false};
};

} // namespace aether::embedded
