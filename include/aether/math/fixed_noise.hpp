#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedNoise
 * Embedded Pseudo-Random Noise and Random Walk generator in Q15.
 * Modes:
 * - White noise (uniform 32-bit xorshift)
 * - Pink/Brown noise (integrated random walk with leaky leak)
 * Zero heap, zero FPU, ultra-fast for percussive hits and analog warmth.
 */
class FixedNoise {
public:
    constexpr FixedNoise() noexcept = default;

    void seed(uint32_t s) noexcept {
        state_ = s != 0 ? s : 0x12345678u;
    }

    [[nodiscard]] FixedQ15 process_white() noexcept {
        // 32-bit Xorshift
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        // Take middle 16 bits as signed Q15
        int16_t sample = static_cast<int16_t>((state_ >> 8) & 0xFFFF);
        return FixedQ15::from_raw(sample);
    }

    [[nodiscard]] FixedQ15 process_pink() noexcept {
        FixedQ15 white = process_white();
        // 1st order leaky integrator for Brownian/Pink slope:
        // pink = 0.85 * pink + 0.15 * white
        int32_t integrated = (pink_state_ * 27852) >> 15; // 0.85 in Q15
        integrated += (static_cast<int32_t>(white.raw) * 4915) >> 15; // 0.15 in Q15
        pink_state_ = std::clamp<int32_t>(integrated, -32768, 32767);
        return FixedQ15::from_raw(static_cast<int16_t>(pink_state_));
    }

private:
    uint32_t state_{0xDEADBEEFu};
    int32_t pink_state_{0};
};

} // namespace aether::math
