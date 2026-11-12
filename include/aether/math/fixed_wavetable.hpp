#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedWavetableOscillator
 * Dual-wavetable morphing oscillator in Q15:
 * - Table 0: Sine wave (pure fundamental)
 * - Table 1: Chebyshev / Hyperbolic nonlinear waveshape (rich harmonics)
 * - 32-bit Phase Accumulator
 * - Morph parameter (Q15: 0 = Table 0, 1.0 = Table 1) with linear interpolation
 * Deterministic, no dynamic allocation, suitable for MCUs.
 */
class FixedWavetableOscillator {
public:
    static constexpr size_t TABLE_SIZE = 256;
    static constexpr uint32_t TABLE_MASK = TABLE_SIZE - 1;
    static constexpr uint32_t PHASE_TO_INDEX_SHIFT = 24; // 32 - 8 = 24 bits

    constexpr FixedWavetableOscillator() noexcept {
        init_tables();
    }

    void set_phase_increment(uint32_t inc) noexcept {
        phase_inc_ = inc;
    }

    [[nodiscard]] uint32_t phase_increment() const noexcept {
        return phase_inc_;
    }

    // Set morph parameter in Q15 [0, 32767]
    void set_morph(FixedQ15 morph) noexcept {
        morph_ = morph.raw < 0 ? FixedQ15::from_raw(0) : morph;
    }

    [[nodiscard]] FixedQ15 morph() const noexcept {
        return morph_;
    }

    void reset_phase() noexcept {
        phase_ = 0;
    }

    [[nodiscard]] FixedQ15 process_sample() noexcept {
        uint32_t idx0 = (phase_ >> PHASE_TO_INDEX_SHIFT) & TABLE_MASK;
        uint32_t idx1 = (idx0 + 1) & TABLE_MASK;

        // Sub-table fractional interpolation: 16 bits of fraction
        int64_t frac = static_cast<int64_t>((phase_ >> 8) & 0xFFFF); // Q16 fraction

        // Interpolate Table 0 (Sine)
        int64_t s0 = table0_[idx0].raw;
        int64_t s1 = table0_[idx1].raw;
        int64_t t0_val = s0 + (((s1 - s0) * frac) >> 16);

        // Interpolate Table 1 (Harmonic / Hyperbolic)
        int64_t h0 = table1_[idx0].raw;
        int64_t h1 = table1_[idx1].raw;
        int64_t t1_val = h0 + (((h1 - h0) * frac) >> 16);

        // Morph between t0 and t1: out = t0 + morph * (t1 - t0)
        int64_t diff = t1_val - t0_val;
        int64_t morphed = t0_val + ((diff * static_cast<int64_t>(morph_.raw)) >> 15);

        // Saturate to Q15 range
        int32_t clamped = static_cast<int32_t>(std::clamp<int64_t>(morphed, -32768, 32767));

        phase_ += phase_inc_;
        return FixedQ15::from_raw(static_cast<int16_t>(clamped));
    }

private:
    constexpr void init_tables() noexcept {
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
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
            table0_[i] = FixedQ15::from_raw(static_cast<int16_t>(q));

            int32_t ramp = (static_cast<int32_t>(i) - 128) * 512;
            ramp = std::clamp(ramp, -32767, 32767);
            table1_[i] = FixedQ15::from_raw(static_cast<int16_t>(ramp));
        }
    }

    uint32_t phase_{0};
    uint32_t phase_inc_{0};
    FixedQ15 morph_{FixedQ15::from_raw(0)};
    std::array<FixedQ15, TABLE_SIZE> table0_{};
    std::array<FixedQ15, TABLE_SIZE> table1_{};
};

} // namespace aether::math
