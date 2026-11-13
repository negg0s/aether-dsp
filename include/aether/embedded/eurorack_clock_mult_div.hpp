#pragma once

#include <cstdint>
#include <cstddef>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack 8-Stage Digital Phase Accumulator Clock Divider & Multiplier.
 * Takes incoming clock pulses, measures inter-pulse period via 32-bit counter,
 * and generates 8 simultaneous metric divisions and multiplications:
 * /16, /8, /4, /2, x1, x2, x4, x8.
 *
 * All operations integer/fixed-point, zero heap allocation.
 */
class EurorackClockMultDiv {
public:
    EurorackClockMultDiv()
        : last_clock_state_(false),
          sample_counter_(0),
          measured_period_(48000), // Default 1 Hz at 48kHz
          phase_x1_(0) {
        reset();
    }

    struct Output {
        bool div16;
        bool div8;
        bool div4;
        bool div2;
        bool mult1;
        bool mult2;
        bool mult4;
        bool mult8;
    };

    // Process single sample tick with input clock trigger
    Output process(bool clock_in) {
        // Detect rising edge of input clock
        if (clock_in && !last_clock_state_) {
            if (sample_counter_ > 200) { // debounce ~4ms
                measured_period_ = sample_counter_;
            }
            sample_counter_ = 0;
            phase_x1_ = 0;
        } else {
            sample_counter_++;
        }
        last_clock_state_ = clock_in;

        // Phase accumulation for multiplications
        // Phase increment for x1: (65536 / measured_period_) in Q16
        uint32_t inc = 65536u / ((measured_period_ > 0) ? measured_period_ : 1u);
        phase_x1_ += inc;

        Output out;
        // Multipliers based on phase high bit / duty cycle (50% duty)
        out.mult1 = (phase_x1_ & 0x8000u) != 0;
        out.mult2 = ((phase_x1_ * 2u) & 0x8000u) != 0;
        out.mult4 = ((phase_x1_ * 4u) & 0x8000u) != 0;
        out.mult8 = ((phase_x1_ * 8u) & 0x8000u) != 0;

        // Dividers based on period counts
        uint32_t cycle_index = sample_counter_ / ((measured_period_ > 0) ? measured_period_ : 1u);
        out.div2  = ((cycle_index / 2u) % 2u) == 0;
        out.div4  = ((cycle_index / 4u) % 2u) == 0;
        out.div8  = ((cycle_index / 8u) % 2u) == 0;
        out.div16 = ((cycle_index / 16u) % 2u) == 0;

        return out;
    }

    void reset() {
        last_clock_state_ = false;
        sample_counter_ = 0;
        measured_period_ = 48000;
        phase_x1_ = 0;
    }

private:
    bool last_clock_state_;
    uint32_t sample_counter_;
    uint32_t measured_period_;
    uint32_t phase_x1_;
};

} // namespace embedded
} // namespace aether
