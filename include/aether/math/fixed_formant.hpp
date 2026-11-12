#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedFormantFilter
 * Vocal tract resonator in Q15:
 * - Triple parallel 2nd-order bandpass resonators
 * - Vowel presets (A, E, I, O, U) modeled via fixed-point coefficients
 * - Vocal formants F1, F2, F3 emulating human phonetic resonances
 * - Zero heap, zero FPU, ultra-fast.
 */
class FixedFormantFilter {
public:
    enum class Vowel : uint8_t {
        A = 0, // [a] "father"
        E = 1, // [e] "bed"
        I = 2, // [i] "see"
        O = 3, // [o] "boat"
        U = 4  // [u] "boot"
    };

    constexpr FixedFormantFilter() noexcept = default;

    void set_vowel(Vowel v) noexcept {
        vowel_ = v;
        // Formant coefficient lookup in Q15:
        // f_coef = 2 * sin(pi * fc / 48000)
        // [A]: F1 ~ 800 Hz, F2 ~ 1200 Hz, F3 ~ 2500 Hz
        // [E]: F1 ~ 500 Hz, F2 ~ 1800 Hz, F3 ~ 2500 Hz
        // [I]: F1 ~ 300 Hz, F2 ~ 2300 Hz, F3 ~ 3000 Hz
        // [O]: F1 ~ 500 Hz, F2 ~ 900 Hz,  F3 ~ 2400 Hz
        // [U]: F1 ~ 350 Hz, F2 ~ 800 Hz,  F3 ~ 2200 Hz
        static constexpr std::array<std::array<int16_t, 3>, 5> FORMANT_F_Q15 = {{
            {{ 3490, 5210, 10700 }}, // A
            {{ 2185, 7780, 10700 }}, // E
            {{ 1315, 9880, 12760 }}, // I
            {{ 2185, 3920, 10300 }}, // O
            {{ 1530, 3490,  9480 }}  // U
        }};

        size_t idx = static_cast<size_t>(v);
        if (idx >= 5) idx = 0;
        f1_ = FixedQ15::from_raw(FORMANT_F_Q15[idx][0]);
        f2_ = FixedQ15::from_raw(FORMANT_F_Q15[idx][1]);
        f3_ = FixedQ15::from_raw(FORMANT_F_Q15[idx][2]);
    }

    void reset() noexcept {
        s1_lp_ = s1_bp_ = 0;
        s2_lp_ = s2_bp_ = 0;
        s3_lp_ = s3_bp_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        // Run 3 parallel 2nd-order bandpass resonators (Q ~ 10 -> q_inv ~ 3276)
        static constexpr int32_t Q_INV = 3276; // Q15 ~ 0.1

        int32_t in_val = input.raw;

        // Formant 1
        int32_t hp1 = in_val - s1_lp_ - ((s1_bp_ * Q_INV) >> 15);
        hp1 = std::clamp<int32_t>(hp1, -32768, 32767);
        int32_t bp1 = s1_bp_ + ((hp1 * f1_.raw) >> 15);
        bp1 = std::clamp<int32_t>(bp1, -32768, 32767);
        s1_lp_ += (bp1 * f1_.raw) >> 15;
        s1_lp_ = std::clamp<int32_t>(s1_lp_, -32768, 32767);
        s1_bp_ = bp1;

        // Formant 2
        int32_t hp2 = in_val - s2_lp_ - ((s2_bp_ * Q_INV) >> 15);
        hp2 = std::clamp<int32_t>(hp2, -32768, 32767);
        int32_t bp2 = s2_bp_ + ((hp2 * f2_.raw) >> 15);
        bp2 = std::clamp<int32_t>(bp2, -32768, 32767);
        s2_lp_ += (bp2 * f2_.raw) >> 15;
        s2_lp_ = std::clamp<int32_t>(s2_lp_, -32768, 32767);
        s2_bp_ = bp2;

        // Formant 3
        int32_t hp3 = in_val - s3_lp_ - ((s3_bp_ * Q_INV) >> 15);
        hp3 = std::clamp<int32_t>(hp3, -32768, 32767);
        int32_t bp3 = s3_bp_ + ((hp3 * f3_.raw) >> 15);
        bp3 = std::clamp<int32_t>(bp3, -32768, 32767);
        s3_lp_ += (bp3 * f3_.raw) >> 15;
        s3_lp_ = std::clamp<int32_t>(s3_lp_, -32768, 32767);
        s3_bp_ = bp3;

        // Sum formant outputs: bp1 (1.0x) + bp2 (0.6x) + bp3 (0.3x)
        int32_t vocal = bp1 + ((bp2 * 19660) >> 15) + ((bp3 * 9830) >> 15);
        vocal = std::clamp<int32_t>(vocal, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(vocal));
    }

private:
    Vowel vowel_{Vowel::A};
    FixedQ15 f1_{FixedQ15::from_raw(3490)};
    FixedQ15 f2_{FixedQ15::from_raw(5210)};
    FixedQ15 f3_{FixedQ15::from_raw(10700)};

    int32_t s1_lp_{0}, s1_bp_{0};
    int32_t s2_lp_{0}, s2_bp_{0};
    int32_t s3_lp_{0}, s3_bp_{0};
};

} // namespace aether::math
