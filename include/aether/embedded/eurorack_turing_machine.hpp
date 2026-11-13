#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Stochastic Turing Machine Shift Register (Music Thing Modular Turing Machine).
 * Generates looped pseudo-random control voltages, pitch sequences, and rhythmic gate patterns.
 * Features:
 * - 16-bit circulating shift register with probabilistic bit-flipping on each clock tick.
 * - Lock/Write probability knob [0, 32767]:
 *     0 = Completely locked loop (repeats 16 steps infinitely)
 *     16384 = Maximum chaos / 50% random chance of flipping bits
 *     32767 = Inverted loop (repeats 16 steps inverted each cycle)
 * - 8-bit DAC output converting byte slice into CV (0 to 4095 mV).
 * - Pulse outputs: Clock passthrough, 2+4 gate outputs, and dynamic rhythm triggers.
 */
class EurorackTuringMachine {
public:
    EurorackTuringMachine() {
        reset();
    }

    void reset() {
        shift_reg_ = 0xAA55u; // 1010101001010101 pattern
        prob_q15_ = 0;        // Locked loop by default
        clk_last_ = false;
        length_ = 16;
        lfsr_state_ = 0x5821u;
    }

    void set_probability(math::FixedQ15 prob) {
        if (prob.raw < 0) prob.raw = 0;
        prob_q15_ = prob.raw;
    }

    void set_length(size_t len) {
        if (len < 2) len = 2;
        if (len > 16) len = 16;
        length_ = len;
    }

    struct Output {
        int16_t cv_mV;            // Quantized/continuous pitch CV in mV (0 to 4000 mV)
        math::FixedQ15 gate_out1; // Trigger on bit 0
        math::FixedQ15 gate_out2; // Trigger on bit 1 & 2
        math::FixedQ15 cv_q15;    // Q15 normalized CV output
    };

    Output process(math::FixedQ15 clock_in) {
        // Schmitt trigger clock detection
        bool clk_high = clock_in.raw > 2000;
        bool clk_rising = clk_high && !clk_last_;
        clk_last_ = clk_high;

        if (clk_rising) {
            // Read MSB at current length
            uint16_t msb = (shift_reg_ >> (length_ - 1)) & 1u;

            // Probability test for bit flipping
            // If random roll < prob_q15_, flip bit. Else preserve bit.
            uint16_t rnd = step_lfsr() & 0x7FFFu; // [0, 32767]
            uint16_t new_bit = msb;
            if (rnd < prob_q15_) {
                new_bit = msb ^ 1u;
            }

            // Shift register left and insert new LSB
            shift_reg_ = ((shift_reg_ << 1) | new_bit) & 0xFFFFu;
        }

        // 8-bit DAC output taken from lowest 8 bits of shift register
        uint16_t dac_val = shift_reg_ & 0x00FFu; // [0, 255]
        int32_t cv_mV = (static_cast<int32_t>(dac_val) * 4000) / 255; // 0 to 4000 mV (4 octaves)
        int32_t cv_q15 = (static_cast<int32_t>(dac_val) * 32767) / 255;

        // Pulse gates
        bool g1 = (shift_reg_ & 0x0001u) != 0;
        bool g2 = ((shift_reg_ & 0x0002u) != 0) && ((shift_reg_ & 0x0004u) != 0);

        Output out;
        out.cv_mV = static_cast<int16_t>(cv_mV);
        out.gate_out1 = math::FixedQ15::from_raw(g1 ? 32767 : 0);
        out.gate_out2 = math::FixedQ15::from_raw(g2 ? 32767 : 0);
        out.cv_q15 = math::FixedQ15::from_raw(static_cast<int16_t>(cv_q15));

        return out;
    }

private:
    uint16_t shift_reg_{0xAA55u};
    int16_t prob_q15_{0};
    bool clk_last_{false};
    size_t length_{16};
    uint16_t lfsr_state_{0x5821u};

    inline uint16_t step_lfsr() {
        uint16_t bit = ((lfsr_state_ >> 0) ^ (lfsr_state_ >> 2) ^ (lfsr_state_ >> 3) ^ (lfsr_state_ >> 5)) & 1u;
        lfsr_state_ = (lfsr_state_ >> 1) | (bit << 15);
        return lfsr_state_;
    }
};

} // namespace aether::embedded
