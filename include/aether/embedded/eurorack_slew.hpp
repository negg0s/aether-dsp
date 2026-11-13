#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Slew Limiter (Portamento / Glide)
// Models linear and exponential slew rates with independent Rise and Fall curves.
// In self-cycle mode functions as an analog LFO / function generator (similar to Make Noise Maths / Buchla 281).
// Zero-heap, zero-allocation implementation for bare-metal MCU.
class EurorackSlewLimiter {
public:
    enum class Curve : uint8_t {
        Linear = 0,
        Exponential
    };

    EurorackSlewLimiter() {
        reset();
    }

    void reset() noexcept {
        current_mv_ = 0;
        cycle_mode_ = false;
        end_of_rise_ = false;
        end_of_fall_ = false;
        rise_rate_mv_ = 50; // default 50 mV per sample
        fall_rate_mv_ = 50;
        curve_ = Curve::Linear;
    }

    void set_rates(int16_t rise_rate_mv, int16_t fall_rate_mv, Curve curve = Curve::Linear) noexcept {
        rise_rate_mv_ = (rise_rate_mv < 1) ? 1 : rise_rate_mv;
        fall_rate_mv_ = (fall_rate_mv < 1) ? 1 : fall_rate_mv;
        curve_ = curve;
    }

    void set_cycle(bool cycle) noexcept {
        cycle_mode_ = cycle;
    }

    struct SlewOutput {
        int16_t cv_out_mv{0};
        bool end_of_rise{false};
        bool end_of_fall{false};
    };

    SlewOutput process(int16_t target_mv) noexcept {
        end_of_rise_ = false;
        end_of_fall_ = false;

        // In self-cycle mode, target alternates between 5000 mV and 0 mV upon reaching peaks
        if (cycle_mode_) {
            if (current_mv_ >= 5000) {
                cycling_up_ = false;
            } else if (current_mv_ <= 0) {
                cycling_up_ = true;
            }
            target_mv = cycling_up_ ? 5000 : 0;
        }

        if (current_mv_ < target_mv) {
            // Rising edge
            int32_t step = rise_rate_mv_;
            if (curve_ == Curve::Exponential) {
                int32_t delta = target_mv - current_mv_;
                step = (delta * rise_rate_mv_) >> 8;
                if (step < 1) step = 1;
            }

            current_mv_ += step;
            if (current_mv_ >= target_mv) {
                current_mv_ = target_mv;
                end_of_rise_ = true;
            }
        } else if (current_mv_ > target_mv) {
            // Falling edge
            int32_t step = fall_rate_mv_;
            if (curve_ == Curve::Exponential) {
                int32_t delta = current_mv_ - target_mv;
                step = (delta * fall_rate_mv_) >> 8;
                if (step < 1) step = 1;
            }

            current_mv_ -= step;
            if (current_mv_ <= target_mv) {
                current_mv_ = target_mv;
                end_of_fall_ = true;
            }
        }

        return {static_cast<int16_t>(current_mv_), end_of_rise_, end_of_fall_};
    }

    [[nodiscard]] int16_t current_cv() const noexcept {
        return static_cast<int16_t>(current_mv_);
    }

private:
    int32_t current_mv_{0};
    int32_t rise_rate_mv_{50};
    int32_t fall_rate_mv_{50};
    Curve curve_{Curve::Linear};
    bool cycle_mode_{false};
    bool cycling_up_{true};
    bool end_of_rise_{false};
    bool end_of_fall_{false};
};

} // namespace aether::embedded
