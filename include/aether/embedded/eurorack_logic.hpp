#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Quad Logic & Gate Delay module
// Provides:
// 1. Boolean logic combinations (AND, OR, XOR, NAND, NOR) of two incoming gate signals.
// 2. Flip-Flop T-gate divider (/2, /4, /8).
// 3. Sub-sample gate pulse delay line with burst generator.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackLogicCore {
public:
    enum class LogicOp : uint8_t {
        AND = 0,
        OR,
        XOR,
        NAND,
        NOR
    };

    EurorackLogicCore() {
        reset();
    }

    void reset() noexcept {
        ff_div2_ = false;
        ff_div4_ = false;
        ff_div8_ = false;
        div_counter_ = 0;
        prev_gate_a_ = false;
        delay_write_ = 0;
        gate_delay_line_.fill(false);
    }

    [[nodiscard]] static bool evaluate_logic(bool a, bool b, LogicOp op) noexcept {
        switch (op) {
            case LogicOp::AND:  return a && b;
            case LogicOp::OR:   return a || b;
            case LogicOp::XOR:  return a != b;
            case LogicOp::NAND: return !(a && b);
            case LogicOp::NOR:  return !(a || b);
            default:            return false;
        }
    }

    struct ClockDividerOutput {
        bool div2{false};
        bool div4{false};
        bool div8{false};
    };

    ClockDividerOutput clock_divider(bool gate_in) noexcept {
        // Trigger on rising edge
        if (gate_in && !prev_gate_a_) {
            div_counter_++;
            ff_div2_ = (div_counter_ & 1) != 0;
            ff_div4_ = (div_counter_ & 2) != 0;
            ff_div8_ = (div_counter_ & 4) != 0;
        }
        prev_gate_a_ = gate_in;
        return {ff_div2_, ff_div4_, ff_div8_};
    }

    // Gate delay line up to 256 clock ticks
    bool process_gate_delay(bool gate_in, uint8_t delay_ticks) noexcept {
        size_t read_idx = (delay_write_ + 256 - delay_ticks) & 0xFF;
        bool out_gate = gate_delay_line_[read_idx];

        gate_delay_line_[delay_write_] = gate_in;
        delay_write_ = (delay_write_ + 1) & 0xFF;

        return out_gate;
    }

private:
    bool ff_div2_{false};
    bool ff_div4_{false};
    bool ff_div8_{false};
    uint8_t div_counter_{0};
    bool prev_gate_a_{false};

    size_t delay_write_{0};
    std::array<bool, 256> gate_delay_line_{};
};

} // namespace aether::embedded
