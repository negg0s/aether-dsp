#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Slope / Envelope Generator
// Multi-segment ADSR envelope with shape curvature morphing (Logarithmic -> Linear -> Exponential)
// and Retrigger / Loop modes in fixed-point Q15.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackSlopeGenerator {
public:
    enum class State : uint8_t {
        Idle = 0,
        Attack,
        Decay,
        Sustain,
        Release
    };

    enum class Shape : uint8_t {
        Linear = 0,
        Exponential,
        Logarithmic
    };

    EurorackSlopeGenerator() {
        reset();
    }

    void reset() noexcept {
        state_ = State::Idle;
        level_ = 0;
        attack_rate_ = 500;
        decay_rate_ = 200;
        sustain_level_ = 20000; // ~0.61 in Q15
        release_rate_ = 100;
        shape_ = Shape::Linear;
        loop_mode_ = false;
        eor_gate_ = false;
        eoc_gate_ = false;
    }

    void configure(uint16_t attack, uint16_t decay, int16_t sustain, uint16_t release, Shape shape = Shape::Linear) noexcept {
        attack_rate_ = (attack == 0) ? 1 : attack;
        decay_rate_ = (decay == 0) ? 1 : decay;
        sustain_level_ = (sustain < 0) ? 0 : sustain;
        release_rate_ = (release == 0) ? 1 : release;
        shape_ = shape;
    }

    void set_loop(bool loop) noexcept {
        loop_mode_ = loop;
    }

    struct Output {
        math::FixedQ15 cv_out{0};
        bool end_of_rise{false};
        bool end_of_cycle{false};
    };

    Output process(bool gate_in) noexcept {
        eor_gate_ = false;
        eoc_gate_ = false;

        // Gate on / retrigger
        if (gate_in && (state_ == State::Idle || state_ == State::Release)) {
            state_ = State::Attack;
        } else if (!gate_in && state_ != State::Idle && state_ != State::Release) {
            state_ = State::Release;
        }

        switch (state_) {
            case State::Attack:
                level_ += attack_rate_;
                if (level_ >= 32767) {
                    level_ = 32767;
                    state_ = State::Decay;
                    eor_gate_ = true;
                }
                break;

            case State::Decay:
                level_ -= decay_rate_;
                if (level_ <= sustain_level_) {
                    level_ = sustain_level_;
                    state_ = State::Sustain;
                }
                break;

            case State::Sustain:
                level_ = sustain_level_;
                break;

            case State::Release:
                level_ -= release_rate_;
                if (level_ <= 0) {
                    level_ = 0;
                    state_ = loop_mode_ ? State::Attack : State::Idle;
                    eoc_gate_ = true;
                }
                break;

            case State::Idle:
                level_ = 0;
                if (loop_mode_) {
                    state_ = State::Attack;
                }
                break;
        }

        // Apply curve shape shaping
        int64_t shaped_level = level_;
        if (shape_ == Shape::Exponential) {
            // Exp: y = level^2 / 32768
            shaped_level = (shaped_level * shaped_level) >> 15;
        } else if (shape_ == Shape::Logarithmic) {
            // Log: y = 2*level - level^2 / 32768
            int64_t sq = (shaped_level * shaped_level) >> 15;
            shaped_level = (2 * shaped_level) - sq;
        }

        if (shaped_level > 32767) shaped_level = 32767;
        if (shaped_level < 0) shaped_level = 0;

        return {math::FixedQ15(static_cast<int16_t>(shaped_level)), eor_gate_, eoc_gate_};
    }

    [[nodiscard]] State current_state() const noexcept {
        return state_;
    }

private:
    State state_{State::Idle};
    int32_t level_{0};
    uint16_t attack_rate_{500};
    uint16_t decay_rate_{200};
    int16_t sustain_level_{20000};
    uint16_t release_rate_{100};
    Shape shape_{Shape::Linear};
    bool loop_mode_{false};
    bool eor_gate_{false};
    bool eoc_gate_{false};
};

} // namespace aether::embedded
