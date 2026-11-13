#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedLeslieSpeaker
 * Rotary speaker horn and drum Doppler simulator in Q15:
 * - Separate rotating horn (high frequencies) and rotating drum (low frequencies)
 * - Quadrature sine modulation for true Doppler pitch deflection and amplitude tremolo
 * - Inertial acceleration/deceleration physics (slow/fast chorale vs tremolo speeds)
 * - Zero dynamic heap allocation.
 */
class FixedLeslieSpeaker {
public:
    enum class Speed : uint8_t {
        Brake = 0,
        Slow = 1,
        Fast = 2
    };

    static constexpr size_t DELAY_BUF_SIZE = 256;
    static constexpr size_t DELAY_MASK = DELAY_BUF_SIZE - 1;

    constexpr FixedLeslieSpeaker() noexcept {
        init_sin_lut();
    }

    void set_speed(Speed s) noexcept {
        target_speed_ = s;
    }

    void reset() noexcept {
        horn_buf_l_.fill(FixedQ15::from_raw(0));
        horn_buf_r_.fill(FixedQ15::from_raw(0));
        horn_write_ = 0;
        horn_phase_ = 0;
        drum_phase_ = 0;
        current_horn_rate_ = 100000;
        current_drum_rate_ = 80000;
    }

    struct StereoRotary {
        FixedQ15 left;
        FixedQ15 right;
    };

    [[nodiscard]] StereoRotary process(FixedQ15 in_l, FixedQ15 in_r) noexcept {
        // Target rate increments:
        // Slow (chorale): horn ~0.8 Hz (71582788), drum ~0.6 Hz (53687091)
        // Fast (tremolo): horn ~6.8 Hz (608453700), drum ~5.9 Hz (527765500)
        // Brake: 0
        uint32_t target_horn_rate = 0;
        uint32_t target_drum_rate = 0;
        if (target_speed_ == Speed::Slow) {
            target_horn_rate = 71582788u;
            target_drum_rate = 53687091u;
        } else if (target_speed_ == Speed::Fast) {
            target_horn_rate = 608453700u;
            target_drum_rate = 527765500u;
        }

        // Inertial acceleration: horn responds faster than heavy drum
        if (current_horn_rate_ < target_horn_rate) {
            current_horn_rate_ += std::min<uint32_t>(10000u, target_horn_rate - current_horn_rate_);
        } else if (current_horn_rate_ > target_horn_rate) {
            current_horn_rate_ -= std::min<uint32_t>(10000u, current_horn_rate_ - target_horn_rate);
        }

        if (current_drum_rate_ < target_drum_rate) {
            current_drum_rate_ += std::min<uint32_t>(3000u, target_drum_rate - current_drum_rate_);
        } else if (current_drum_rate_ > target_drum_rate) {
            current_drum_rate_ -= std::min<uint32_t>(3000u, current_drum_rate_ - target_drum_rate);
        }

        horn_phase_ += current_horn_rate_;
        drum_phase_ += current_drum_rate_;

        // Write input to buffer
        horn_buf_l_[horn_write_] = in_l;
        horn_buf_r_[horn_write_] = in_r;

        // Doppler pitch modulation via modulated delay: delay = 32 + (depth * sin) >> 15
        uint32_t idx_h_l = (horn_phase_ >> 24) & 0xFF;
        uint32_t idx_h_r = ((horn_phase_ + 0x40000000u) >> 24) & 0xFF; // 90 deg offset

        int32_t sin_h_l = sin_lut_[idx_h_l];
        int32_t sin_h_r = sin_lut_[idx_h_r];

        size_t d_l = 32 + ((16 * (sin_h_l + 32768)) >> 16);
        size_t d_r = 32 + ((16 * (sin_h_r + 32768)) >> 16);

        int32_t s_l = horn_buf_l_[(horn_write_ + DELAY_BUF_SIZE - d_l) & DELAY_MASK].raw;
        int32_t s_r = horn_buf_r_[(horn_write_ + DELAY_BUF_SIZE - d_r) & DELAY_MASK].raw;

        horn_write_ = (horn_write_ + 1) & DELAY_MASK;

        // Amplitude tremolo on horn
        int32_t amp_h_l = (sin_h_l + 32768) >> 1; // 0..32767
        int32_t amp_h_r = (sin_h_r + 32768) >> 1;

        int32_t out_l = (s_l * (16384 + (amp_h_l >> 1))) >> 15;
        int32_t out_r = (s_r * (16384 + (amp_h_r >> 1))) >> 15;

        // Low drum rotor amplitude modulation
        uint32_t idx_d_l = (drum_phase_ >> 24) & 0xFF;
        uint32_t idx_d_r = ((drum_phase_ + 0x80000000u) >> 24) & 0xFF;
        int32_t amp_d_l = (sin_lut_[idx_d_l] + 32768) >> 1;
        int32_t amp_d_r = (sin_lut_[idx_d_r] + 32768) >> 1;

        out_l = (out_l * (20000 + ((amp_d_l * 12767) >> 15))) >> 15;
        out_r = (out_r * (20000 + ((amp_d_r * 12767) >> 15))) >> 15;

        out_l = std::clamp<int32_t>(out_l, -32768, 32767);
        out_r = std::clamp<int32_t>(out_r, -32768, 32767);

        return { FixedQ15::from_raw(static_cast<int16_t>(out_l)), FixedQ15::from_raw(static_cast<int16_t>(out_r)) };
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

    std::array<FixedQ15, DELAY_BUF_SIZE> horn_buf_l_{};
    std::array<FixedQ15, DELAY_BUF_SIZE> horn_buf_r_{};
    size_t horn_write_{0};
    uint32_t horn_phase_{0};
    uint32_t drum_phase_{0};
    uint32_t current_horn_rate_{0};
    uint32_t current_drum_rate_{0};
    Speed target_speed_{Speed::Fast};
    std::array<int16_t, 256> sin_lut_{};
};

} // namespace aether::math
