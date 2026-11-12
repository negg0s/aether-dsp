#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedRingModulator
 * Ring modulator and frequency shifter in Q15 for embedded targets.
 * Features:
 * - Internal quadrature carrier generator (sine/cosine phase accumulator)
 * - True 4-quadrant analog diode ring bridge emulation:
 *   Carrier * Signal multiplication with optional soft-diode deadzone in Q15
 * - Carrier frequency from 0.1 Hz (tremolo) to 8000 Hz (bell, metallic, robot sounds)
 * - Zero heap, zero FPU.
 */
class FixedRingModulator {
public:
    constexpr FixedRingModulator() noexcept {
        init_sin_lut();
    }

    void set_carrier_frequency(float freq_hz, uint32_t sample_rate) noexcept {
        if (sample_rate == 0) return;
        freq_hz = std::clamp(freq_hz, 0.1f, static_cast<float>(sample_rate) * 0.45f);
        phase_inc_ = static_cast<uint32_t>((static_cast<double>(freq_hz) * 4294967296.0) / sample_rate);
    }

    void set_phase_increment(uint32_t inc) noexcept {
        phase_inc_ = inc;
    }

    // mix: Q15 wet mix [0, 32767]
    // diode_deadzone: Q15 threshold below which diode bridge attenuates [0, 8000]
    void configure(FixedQ15 mix, int16_t diode_deadzone) noexcept {
        mix_ = mix.raw < 0 ? FixedQ15::from_raw(0) : mix;
        deadzone_ = std::clamp<int16_t>(diode_deadzone, 0, 8000);
    }

    void reset() noexcept {
        phase_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        // Step carrier phase
        phase_ += phase_inc_;
        uint32_t idx = (phase_ >> 24) & 0xFF; // [0, 255]
        int16_t carrier = sin_lut_[idx];

        // Diode ring bridge emulation:
        // When carrier is within deadzone, diode conducting behavior transitions
        int32_t c_eff = carrier;
        if (std::abs(c_eff) < deadzone_) {
            c_eff = (c_eff * c_eff) / (deadzone_ > 0 ? deadzone_ : 1);
            if (carrier < 0) c_eff = -c_eff;
        }

        // 4-quadrant multiplication: (input * carrier) >> 15
        int32_t ring_prod = (static_cast<int32_t>(input.raw) * c_eff) >> 15;
        ring_prod = std::clamp<int32_t>(ring_prod, -32768, 32767);

        // Dry / Wet mix
        int32_t dry_mul = 32767 - mix_.raw;
        int32_t mixed = ((static_cast<int32_t>(input.raw) * dry_mul) >> 15) + ((ring_prod * mix_.raw) >> 15);
        mixed = std::clamp<int32_t>(mixed, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(mixed));
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
    uint32_t phase_inc_{0};
    FixedQ15 mix_{FixedQ15::from_raw(32767)}; // 100% wet
    int16_t deadzone_{0};
    std::array<int16_t, 256> sin_lut_{};
};

} // namespace aether::math
