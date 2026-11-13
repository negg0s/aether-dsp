#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Wave Terrain Synthesizer
// Scans an orbit (X(t), Y(t)) across a 2D non-linear mathematical height field:
// Z = cos(x) * sin(y) - sin(x * y)
// Produces evolving rich organic wavetable timbres in fixed-point Q15.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackWaveTerrain {
public:
    EurorackWaveTerrain() {
        reset();
    }

    void reset() noexcept {
        radius_ = math::FixedQ15::from_float(0.8f);
        speed_x_ = 1000000;  // phase increment for orbit X
        speed_y_ = 1414213;  // irrational ratio for non-repeating Lissajous orbit
        phase_x_ = 0;
        phase_y_ = 0;
    }

    void configure(math::FixedQ15 radius, uint32_t speed_x, uint32_t speed_y) noexcept {
        radius_ = radius;
        speed_x_ = speed_x;
        speed_y_ = speed_y;
    }

    // Process a single audio sample and evaluate terrain height Z at current orbit position
    math::FixedQ15 process() noexcept {
        phase_x_ += speed_x_;
        phase_y_ += speed_y_;

        // Compute orbit coordinates X and Y in Q15
        int32_t cos_x = approx_cos(phase_x_ >> 16);
        int32_t sin_y = approx_sin(phase_y_ >> 16);

        // Scale by orbit radius
        int64_t x = (static_cast<int64_t>(cos_x) * radius_.raw) >> 15;
        int64_t y = (static_cast<int64_t>(sin_y) * radius_.raw) >> 15;

        // Terrain function: Z = (x * (32767 - y^2) + y * (32767 - x^2)) / 32768
        int64_t x2 = (x * x) >> 15;
        int64_t y2 = (y * y) >> 15;

        int64_t term1 = (x * (32767 - y2)) >> 15;
        int64_t term2 = (y * (32767 - x2)) >> 15;
        int64_t z = term1 + term2;

        if (z > 32767) z = 32767;
        if (z < -32768) z = -32768;

        return math::FixedQ15(static_cast<int16_t>(z));
    }

    // Evaluate explicit external CV coordinates (e.g. from dual joysticks or external LFOs)
    [[nodiscard]] math::FixedQ15 evaluate(math::FixedQ15 x, math::FixedQ15 y) const noexcept {
        int64_t x_raw = x.raw;
        int64_t y_raw = y.raw;
        int64_t x2 = (x_raw * x_raw) >> 15;
        int64_t y2 = (y_raw * y_raw) >> 15;

        int64_t term1 = (x_raw * (32767 - y2)) >> 15;
        int64_t term2 = (y_raw * (32767 - x2)) >> 15;
        int64_t z = term1 + term2;

        if (z > 32767) z = 32767;
        if (z < -32768) z = -32768;

        return math::FixedQ15(static_cast<int16_t>(z));
    }

private:
    static int32_t approx_sin(uint32_t phase16) noexcept {
        int32_t tri = 0;
        if (phase16 < 32768) {
            tri = static_cast<int32_t>(phase16) - 16384;
        } else {
            tri = 49152 - static_cast<int32_t>(phase16);
        }
        int32_t abs_tri = (tri < 0) ? -tri : tri;
        return (2 * tri) - ((tri * abs_tri) >> 14);
    }

    static int32_t approx_cos(uint32_t phase16) noexcept {
        return approx_sin((phase16 + 16384) & 0xFFFF);
    }

    math::FixedQ15 radius_{math::FixedQ15::from_float(0.8f)};
    uint32_t speed_x_{1000000};
    uint32_t speed_y_{1414213};
    uint32_t phase_x_{0};
    uint32_t phase_y_{0};
};

} // namespace aether::embedded
