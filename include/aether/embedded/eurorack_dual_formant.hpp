#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Peak / Notch Formant Filter
// Models vocal formant resonances (mouth/throat cavities) via coupled dual biquad sections
// with variable center frequency, Q resonance, and dry/wet formant morphing.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackDualFormantFilter {
public:
    EurorackDualFormantFilter() {
        reset();
    }

    void reset() noexcept {
        f1_x1_ = 0; f1_x2_ = 0; f1_y1_ = 0; f1_y2_ = 0;
        f2_x1_ = 0; f2_x2_ = 0; f2_y1_ = 0; f2_y2_ = 0;
        b0_1_ = 8192; a1_1_ = 0; a2_1_ = 0;
        b0_2_ = 8192; a1_2_ = 0; a2_2_ = 0;
        blend_q15_ = math::FixedQ15::from_float(0.5f);
    }

    // Configure resonant formant band:
    // band: 0 = F1 formant (throat, 200 - 1000 Hz), 1 = F2 formant (mouth, 800 - 3000 Hz)
    void configure_band(size_t band, int16_t b0, int16_t a1, int16_t a2) noexcept {
        if (band == 0) {
            b0_1_ = b0; a1_1_ = a1; a2_1_ = a2;
        } else {
            b0_2_ = b0; a1_2_ = a1; a2_2_ = a2;
        }
    }

    void set_blend(math::FixedQ15 blend) noexcept {
        blend_q15_ = blend;
    }

    // Process a single audio sample through dual parallel vocal resonators
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        int64_t x = input.raw;

        // Band 1 Direct Form I
        int64_t y1 = (static_cast<int64_t>(b0_1_) * (x - f1_x2_)) >> 14;
        y1 -= (static_cast<int64_t>(a1_1_) * f1_y1_) >> 14;
        y1 -= (static_cast<int64_t>(a2_1_) * f1_y2_) >> 14;

        f1_x2_ = f1_x1_;
        f1_x1_ = static_cast<int16_t>(x);
        f1_y2_ = f1_y1_;
        f1_y1_ = static_cast<int16_t>(clamp16(y1));

        // Band 2 Direct Form I
        int64_t y2 = (static_cast<int64_t>(b0_2_) * (x - f2_x2_)) >> 14;
        y2 -= (static_cast<int64_t>(a1_2_) * f2_y1_) >> 14;
        y2 -= (static_cast<int64_t>(a2_2_) * f2_y2_) >> 14;

        f2_x2_ = f2_x1_;
        f2_x1_ = static_cast<int16_t>(x);
        f2_y2_ = f2_y1_;
        f2_y1_ = static_cast<int16_t>(clamp16(y2));

        // Morph between F1 and F2
        int64_t blended = ((y1 * (32767 - blend_q15_.raw)) >> 15) + ((y2 * blend_q15_.raw) >> 15);

        return math::FixedQ15(static_cast<int16_t>(clamp16(blended)));
    }

private:
    static int64_t clamp16(int64_t val) noexcept {
        if (val > 32767) return 32767;
        if (val < -32768) return -32768;
        return val;
    }

    int16_t f1_x1_{0}, f1_x2_{0}, f1_y1_{0}, f1_y2_{0};
    int16_t f2_x1_{0}, f2_x2_{0}, f2_y1_{0}, f2_y2_{0};
    int16_t b0_1_{8192}, a1_1_{0}, a2_1_{0};
    int16_t b0_2_{8192}, a1_2_{0}, a2_2_{0};
    math::FixedQ15 blend_q15_{math::FixedQ15::from_float(0.5f)};
};

} // namespace aether::embedded
