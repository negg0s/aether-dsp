#pragma once

#include "../types.hpp"
#include "poincare.hpp"
#include <vector>
#include <complex>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace aether::math {

/**
 * @brief Bolza Surface (Genus g = 2 Compact Riemann Surface H^2 / Gamma).
 *
 * Implements the geometry, Fuchsian side-pairing Möbius isometries, and
 * chaotic geodesic flow on the most symmetric compact hyperbolic surface.
 *
 * Properties:
 *   - Fundamental domain: Regular hyperbolic octagon in Poincare disk D.
 *   - Vertex radius: R_v = 2^(-1/4) ~= 0.8408964
 *   - All 8 interior angles are exactly pi/4 (45 degrees), summing to 2*pi.
 *   - Euler characteristic: chi = 2 - 2g = -2.
 *   - Total hyperbolic surface area: Area = 4*pi (Gauss-Bonnet).
 *   - Opposite side pairing: side k is mapped to side (k + 4) mod 8 via
 *     Fuchsian translation generators in PSL(2, C) preserving the unit disk.
 */
class BolzaSurface {
public:
    using Complex = std::complex<float>;

    static constexpr float VERTEX_RADIUS = 0.84089641525f; // 2^(-1/4)
    static constexpr size_t NUM_SIDES = 8;

    struct RayArrival {
        float time_sec{0.0f};
        float amplitude{0.0f};
        float azimuth_rad{0.0f};
    };

    /**
     * @brief Get coordinates of the 8 vertices of the fundamental regular octagon.
     */
    [[nodiscard]] static std::vector<Vec2> get_octagon_vertices() {
        std::vector<Vec2> vertices(NUM_SIDES);
        constexpr float PI = std::numbers::pi_v<float>;

        for (size_t k = 0; k < NUM_SIDES; ++k) {
            const float angle = (2.0f * k + 1.0f) * (PI / 8.0f);
            vertices[k] = Vec2{
                VERTEX_RADIUS * std::cos(angle),
                VERTEX_RADIUS * std::sin(angle)
            };
        }
        return vertices;
    }

    /**
     * @brief Check whether a point in the Poincare disk lies within the Bolza fundamental octagon.
     */
    [[nodiscard]] static bool is_inside_fundamental_domain(Vec2 p) noexcept {
        const auto verts = get_octagon_vertices();
        for (size_t i = 0; i < NUM_SIDES; ++i) {
            const auto& v1 = verts[i];
            const auto& v2 = verts[(i + 1) % NUM_SIDES];

            const float cross = (v2.x - v1.x) * (p.y - v1.y) - (v2.y - v1.y) * (p.x - v1.x);
            if (cross < -1e-4f) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Apply Fuchsian side-pairing Möbius isometry when crossing side k.
     * Maps edge k to opposite edge (k + 4) mod 8, wrapping the acoustic wavefront
     * across the compact genus-2 manifold without boundaries.
     */
    [[nodiscard]] static Vec2 wrap_point(Vec2 p, size_t side_k) noexcept {
        constexpr float PI = std::numbers::pi_v<float>;
        // Angle of side k normal: theta_k = k * (2*pi / 8)
        const float theta_k = static_cast<float>(side_k) * (PI / 4.0f);

        // Hyperbolic translation distance across the octagon: d = 2 * arcosh(1 + sqrt(2)) ~= 3.057
        constexpr float TRANS_DIST = 3.0573068f;
        const float tanh_half_d = std::tanh(TRANS_DIST * 0.5f);

        // Standard hyperbolic translation along direction theta_k
        const Complex a(std::cos(theta_k) * tanh_half_d, std::sin(theta_k) * tanh_half_d);
        const Complex z(p.x, p.y);

        // Poincare disk translation: T(z) = (z - a) / (1 - conj(a) * z)
        const Complex numerator = z - a;
        const Complex denominator = Complex(1.0f, 0.0f) - std::conj(a) * z;
        const Complex wrapped = numerator / denominator;

        return Vec2{wrapped.real(), wrapped.imag()};
    }

    /**
     * @brief Synthesize a Manifold Impulse Response (MIR) for the closed Bolza Universe.
     * Computes chaotic ergodic geodesic rays circulating through the genus-2 compact manifold.
     */
    [[nodiscard]] static std::vector<float> synthesize_bolza_ir(uint32_t sample_rate, float duration_sec = 1.5f) {
        const size_t total_samples = static_cast<size_t>(sample_rate * duration_sec);
        std::vector<float> ir(total_samples, 0.0f);

        constexpr size_t NUM_RAYS = 96;
        constexpr float DT = 0.0004f;
        const size_t steps = static_cast<size_t>(duration_sec / DT);

        const Vec2 receiver{0.15f, -0.2f};
        constexpr float CAPTURE_RADIUS = 0.08f;

        for (size_t ray = 0; ray < NUM_RAYS; ++ray) {
            const float angle = (2.0f * std::numbers::pi_v<float> * ray) / NUM_RAYS;
            Vec2 pos{0.0f, 0.0f}; // Source at origin
            Vec2 vel{std::cos(angle) * 0.45f, std::sin(angle) * 0.45f};

            float energy = 1.0f;
            float elapsed_time = 0.0f;

            for (size_t s = 0; s < steps; ++s) {
                // Hyperbolic geodesic step: conformal factor lambda = 2 / (1 - ||p||^2)
                const float r2 = pos.x * pos.x + pos.y * pos.y;
                const float cf = 2.0f / (1.0f - std::min(r2, 0.95f));

                pos.x += vel.x * (DT / cf);
                pos.y += vel.y * (DT / cf);
                elapsed_time += DT;

                const float dist_center = std::sqrt(pos.x * pos.x + pos.y * pos.y);
                if (dist_center >= VERTEX_RADIUS * 0.92f) {
                    const float ray_ang = std::atan2(pos.y, pos.x);
                    const int side = std::clamp(static_cast<int>((ray_ang + std::numbers::pi_v<float>) / (std::numbers::pi_v<float> / 4.0f)), 0, 7);
                    pos = wrap_point(pos, static_cast<size_t>(side));
                    energy *= 0.965f;
                }

                const float rx = pos.x - receiver.x;
                const float ry = pos.y - receiver.y;
                const float dist_rec = std::sqrt(rx * rx + ry * ry);

                if (dist_rec < CAPTURE_RADIUS) {
                    const size_t smp = static_cast<size_t>(elapsed_time * sample_rate);
                    if (smp < total_samples) {
                        const float gain = energy * (1.0f - dist_rec / CAPTURE_RADIUS) * 0.15f;
                        ir[smp] += (ray % 2 == 0 ? gain : -gain);
                    }
                }

                energy *= 0.9997f;
                if (energy < 1e-4f) break;
            }
        }

        // Normalize IR
        float max_peak = 1e-6f;
        for (float val : ir) {
            max_peak = std::max(max_peak, std::abs(val));
        }
        if (max_peak > 1e-4f) {
            for (float& val : ir) {
                val /= max_peak;
            }
        }

        return ir;
    }
};

} // namespace aether::math
