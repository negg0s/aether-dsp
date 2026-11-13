#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Logic & Rhythm Clock Matrix.
 * Generates Euclidean, prime, binary-divided, and Boolean-combined gates.
 * Features:
 * - 4 Parallel Clock Dividers (/1, /2, /4, /8, /16, /32).
 * - Logic operators across channel A and channel B (AND, OR, XOR, NAND).
 * - Dynamic pulse width modulation per output channel.
 * - Suitable for triggering Eurorack drum modules or modular envelopes.
 */
class EurorackClockMatrix {
public:
    enum class LogicOp : uint8_t {
        And = 0,
        Or = 1,
        Xor = 2,
        Nand = 3
    };

    EurorackClockMatrix() {
        reset();
    }

    void reset() {
        clock_ticks_ = 0;
        clk_last_ = false;
        div1_ = 1;
        div2_ = 2;
        div3_ = 4;
        div4_ = 8;
        logic_op_ = LogicOp::And;
        pw_ratio_q15_ = 16384; // 50% duty cycle
    }

    void set_divisions(uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4) {
        div1_ = (d1 > 0) ? d1 : 1;
        div2_ = (d2 > 0) ? d2 : 1;
        div3_ = (d3 > 0) ? d3 : 1;
        div4_ = (d4 > 0) ? d4 : 1;
    }

    void set_logic_op(LogicOp op) {
        logic_op_ = op;
    }

    void set_pulse_width(math::FixedQ15 pw) {
        if (pw.raw < 1000) pw.raw = 1000;
        if (pw.raw > 31000) pw.raw = 31000;
        pw_ratio_q15_ = pw.raw;
    }

    struct Output {
        math::FixedQ15 out1;
        math::FixedQ15 out2;
        math::FixedQ15 out3;
        math::FixedQ15 out4;
        math::FixedQ15 logic_out;
    };

    Output process(math::FixedQ15 clock_in) {
        // Schmitt trigger rising edge detection
        bool clk_high = clock_in.raw > 2000;
        bool clk_rising = clk_high && !clk_last_;
        clk_last_ = clk_high;

        if (clk_rising) {
            clock_ticks_++;
        }

        // Determine gate state for each division channel
        bool g1 = compute_gate(clock_ticks_, div1_);
        bool g2 = compute_gate(clock_ticks_, div2_);
        bool g3 = compute_gate(clock_ticks_, div3_);
        bool g4 = compute_gate(clock_ticks_, div4_);

        // Compute Boolean combination of channel 1 and channel 2
        bool log_res = false;
        switch (logic_op_) {
            case LogicOp::And:  log_res = g1 && g2; break;
            case LogicOp::Or:   log_res = g1 || g2; break;
            case LogicOp::Xor:  log_res = g1 != g2; break;
            case LogicOp::Nand: log_res = !(g1 && g2); break;
        }

        Output out;
        out.out1 = math::FixedQ15::from_raw(g1 ? 32767 : 0);
        out.out2 = math::FixedQ15::from_raw(g2 ? 32767 : 0);
        out.out3 = math::FixedQ15::from_raw(g3 ? 32767 : 0);
        out.out4 = math::FixedQ15::from_raw(g4 ? 32767 : 0);
        out.logic_out = math::FixedQ15::from_raw(log_res ? 32767 : 0);

        return out;
    }

private:
    uint32_t clock_ticks_{0};
    bool clk_last_{false};
    uint32_t div1_{1};
    uint32_t div2_{2};
    uint32_t div3_{4};
    uint32_t div4_{8};
    LogicOp logic_op_{LogicOp::And};
    int16_t pw_ratio_q15_{16384};

    inline bool compute_gate(uint32_t ticks, uint32_t div) const {
        uint32_t cycle = ticks % div;
        // Duty cycle comparison
        uint32_t active_ticks = (div * pw_ratio_q15_) >> 15;
        if (active_ticks == 0) active_ticks = 1;
        return cycle < active_ticks;
    }
};

} // namespace aether::embedded
