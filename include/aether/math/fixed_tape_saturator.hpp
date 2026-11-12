#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedHarmonicExciter
 * Psychoacoustic tape saturation and warm odd/even harmonic enhancer in Q15:
 * - Emulates analog magnetic tape saturation with soft hysteresis compression
 * - Variable tape speed / bias (controls high-frequency tape head flux saturation)
 * - Mix blend for mastering and bus processing
 * - Zero dynamic heap allocation.
 */
class FixedTapeSaturator {
public:
    constexpr FixedTapeSaturator() noexcept = default;

    /**
     * configure
     * drive_q15: tape input drive (32768 = 1.0x, up to 131072 = 4.0x)
     * bias_q15: high-frequency flux bias / warmth [1000, 32767]
     * mix: wet mix in Q15 [0, 32767]
     */
    void configure(int32_t drive_q15, FixedQ15 bias, FixedQ15 mix) noexcept {
        drive_ = std::clamp<int32_t>(drive_q15, 1000, 262144);
        bias_ = std::clamp<int16_t>(bias.raw, 1000, 32767);
        mix_ = mix;
    }

    void reset() noexcept {
        tape_flux_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        int64_t driven = (static_cast<int64_t>(input.raw) * drive_) >> 15;

        // Tape head hysteresis integration: flux += (driven - flux) * bias
        tape_flux_ += ((driven - tape_flux_) * bias_) >> 15;
        tape_flux_ = std::clamp<int64_t>(tape_flux_, -32768, 32767);

        // Hyperbolic rational saturation: y = 1.5 * x * (1 - x^2 / 3)
        int64_t x = tape_flux_;
        int64_t x2 = (x * x) >> 15;
        int64_t x3 = (x2 * x) >> 15;
        int64_t sat = x - (x3 / 3);
        sat = std::clamp<int64_t>(sat, -32768, 32767);

        // Dry / Wet mix
        int32_t dry_mul = 32767 - mix_.raw;
        int32_t out = ((static_cast<int32_t>(input.raw) * dry_mul) >> 15) + ((static_cast<int32_t>(sat) * mix_.raw) >> 15);
        out = std::clamp<int32_t>(out, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t drive_{32768}; // 1.0x
    int16_t bias_{24000};
    FixedQ15 mix_{FixedQ15::from_raw(32767)}; // 100% wet
    int64_t tape_flux_{0};
};

} // namespace aether::math
