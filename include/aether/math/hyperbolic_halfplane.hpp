#pragma once

#include "../types.hpp"
#include <complex>
#include <vector>
#include <cmath>
#include <numbers>

namespace aether::math {

/**
 * @brief Poincare Upper Half-Plane Hyperbolic Geometry & Mobius Isometry Engine.
 *
 * Space: H^2 = { z = x + i*y in C : y > 0 }
 * Metric: ds^2 = (dx^2 + dy^2) / y^2
 * Isometry Group: PSL(2, R) acting by Mobius transformations z |-> (az + b)/(cz + d)
 */
class HyperbolicHalfPlane {
public:
    using Complex = std::complex<double>;

    struct MobiusTransform {
        double a{1.0};
        double b{0.0};
        double c{0.0};
        double d{1.0};

        [[nodiscard]] double determinant() const noexcept {
            return a * d - b * c;
        }

        [[nodiscard]] Complex apply(const Complex& z) const noexcept {
            const Complex num = a * z + b;
            const Complex den = c * z + d;
            if (std::abs(den) < 1e-12) {
                return Complex(0.0, 1e9);
            }
            return num / den;
        }

        [[nodiscard]] MobiusTransform inverse() const noexcept {
            const double det = determinant();
            if (std::abs(det) < 1e-12) {
                return MobiusTransform{1.0, 0.0, 0.0, 1.0};
            }
            return MobiusTransform{d / det, -b / det, -c / det, a / det};
        }

        [[nodiscard]] MobiusTransform compose(const MobiusTransform& other) const noexcept {
            return MobiusTransform{
                a * other.a + b * other.c,
                a * other.b + b * other.d,
                c * other.a + d * other.c,
                c * other.b + d * other.d
            };
        }
    };

    /**
     * @brief Cayley Transform: Isometry mapping Poincare Disk D to Upper Half-Plane H^2.
     *   w in D |-> z = i * (1 + w) / (1 - w) in H^2
     */
    static Complex disk_to_halfplane(const Complex& w) noexcept {
        if (std::abs(1.0 - w) < 1e-9) {
            return Complex(0.0, 1e9);
        }
        const Complex i(0.0, 1.0);
        return i * (1.0 + w) / (1.0 - w);
    }

    /**
     * @brief Inverse Cayley Transform: Upper Half-Plane H^2 to Poincare Disk D.
     *   z in H^2 |-> w = (z - i) / (z + i) in D
     */
    static Complex halfplane_to_disk(const Complex& z) noexcept {
        const Complex i(0.0, 1.0);
        if (std::abs(z + i) < 1e-9) {
            return Complex(1.0, 0.0);
        }
        return (z - i) / (z + i);
    }

    /**
     * @brief Geodesic distance between two points in H^2.
     *   d(z1, z2) = arcosh(1 + |z1 - z2|^2 / (2 * y1 * y2))
     */
    static double distance(const Complex& z1, const Complex& z2) noexcept {
        const double y1 = z1.imag();
        const double y2 = z2.imag();
        if (y1 <= 1e-9 || y2 <= 1e-9) return 0.0;

        const double diff2 = std::norm(z1 - z2);
        const double val = 1.0 + diff2 / (2.0 * y1 * y2);
        return std::acosh(std::max(1.0, val));
    }

    /**
     * @brief Generate an ensemble of Modular Group SL(2, Z) delay line taps for acoustic diffusion.
     *
     * Generators:
     *   T: z |-> z + 1  (translation)
     *   S: z |-> -1 / z  (inversion)
     */
    static std::vector<float> generate_modular_delay_taps(int num_taps, float base_delay_ms) {
        if (num_taps <= 0 || base_delay_ms <= 0.0f) {
            return {};
        }
        num_taps = std::min(num_taps, 1024);
        std::vector<float> taps;
        taps.reserve(num_taps);

        Complex z(0.0, 1.0); // Origin in H^2 (i)
        const MobiusTransform S{0.0, -1.0, 1.0, 0.0};
        const MobiusTransform T{1.0, 1.0, 0.0, 1.0};

        MobiusTransform current{1.0, 0.0, 0.0, 1.0};

        for (int i = 0; i < num_taps; ++i) {
            if (i % 2 == 0) {
                current = current.compose(T);
            } else {
                current = current.compose(S);
            }

            const Complex pt = current.apply(z);
            const double dist = distance(Complex(0.0, 1.0), pt);

            // Map geodesic distance to delay tap time
            const float tap_ms = base_delay_ms * (1.0f + static_cast<float>(dist) * 0.45f);
            taps.push_back(tap_ms);
        }

        return taps;
    }
};

} // namespace aether::math
