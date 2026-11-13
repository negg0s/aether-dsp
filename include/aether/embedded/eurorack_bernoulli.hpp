#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Bernoulli Gate & Stochastic Burst Generator
// Generates probabilistic routing of incoming triggers to Output A or Output B
// with deterministic LFSR Galois PRNG and configurable burst cluster trains.
// Zero-heap, zero-allocation bare-metal MCU implementation (similar to Mutable Instruments Branches).
class EurorackBernoulliGate {
public:
    EurorackBernoulliGate() {
        reset();
    }

    void reset() noexcept {
        probability_a_pct_ = 50; // default 50% to A, 50% to B
        burst_count_ = 1;
        burst_counter_ = 0;
        burst_div_ = 2;
        subtick_ = 0;
        lfsr_state_ = 0xACE1ACE1;
        route_to_a_ = true;
        out_a_ = false;
        out_b_ = false;
    }

    // Set probability for Output A: 0% to 100%
    void set_probability(uint8_t prob_a_pct) noexcept {
        probability_a_pct_ = (prob_a_pct > 100) ? 100 : prob_a_pct;
    }

    // Set burst generation: on each trigger event, emit N micro-pulses
    void set_burst(uint8_t count, uint8_t tick_spacing = 2) noexcept {
        burst_count_ = (count == 0) ? 1 : count;
        burst_div_ = (tick_spacing == 0) ? 1 : tick_spacing;
    }

    struct Output {
        bool gate_a{false};
        bool gate_b{false};
    };

    Output process(bool trigger_in) noexcept {
        if (trigger_in && !prev_trigger_) {
            // Roll PRNG for routing
            uint32_t roll = next_random() % 100;
            route_to_a_ = (roll < probability_a_pct_);
            burst_counter_ = burst_count_;
            subtick_ = 0;
        }
        prev_trigger_ = trigger_in;

        // Process burst pulses
        if (burst_counter_ > 0) {
            if ((subtick_ % burst_div_) == 0) {
                out_a_ = route_to_a_;
                out_b_ = !route_to_a_;
                burst_counter_--;
            } else {
                out_a_ = false;
                out_b_ = false;
            }
            subtick_++;
        } else {
            out_a_ = false;
            out_b_ = false;
        }

        return {out_a_, out_b_};
    }

    [[nodiscard]] bool last_routed_to_a() const noexcept {
        return route_to_a_;
    }

private:
    uint32_t next_random() noexcept {
        // 32-bit Galois LFSR
        lfsr_state_ = (lfsr_state_ >> 1) ^ (-(lfsr_state_ & 1u) & 0xD0000001u);
        return lfsr_state_;
    }

    uint8_t probability_a_pct_{50};
    uint8_t burst_count_{1};
    uint8_t burst_counter_{0};
    uint8_t burst_div_{2};
    uint8_t subtick_{0};
    bool prev_trigger_{false};
    bool route_to_a_{true};
    bool out_a_{false};
    bool out_b_{false};
    uint32_t lfsr_state_{0xACE1ACE1};
};

} // namespace aether::embedded
