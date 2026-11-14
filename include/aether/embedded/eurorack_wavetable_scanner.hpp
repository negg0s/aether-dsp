#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>
#include <cmath>

namespace aether::embedded {

/**
 * @brief Fixed-point 2D wavetable scanner with sub-sample Hermite interpolation.
 * Supports multi-bank morphing across 4 distinct single-cycle waveforms:
 * Bank 0: Bandlimited Saw
 * Bank 1: Pulse/Square
 * Bank 2: Triangle
 * Bank 3: Parabolic Sine
 * 
 * Wave morphing position spans Bank 0 to Bank 3 continuously with Q15 interpolation.
 * Zero dynamic heap allocations. Suitable for STM32 / Daisy Patch Eurorack oscillators.
 */
class EurorackWavetableScanner {
public:
    static constexpr size_t kWavetableSize = 256;
    static constexpr size_t kNumBanks = 4;
    static constexpr size_t kTableMask = kWavetableSize - 1;

    EurorackWavetableScanner() {
        init_tables();
        reset();
    }

    void reset() {
        phase_ = 0;
        phase_inc_ = 0;
        morph_pos_ = 0;
    }

    void set_frequency(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 0.1f) freq_hz = 0.1f;
        if (freq_hz > sample_rate_hz * 0.45f) freq_hz = sample_rate_hz * 0.45f;

        // phase is 32-bit unsigned accumulator representing [0, 1)
        double inc = (static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        phase_inc_ = static_cast<uint32_t>(inc);
    }

    /**
     * @brief Set morph position across banks [0.0, 3.0] mapped to Q15 [0, 32767].
     * 0 -> Bank 0 (Saw)
     * 10922 -> Bank 1 (Pulse)
     * 21845 -> Bank 2 (Triangle)
     * 32767 -> Bank 3 (Sine)
     */
    void set_morph(math::FixedQ15 morph) {
        if (morph.raw < 0) morph.raw = 0;
        morph_pos_ = morph.raw;
    }

    math::FixedQ15 process() {
        // Step 1: Advance phase accumulator
        phase_ += phase_inc_;

        // High 8 bits give table index, next 15 bits give sub-sample fraction
        uint32_t table_idx = (phase_ >> 24) & kTableMask;
        int16_t frac_q15 = static_cast<int16_t>((phase_ >> 9) & 0x7FFF); // [0, 32767]

        // Determine bank index and bank fractional morph
        // morph_pos_ is [0, 32767] spanning 3 intervals (banks 0->1, 1->2, 2->3)
        // Interval width = 32767 / 3 = 10922
        size_t bank_low = 0;
        size_t bank_high = 1;
        int16_t bank_frac = 0;

        if (morph_pos_ < 10922) {
            bank_low = 0;
            bank_high = 1;
            bank_frac = static_cast<int16_t>((static_cast<int32_t>(morph_pos_) * 32767) / 10922);
        } else if (morph_pos_ < 21845) {
            bank_low = 1;
            bank_high = 2;
            bank_frac = static_cast<int16_t>((static_cast<int32_t>(morph_pos_ - 10922) * 32767) / (21845 - 10922));
        } else {
            bank_low = 2;
            bank_high = 3;
            int32_t rem = morph_pos_ - 21845;
            if (rem > (32767 - 21845)) rem = 32767 - 21845;
            bank_frac = static_cast<int16_t>((rem * 32767) / (32767 - 21845));
        }

        // Hermite 4-point cubic interpolation on low bank
        int32_t s_low = interpolate_hermite(bank_low, table_idx, frac_q15);
        // Hermite 4-point cubic interpolation on high bank
        int32_t s_high = interpolate_hermite(bank_high, table_idx, frac_q15);

        // Crossfade between bank_low and bank_high
        int32_t cross = s_low + static_cast<int32_t>((static_cast<int64_t>(s_high - s_low) * bank_frac) >> 15);
        if (cross > 32767) cross = 32767;
        if (cross < -32768) cross = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(cross));
    }

private:
    uint32_t phase_{0};
    uint32_t phase_inc_{0};
    int16_t morph_pos_{0};
    int16_t tables_[kNumBanks][kWavetableSize];

    void init_tables() {
        for (size_t i = 0; i < kWavetableSize; ++i) {
            double phase = static_cast<double>(i) / static_cast<double>(kWavetableSize); // [0, 1)

            // Bank 0: Sawtooth (-1 to +1)
            double saw = 2.0 * phase - 1.0;
            tables_[0][i] = static_cast<int16_t>(saw * 32767.0);

            // Bank 1: Pulse/Square (-1 or +1)
            double pulse = (phase < 0.5) ? 0.95 : -0.95;
            tables_[1][i] = static_cast<int16_t>(pulse * 32767.0);

            // Bank 2: Triangle (-1 to +1 to -1)
            double tri = (phase < 0.5) ? (4.0 * phase - 1.0) : (3.0 - 4.0 * phase);
            tables_[2][i] = static_cast<int16_t>(tri * 32767.0);

            // Bank 3: Sine wave
            double sine = std::sin(phase * 2.0 * 3.14159265358979323846);
            tables_[3][i] = static_cast<int16_t>(sine * 32767.0);
        }
    }

    // 4-point Hermite cubic polynomial interpolation
    int32_t interpolate_hermite(size_t bank, uint32_t idx, int16_t frac) const {
        size_t i0 = (idx + kWavetableSize - 1) & kTableMask;
        size_t i1 = idx;
        size_t i2 = (idx + 1) & kTableMask;
        size_t i3 = (idx + 2) & kTableMask;

        int32_t y0 = tables_[bank][i0];
        int32_t y1 = tables_[bank][i1];
        int32_t y2 = tables_[bank][i2];
        int32_t y3 = tables_[bank][i3];

        // x is frac in range [0, 32767] (representing 0.0 to 1.0)
        // c0 = y1
        // c1 = 0.5 * (y2 - y0)
        // c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3
        // c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2)
        // Hermite: ((c3 * x + c2) * x + c1) * x + c0
        int64_t x = frac; // Q15
        int64_t c0 = y1;
        int64_t c1 = (y2 - y0) / 2;
        int64_t c2 = y0 - (5 * y1) / 2 + 2 * y2 - y3 / 2;
        int64_t c3 = (y3 - y0) / 2 + (3 * (y1 - y2)) / 2;

        int64_t val = (c3 * x) >> 15;
        val = ((val + c2) * x) >> 15;
        val = ((val + c1) * x) >> 15;
        val = val + c0;

        return static_cast<int32_t>(val);
    }
};

} // namespace aether::embedded
