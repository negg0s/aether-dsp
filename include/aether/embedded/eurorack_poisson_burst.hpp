#pragma once

#include <cstdint>
#include <cstddef>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack Stochastic Poisson Bernoulli Burst Engine.
 * Generates probabilistic gate triggers and random voltage steps using an LFSR
 * pseudo-random generator coupled to a Poisson arrival interval distribution.
 *
 * Q15 fixed-point arithmetic, zero heap allocation.
 */
class EurorackPoissonBurst {
public:
    EurorackPoissonBurst()
        : lfsr_(0xACE1u),
          density_q15_(16384),   // ~0.5 average density
          gate_length_samples_(48), // ~1 ms at 48 kHz
          counter_(0),
          gate_timer_(0),
          random_cv_mV_(0) {}

    // Set trigger density (0 to 32767)
    void set_density(int16_t density_q15) {
        density_q15_ = (density_q15 < 1) ? 1 : density_q15;
    }

    // Set gate length in samples (1 to 480)
    void set_gate_length(uint16_t length) {
        gate_length_samples_ = (length < 1) ? 1 : ((length > 480) ? 480 : length);
    }

    struct Output {
        bool gate;
        int16_t cv_mV; // Random stepped CV from 0 to 5000 mV (0 to 5V)
    };

    // Process single sample tick
    Output process() {
        Output out;

        // Advance LFSR
        uint16_t lsb = lfsr_ & 1u;
        lfsr_ >>= 1;
        if (lsb) {
            lfsr_ ^= 0xB400u;
        }

        // Poisson-like arrival threshold test
        uint16_t rand_val = lfsr_ & 0x7FFFu; // 0 to 32767
        // Trigger condition based on density
        if (rand_val < (static_cast<uint32_t>(density_q15_) >> 4)) {
            // Trigger fired
            gate_timer_ = gate_length_samples_;
            // Update stepped random CV: 0 to 5000 mV
            random_cv_mV_ = static_cast<int16_t>((static_cast<int32_t>(lfsr_ & 0x7FFFu) * 5000) >> 15);
        }

        if (gate_timer_ > 0) {
            gate_timer_--;
            out.gate = true;
        } else {
            out.gate = false;
        }

        out.cv_mV = random_cv_mV_;
        return out;
    }

    void reset() {
        lfsr_ = 0xACE1u;
        gate_timer_ = 0;
        random_cv_mV_ = 0;
    }

private:
    uint16_t lfsr_;
    int16_t density_q15_;
    uint16_t gate_length_samples_;
    uint32_t counter_;
    uint16_t gate_timer_;
    int16_t random_cv_mV_;
};

} // namespace embedded
} // namespace aether
