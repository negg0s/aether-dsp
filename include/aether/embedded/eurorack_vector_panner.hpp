#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Microtonal Vector Interpolator & Joystiq Panner
// 2D Cartesian (X, Y) to 4-Corner (A, B, C, D) Planar / Vector synthesis panner
// Uses equal-power circular laws or linear coordinate weighting in Q15.
// Zero-heap, zero-allocation bare-metal MCU vector module (similar to Prophet VS / Planar 2).
class EurorackVectorPanner {
public:
    enum class PanningLaw : uint8_t {
        Linear = 0,
        EqualPower
    };

    EurorackVectorPanner() {
        set_law(PanningLaw::Linear);
        set_position(math::FixedQ15(0), math::FixedQ15(0));
    }

    void set_law(PanningLaw law) noexcept {
        law_ = law;
    }

    // Set 2D joystick coordinates: X and Y from -1.0 to +1.0 in Q15
    // (-1, -1) -> Corner A (South-West)
    // (+1, -1) -> Corner B (South-East)
    // (-1, +1) -> Corner C (North-West)
    // (+1, +1) -> Corner D (North-East)
    void set_position(math::FixedQ15 x, math::FixedQ15 y) noexcept {
        // Map x, y from [-1.0, 1.0] to [0.0, 1.0]: u = (x + 1)/2, v = (y + 1)/2
        int32_t u = (static_cast<int32_t>(x.raw) + 32768) >> 1;
        int32_t v = (static_cast<int32_t>(y.raw) + 32768) >> 1;

        if (u < 0) u = 0;
        if (u > 32767) u = 32767;
        if (v < 0) v = 0;
        if (v > 32767) v = 32767;

        int32_t inv_u = 32767 - u;
        int32_t inv_v = 32767 - v;

        if (law_ == PanningLaw::Linear) {
            // A = (1 - u) * (1 - v)
            // B = u * (1 - v)
            // C = (1 - u) * v
            // D = u * v
            gain_a_ = math::FixedQ15(static_cast<int16_t>((inv_u * inv_v) >> 15));
            gain_b_ = math::FixedQ15(static_cast<int16_t>((u * inv_v) >> 15));
            gain_c_ = math::FixedQ15(static_cast<int16_t>((inv_u * v) >> 15));
            gain_d_ = math::FixedQ15(static_cast<int16_t>((u * v) >> 15));
        } else {
            // Equal Power approximation: gain ~ sqrt(weight) via quadratic polynomial
            gain_a_ = approx_sqrt((inv_u * inv_v) >> 15);
            gain_b_ = approx_sqrt((u * inv_v) >> 15);
            gain_c_ = approx_sqrt((inv_u * v) >> 15);
            gain_d_ = approx_sqrt((u * v) >> 15);
        }
    }

    struct QuadOutput {
        math::FixedQ15 a{0};
        math::FixedQ15 b{0};
        math::FixedQ15 c{0};
        math::FixedQ15 d{0};
    };

    // Distribute a single input into 4 quad outputs
    [[nodiscard]] QuadOutput pan(math::FixedQ15 input) const noexcept {
        QuadOutput out;
        out.a = input * gain_a_;
        out.b = input * gain_b_;
        out.c = input * gain_c_;
        out.d = input * gain_d_;
        return out;
    }

    // Mix 4 corner inputs into a single composite vector output
    [[nodiscard]] math::FixedQ15 mix(math::FixedQ15 in_a, math::FixedQ15 in_b, math::FixedQ15 in_c, math::FixedQ15 in_d) const noexcept {
        int64_t sum = static_cast<int64_t>((in_a * gain_a_).raw) +
                      static_cast<int64_t>((in_b * gain_b_).raw) +
                      static_cast<int64_t>((in_c * gain_c_).raw) +
                      static_cast<int64_t>((in_d * gain_d_).raw);

        if (sum > 32767) sum = 32767;
        if (sum < -32768) sum = -32768;
        return math::FixedQ15(static_cast<int16_t>(sum));
    }

    [[nodiscard]] QuadOutput get_gains() const noexcept {
        return {gain_a_, gain_b_, gain_c_, gain_d_};
    }

private:
    static math::FixedQ15 approx_sqrt(int32_t val_q15) noexcept {
        if (val_q15 <= 0) return math::FixedQ15(0);
        if (val_q15 >= 32767) return math::FixedQ15(32767);
        // Integer square root via bit shifting
        uint32_t rem = static_cast<uint32_t>(val_q15) << 15;
        uint32_t root = 0;
        for (uint32_t bit = 1U << 30; bit > 0; bit >>= 2) {
            if (rem >= root + bit) {
                rem -= root + bit;
                root = (root >> 1) + bit;
            } else {
                root >>= 1;
            }
        }
        if (root > 32767) root = 32767;
        return math::FixedQ15(static_cast<int16_t>(root));
    }

    PanningLaw law_{PanningLaw::Linear};
    math::FixedQ15 gain_a_{0};
    math::FixedQ15 gain_b_{0};
    math::FixedQ15 gain_c_{0};
    math::FixedQ15 gain_d_{0};
};

} // namespace aether::embedded
