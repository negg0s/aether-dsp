#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedCompressor
 * Zero-heap, fixed-point feed-forward dynamic range compressor and limiter in Q15.
 * Features:
 * - Peak detector with separate attack and release time constants
 * - Threshold and compression ratio (1:1 up to infinite brickwall limit)
 * - Make-up gain in Q15
 * - Zero FPU, zero transcendental functions, branchless attenuation math.
 */
class FixedCompressor {
public:
    constexpr FixedCompressor() noexcept = default;

    /**
     * configure
     * threshold: Q15 in range [0, 32767] (default ~0.5 -> 16384)
     * ratio: compression slope factor in Q15 (0 = 1:1 no compression, 32767 = brickwall limiter)
     * attack_coeff: Q15 smoothing factor for envelope attack [0, 32767]
     * release_coeff: Q15 smoothing factor for envelope release [0, 32767]
     * makeup_gain_q15: linear gain multiplier in Q15 (32768 = 1.0x, 65536 = 2.0x)
     */
    void configure(FixedQ15 threshold, FixedQ15 ratio, FixedQ15 attack_coeff, FixedQ15 release_coeff, int32_t makeup_gain_q15) noexcept {
        threshold_ = threshold.raw < 0 ? FixedQ15::from_raw(0) : threshold;
        ratio_ = ratio.raw < 0 ? FixedQ15::from_raw(0) : ratio;
        attack_coeff_ = attack_coeff.raw < 0 ? FixedQ15::from_raw(0) : attack_coeff;
        release_coeff_ = release_coeff.raw < 0 ? FixedQ15::from_raw(0) : release_coeff;
        makeup_gain_ = std::clamp<int32_t>(makeup_gain_q15, 0, 262144);
    }

    void reset() noexcept {
        env_peak_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        // Absolute input level in Q15
        int32_t in_abs = std::abs(static_cast<int32_t>(input.raw));
        if (in_abs > 32767) in_abs = 32767;

        // Peak follower (ballistics)
        if (in_abs > env_peak_) {
            // Attack phase: env += (in_abs - env) * attack
            int32_t diff = in_abs - env_peak_;
            env_peak_ += (diff * attack_coeff_.raw) >> 15;
        } else {
            // Release phase: env -= env * release
            int32_t delta = (env_peak_ * release_coeff_.raw) >> 15;
            env_peak_ -= delta;
        }
        env_peak_ = std::clamp<int32_t>(env_peak_, 0, 32767);

        // Gain computer: if env_peak > threshold, attenuate
        int32_t gain_raw = 32767; // 1.0 in Q15
        if (env_peak_ > threshold_.raw && threshold_.raw > 0) {
            int32_t over = env_peak_ - threshold_.raw;
            // Reduction: over * ratio
            int32_t reduction = (over * ratio_.raw) >> 15;
            gain_raw = 32767 - reduction;
            gain_raw = std::clamp<int32_t>(gain_raw, 100, 32767);
        }

        // Apply compression gain to input sample
        int64_t compressed = (static_cast<int64_t>(input.raw) * gain_raw) >> 15;

        // Apply make-up gain
        int64_t output = (compressed * makeup_gain_) >> 15;
        int32_t clamped = static_cast<int32_t>(std::clamp<int64_t>(output, -32768, 32767));

        return FixedQ15::from_raw(static_cast<int16_t>(clamped));
    }

private:
    FixedQ15 threshold_{FixedQ15::from_raw(16384)}; // ~0.5
    FixedQ15 ratio_{FixedQ15::from_raw(16384)};     // moderate ratio
    FixedQ15 attack_coeff_{FixedQ15::from_raw(16384)};
    FixedQ15 release_coeff_{FixedQ15::from_raw(1000)};
    int32_t makeup_gain_{32768};                    // 1.0 in Q15
    int32_t env_peak_{0};
};

} // namespace aether::math
