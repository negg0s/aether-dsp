#pragma once

#include "../types.hpp"
#include <cmath>
#include <complex>

namespace aether::math {

/**
 * @brief 2D Poincaré Conformal Disk Model Mathematics.
 *
 * In hyperbolic geometry with constant negative Gaussian curvature K = -1:
 * Space is represented inside the unit disk D = { z in C : |z| < 1 }.
 * The Riemannian metric tensor is:
 *    ds^2 = 4 * (dx^2 + dy^2) / (1 - (x^2 + y^2))^2
 *
 * As r -> 1, distances and area elements grow exponentially, causing acoustic
 * wavefronts to diverge exponentially without Euclidean modal resonance peaks.
 */
class PoincareDisk {
public:
    static constexpr float BOUNDARY_RADIUS = 0.985f; // Numerical stability boundary inside unit circle
    static constexpr float BOUNDARY_RADIUS_SQ = BOUNDARY_RADIUS * BOUNDARY_RADIUS;

    /**
     * @brief Computes the conformal factor lambda(z) = 2 / (1 - |z|^2)
     */
    [[nodiscard]] static inline float conformal_factor(Vec2 p) noexcept {
        const float r2 = std::min(p.magnitude_squared(), BOUNDARY_RADIUS_SQ);
        return 2.0f / (1.0f - r2);
    }

    /**
     * @brief Computes the inverse conformal factor squared:
     * (1 - r^2)^2 / 4, which scales the Euclidean Laplacian to the
     * Hyperbolic Laplace-Beltrami operator Delta_H.
     */
    [[nodiscard]] static inline float laplace_beltrami_factor(Vec2 p) noexcept {
        const float r2 = std::min(p.magnitude_squared(), BOUNDARY_RADIUS_SQ);
        const float diff = 1.0f - r2;
        return (diff * diff) * 0.25f;
    }

    /**
     * @brief Exact hyperbolic geodesic distance between two points in the Poincare disk:
     * d_H(z1, z2) = arcosh( 1 + 2 * |z1 - z2|^2 / ((1 - |z1|^2) * (1 - |z2|^2)) )
     */
    [[nodiscard]] static inline float geodesic_distance(Vec2 p1, Vec2 p2) noexcept {
        const float r1_sq = std::min(p1.magnitude_squared(), BOUNDARY_RADIUS_SQ);
        const float r2_sq = std::min(p2.magnitude_squared(), BOUNDARY_RADIUS_SQ);
        const float dist_sq = (p1 - p2).magnitude_squared();

        const float denom = (1.0f - r1_sq) * (1.0f - r2_sq);
        if (denom < 1e-7f) return 15.0f; // Saturated boundary distance

        const float delta = 1.0f + 2.0f * dist_sq / denom;
        if (!std::isfinite(delta)) return 15.0f;
        return std::acosh(std::max(1.0f, delta));
    }

    /**
     * @brief Mobius transformation mapping disk onto itself:
     * phi_a(z) = (z - a) / (1 - conj(a) * z)
     */
    [[nodiscard]] static inline Vec2 mobius_transform(Vec2 z, Vec2 a) noexcept {
        std::complex<float> cz(z.x, z.y);
        std::complex<float> ca(a.x, a.y);
        std::complex<float> num = cz - ca;
        std::complex<float> den = std::complex<float>(1.0f, 0.0f) - std::conj(ca) * cz;
        if (std::norm(den) < 1e-7f) return z;
        std::complex<float> res = num / den;
        if (!std::isfinite(res.real()) || !std::isfinite(res.imag())) return z;
        return {res.real(), res.imag()};
    }
};

} // namespace aether::math
