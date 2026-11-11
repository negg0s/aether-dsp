#pragma once

#include "../types.hpp"
#include "../math/poincare.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

#if defined(__AVX2__) || defined(_M_AMD64)
#include <immintrin.h>
#define AETHER_HAS_AVX2 1
#else
#define AETHER_HAS_AVX2 0
#endif

namespace aether::engines {

/**
 * @brief Non-Euclidean Hyperbolic Waveguide 2D FDTD Audio Engine.
 *
 * Solves the wave equation on a 2D Riemannian manifold with constant negative curvature K = -1:
 *    d^2 u / dt^2 = c^2 * Delta_H(u) - gamma * (du / dt)
 *
 * In the Poincare conformal disk, the Laplace-Beltrami operator Delta_H scales the
 * Euclidean Laplacian by the inverse conformal factor squared:
 *    Delta_H u = ((1 - r^2)^2 / 4) * (d^2 u / dx^2 + d^2 u / dy^2)
 *
 * Fully accelerated with AVX2 SIMD vectorized 8-wide fused multiply-add kernels.
 */
class HyperbolicWaveguide {
public:
    static constexpr size_t GRID_DIM = 32; // 32x32 discrete grid inside unit disk
    static constexpr size_t NUM_CELLS = GRID_DIM * GRID_DIM;

    HyperbolicWaveguide() noexcept {
        init_grid();
        reset();
    }

    void reset() noexcept {
        std::fill(u_curr_.begin(), u_curr_.end(), 0.0f);
        std::fill(u_prev_.begin(), u_prev_.end(), 0.0f);
        std::fill(u_next_.begin(), u_next_.end(), 0.0f);
    }

    /**
     * @brief Configure waveguide physical parameters.
     * @param wave_speed Normalized wave speed c (0.1 to 0.65 for Courant stability)
     * @param damping Wave absorption damping coefficient gamma
     * @param curvature_scale Non-Euclidean curvature scale factor
     */
    void configure(float wave_speed = 0.45f, float damping = 0.003f, float curvature_scale = 1.0f) noexcept {
        c_ = std::clamp(wave_speed, 0.05f, 0.65f);
        damping_ = std::clamp(damping, 0.0001f, 0.05f);
        curvature_scale_ = std::clamp(curvature_scale, 0.1f, 3.0f);
    }

    /**
     * @brief Strike the hyperbolic space at a specific point inside the Poincare disk.
     */
    void strike(Vec2 strike_pos, float velocity = 0.8f, float radius = 0.12f) noexcept {
        if (!std::isfinite(strike_pos.x) || !std::isfinite(strike_pos.y)) {
            strike_pos = {0.0f, 0.0f};
        }
        if (!std::isfinite(velocity)) velocity = 0.8f;
        if (!std::isfinite(radius)) radius = 0.12f;
        velocity = std::clamp(velocity, -5.0f, 5.0f);
        radius = std::clamp(radius, 0.01f, 1.0f);

        const float r_sq = strike_pos.magnitude_squared();
        if (r_sq >= 0.95f) {
            strike_pos = strike_pos * (0.90f / std::sqrt(r_sq));
        }

        for (size_t y = 0; y < GRID_DIM; ++y) {
            for (size_t x = 0; x < GRID_DIM; ++x) {
                const size_t idx = y * GRID_DIM + x;
                if (!is_inside_disk_[idx]) continue;

                const Vec2 pos = cell_coords_[idx];
                const float dist = (pos - strike_pos).magnitude();

                if (dist < radius) {
                    const float profile = 0.5f * (1.0f + std::cos(static_cast<float>(constants::PI) * dist / radius));
                    u_curr_[idx] += velocity * profile;
                    u_prev_[idx] += velocity * profile * 0.95f;
                }
            }
        }
    }

    /**
     * @brief Advance simulation by one sample step and read stereo acoustic pressure.
     */
    [[nodiscard]] StereoFrame step(Vec2 pickup_left = {-0.4f, 0.2f}, Vec2 pickup_right = {0.4f, -0.2f}) noexcept {
        const float c2 = c_ * c_;
        const float half_damp = damping_ * 0.5f;
        const float inv_denom = 1.0f / (1.0f + half_damp);
        const float prev_factor = (1.0f - half_damp);

#if AETHER_HAS_AVX2
        // AVX2 8-wide Vectorized Hyperbolic FDTD Kernel
        const __m256 v_c2 = _mm256_set1_ps(c2);
        const __m256 v_curv = _mm256_set1_ps(curvature_scale_);
        const __m256 v_four = _mm256_set1_ps(4.0f);
        const __m256 v_two = _mm256_set1_ps(2.0f);
        const __m256 v_prev_fac = _mm256_set1_ps(prev_factor);
        const __m256 v_inv_denom = _mm256_set1_ps(inv_denom);

        for (size_t y = 1; y < GRID_DIM - 1; ++y) {
            const size_t row_offset = y * GRID_DIM;

            // Vectorized chunks of 8 (x = 1..8, 9..16, 17..24)
            for (size_t x = 1; x <= 17; x += 8) {
                const size_t idx = row_offset + x;

                // Load 8 contiguous cells
                const __m256 u_c = _mm256_loadu_ps(&u_curr_[idx]);
                const __m256 u_l = _mm256_loadu_ps(&u_curr_[idx - 1]);
                const __m256 u_r = _mm256_loadu_ps(&u_curr_[idx + 1]);
                const __m256 u_d = _mm256_loadu_ps(&u_curr_[idx - GRID_DIM]);
                const __m256 u_u = _mm256_loadu_ps(&u_curr_[idx + GRID_DIM]);

                // Euclidean Laplacian: (L + R + D + U) - 4 * C
                const __m256 lap_sum = _mm256_add_ps(_mm256_add_ps(u_l, u_r), _mm256_add_ps(u_d, u_u));
                const __m256 lap_e = _mm256_sub_ps(lap_sum, _mm256_mul_ps(v_four, u_c));

                // Scale by Poincare Laplace-Beltrami factor: lap_h = lap_e * lb * curv
                const __m256 lb = _mm256_loadu_ps(&laplace_beltrami_factors_[idx]);
                const __m256 lap_h = _mm256_mul_ps(_mm256_mul_ps(lap_e, lb), v_curv);

                // Wave update: (2 * u_c - prev_factor * u_p + c^2 * lap_h) * inv_denom
                const __m256 u_p = _mm256_loadu_ps(&u_prev_[idx]);
#if defined(__FMA__)
                __m256 term = _mm256_fmadd_ps(v_two, u_c, _mm256_mul_ps(v_c2, lap_h));
#else
                __m256 term = _mm256_add_ps(_mm256_mul_ps(v_two, u_c), _mm256_mul_ps(v_c2, lap_h));
#endif
                term = _mm256_sub_ps(term, _mm256_mul_ps(v_prev_fac, u_p));
                __m256 u_new = _mm256_mul_ps(term, v_inv_denom);

                // Boundary absorption dampening
                const __m256 pml = _mm256_loadu_ps(&boundary_absorption_[idx]);
                u_new = _mm256_mul_ps(u_new, pml);

                _mm256_storeu_ps(&u_next_[idx], u_new);
            }

            // Scalar tail for remainder interior cells (x = 25..30)
            for (size_t x = 25; x < GRID_DIM - 1; ++x) {
                const size_t idx = row_offset + x;
                if (!is_inside_disk_[idx]) continue;

                const float u_c = u_curr_[idx];
                const float laplacian_e = u_curr_[idx + 1] + u_curr_[idx - 1] +
                                          u_curr_[idx + GRID_DIM] + u_curr_[idx - GRID_DIM] -
                                          4.0f * u_c;
                const float lb_metric = laplace_beltrami_factors_[idx];
                const float laplacian_h = laplacian_e * lb_metric * curvature_scale_;

                float u_new = inv_denom * (2.0f * u_c - prev_factor * u_prev_[idx] + c2 * laplacian_h);
                u_new *= boundary_absorption_[idx];

                u_next_[idx] = u_new;
            }
        }
#else
        // Fallback scalar kernel
        for (size_t y = 1; y < GRID_DIM - 1; ++y) {
            for (size_t x = 1; x < GRID_DIM - 1; ++x) {
                const size_t idx = y * GRID_DIM + x;
                if (!is_inside_disk_[idx]) continue;

                const float u_c = u_curr_[idx];
                const float laplacian_e = u_curr_[idx + 1] + u_curr_[idx - 1] +
                                          u_curr_[idx + GRID_DIM] + u_curr_[idx - GRID_DIM] -
                                          4.0f * u_c;

                const float lb_metric = laplace_beltrami_factors_[idx];
                const float laplacian_h = laplacian_e * lb_metric * curvature_scale_;

                float u_new = inv_denom * (2.0f * u_c - prev_factor * u_prev_[idx] + c2 * laplacian_h);
                u_new *= boundary_absorption_[idx];

                u_next_[idx] = u_new;
            }
        }
#endif

        // Buffer cycle
        std::swap(u_prev_, u_curr_);
        std::swap(u_curr_, u_next_);

        // Readout pickups
        Sample left = sample_at(pickup_left);
        Sample right = sample_at(pickup_right);

        if (!std::isfinite(left) || !std::isfinite(right)) {
            reset();
            left = 0.0f;
            right = 0.0f;
        }

        return {left, right};
    }

    /**
     * @brief Bilinear sampling at continuous coordinate inside the disk.
     */
    [[nodiscard]] Sample sample_at(Vec2 pos) const noexcept {
        const float gx = (pos.x * 0.5f + 0.5f) * static_cast<float>(GRID_DIM - 1);
        const float gy = (pos.y * 0.5f + 0.5f) * static_cast<float>(GRID_DIM - 1);

        const int x0 = std::clamp(static_cast<int>(gx), 0, static_cast<int>(GRID_DIM - 2));
        const int y0 = std::clamp(static_cast<int>(gy), 0, static_cast<int>(GRID_DIM - 2));
        const int x1 = x0 + 1;
        const int y1 = y0 + 1;

        const float fx = gx - static_cast<float>(x0);
        const float fy = gy - static_cast<float>(y0);

        const float s00 = u_curr_[y0 * GRID_DIM + x0];
        const float s10 = u_curr_[y0 * GRID_DIM + x1];
        const float s01 = u_curr_[y1 * GRID_DIM + x0];
        const float s11 = u_curr_[y1 * GRID_DIM + x1];

        return (1.0f - fx) * (1.0f - fy) * s00 +
               fx * (1.0f - fy) * s10 +
               (1.0f - fx) * fy * s01 +
               fx * fy * s11;
    }

    [[nodiscard]] const std::array<float, NUM_CELLS>& state() const noexcept {
        return u_curr_;
    }

private:
    float c_{0.45f};
    float damping_{0.003f};
    float curvature_scale_{1.0f};

    alignas(64) std::array<float, NUM_CELLS> u_curr_{};
    alignas(64) std::array<float, NUM_CELLS> u_prev_{};
    alignas(64) std::array<float, NUM_CELLS> u_next_{};

    alignas(64) std::array<Vec2, NUM_CELLS> cell_coords_{};
    alignas(64) std::array<float, NUM_CELLS> laplace_beltrami_factors_{};
    alignas(64) std::array<float, NUM_CELLS> boundary_absorption_{};
    alignas(64) std::array<bool, NUM_CELLS> is_inside_disk_{};

    void init_grid() noexcept {
        const float inv_dim = 1.0f / static_cast<float>(GRID_DIM - 1);

        for (size_t y = 0; y < GRID_DIM; ++y) {
            for (size_t x = 0; x < GRID_DIM; ++x) {
                const size_t idx = y * GRID_DIM + x;
                const float nx = (static_cast<float>(x) * inv_dim) * 2.0f - 1.0f;
                const float ny = (static_cast<float>(y) * inv_dim) * 2.0f - 1.0f;

                const Vec2 pos{nx, ny};
                cell_coords_[idx] = pos;

                const float r = pos.magnitude();
                if (r < math::PoincareDisk::BOUNDARY_RADIUS) {
                    is_inside_disk_[idx] = true;
                    laplace_beltrami_factors_[idx] = math::PoincareDisk::laplace_beltrami_factor(pos);

                    if (r > 0.82f) {
                        const float ramp = (r - 0.82f) / (math::PoincareDisk::BOUNDARY_RADIUS - 0.82f);
                        boundary_absorption_[idx] = 1.0f - 0.08f * ramp * ramp;
                    } else {
                        boundary_absorption_[idx] = 0.9998f;
                    }
                } else {
                    is_inside_disk_[idx] = false;
                    laplace_beltrami_factors_[idx] = 0.0f;
                    boundary_absorption_[idx] = 0.0f;
                }
            }
        }
    }
};

} // namespace aether::engines
