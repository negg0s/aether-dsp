#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Voltage Inverter, Attenuverter & DC Offset Shifter
// Scaled dual-channel utility for modular CV processing:
// Out = (In * Scale) >> 15 + Offset_mV
// Supports bipolar inversion (-1.0 to +1.0 in Q15) and precision DC offset (-5000 to +5000 mV).
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackAttenuverter {
public:
    EurorackAttenuverter() {
        reset();
    }

    void reset() noexcept {
        scale_q15_ = math::FixedQ15::from_float(1.0f);
        offset_mv_ = 0;
        mute_ = false;
    }

    // Set attenuverting scale (-1.0 to +1.0 in Q15, where -1.0 is full inversion)
    void set_scale(math::FixedQ15 scale) noexcept {
        scale_q15_ = scale;
    }

    // Set DC offset voltage in millivolts (-5000 to +5000 mV)
    void set_offset(int16_t offset_mv) noexcept {
        offset_mv_ = offset_mv;
    }

    void set_mute(bool mute) noexcept {
        mute_ = mute;
    }

    // Process CV in millivolts (e.g. 0 to 5000 mV)
    [[nodiscard]] int16_t process_cv(int16_t input_mv) const noexcept {
        if (mute_) return 0;
        int64_t scaled = (static_cast<int64_t>(input_mv) * scale_q15_.raw) >> 15;
        int64_t out_mv = scaled + offset_mv_;
        if (out_mv > 10000) out_mv = 10000;
        if (out_mv < -10000) out_mv = -10000;
        return static_cast<int16_t>(out_mv);
    }

    // Process audio signal in Q15 (-1.0 to +1.0)
    [[nodiscard]] math::FixedQ15 process_audio(math::FixedQ15 input) const noexcept {
        if (mute_) return math::FixedQ15(0);
        int64_t scaled = (static_cast<int64_t>(input.raw) * scale_q15_.raw) >> 15;
        // Convert offset_mv (where 5000 mV = 1.0 = 32767) to Q15 offset
        int64_t offset_q15 = (static_cast<int64_t>(offset_mv_) * 32767) / 5000;
        int64_t out_audio = scaled + offset_q15;
        if (out_audio > 32767) out_audio = 32767;
        if (out_audio < -32768) out_audio = -32768;
        return math::FixedQ15(static_cast<int16_t>(out_audio));
    }

    [[nodiscard]] math::FixedQ15 scale() const noexcept {
        return scale_q15_;
    }

    [[nodiscard]] int16_t offset() const noexcept {
        return offset_mv_;
    }

private:
    math::FixedQ15 scale_q15_{math::FixedQ15::from_float(1.0f)};
    int16_t offset_mv_{0};
    bool mute_{false};
};

} // namespace aether::embedded
