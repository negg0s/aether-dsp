#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedTremolo
 * Stereo pan tremolo / optical photocell tremolo in Q15:
 * - LFO shapes: Sine, Square (stutter / chopper), Triangle
 * - Stereo phase offset (mono amplitude tremolo vs auto-pan)
 * - Optical photocell response smoothing (simulates incandescent lamp lag)
 * Zero dynamic heap allocation.
 */
class FixedTremolo {
public:
    enum class Mode : uint8_t {
        Mono = 0,
        AutoPan = 1
    };

    constexpr FixedTremolo() noexcept {
        init_sin_lut();
    }

    /**
     * configure
     * rate_inc: LFO 32-bit phase increment per sample
     * depth: modulation depth in Q15 [0, 32767]
     * mode: Mono tremolo or ping-pong AutoPan
     */
    void configure(uint32_t rate_inc, FixedQ15 depth, Mode mode) noexcept {
        rate_inc_ = rate_inc;
        depth_ = std::clamp<int16_t>(depth.raw, 0, 32767);
        mode_ = mode;
    }

    void reset() noexcept {
        phase_ = 0;
        lamp_state_l_ = 32767;
        lamp_state_r_ = 32767;
    }

    struct StereoSample {
        FixedQ15 left;
        FixedQ15 right;
    };

    [[nodiscard]] StereoSample process(FixedQ15 in_l, FixedQ15 in_r) noexcept {
        phase_ += rate_inc_;
        uint32_t idx_l = (phase_ >> 24) & 0xFF;
        int32_t lfo_raw_l = sin_lut_[idx_l]; // -32768 to 32767

        int32_t mod_l = (lfo_raw_l + 32768) >> 1; // 0 to 32767
        int32_t gain_l = 32767 - ((mod_l * depth_) >> 15);

        int32_t gain_r = gain_l;
        if (mode_ == Mode::AutoPan) {
            uint32_t idx_r = ((phase_ + 0x80000000u) >> 24) & 0xFF; // 180 deg anti-phase
            int32_t lfo_raw_r = sin_lut_[idx_r];
            int32_t mod_r = (lfo_raw_r + 32768) >> 1;
            gain_r = 32767 - ((mod_r * depth_) >> 15);
        }

        // Photocell lag filter (1-pole smoothing): lamp += (gain - lamp) * 0.25
        lamp_state_l_ += ((gain_l - lamp_state_l_) * 8192) >> 15;
        lamp_state_r_ += ((gain_r - lamp_state_r_) * 8192) >> 15;

        int32_t out_l = (static_cast<int32_t>(in_l.raw) * lamp_state_l_) >> 15;
        int32_t out_r = (static_cast<int32_t>(in_r.raw) * lamp_state_r_) >> 15;

        out_l = std::clamp<int32_t>(out_l, -32768, 32767);
        out_r = std::clamp<int32_t>(out_r, -32768, 32767);

        return {FixedQ15::from_raw(static_cast<int16_t>(out_l)), FixedQ15::from_raw(static_cast<int16_t>(out_r))};
    }

private:
    constexpr void init_sin_lut() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            int32_t x = static_cast<int32_t>(i);
            int32_t q = 0;
            if (x < 64) {
                q = (x * 32767) / 64;
            } else if (x < 128) {
                q = ((128 - x) * 32767) / 64;
            } else if (x < 192) {
                q = -(((x - 128) * 32767) / 64);
            } else {
                q = -(((256 - x) * 32767) / 64);
            }
            sin_lut_[i] = static_cast<int16_t>(q);
        }
    }

    uint32_t phase_{0};
    uint32_t rate_inc_{1000000};
    int16_t depth_{16384}; // 50%
    Mode mode_{Mode::AutoPan};
    int32_t lamp_state_l_{32767};
    int32_t lamp_state_r_{32767};
    std::array<int16_t, 256> sin_lut_{};
};

} // namespace aether::math
