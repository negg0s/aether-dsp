#pragma once

#include "../types.hpp"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <span>

namespace aether::engines {

/**
 * @brief Topological Acoustic Phononic Metamaterial (Valley-Hall Dirac Cone Engine).
 *
 * Implements a 2D honeycomb/hexagonal phononic crystal with broken inversion symmetry
 * exhibiting Dirac cones and topologically protected acoustic edge states:
 *   H = v_D ( \delta k_x \sigma_x + \delta k_y \sigma_y ) + m \sigma_z
 *
 * Physics Simulated:
 *   - Two triangular sublattices (A and B) with mass contrast \Delta m = m_A - m_B
 *   - Dirac Cone dispersion at K/K' points when \Delta m = 0
 *   - Topological bulk bandgap opening with valley Chern number C_v = \pm 1/2
 *   - Domain wall interface connecting inverted topological domains (\Delta m > 0 / \Delta m < 0)
 *   - Backscattering-immune acoustic propagation along the topological interface
 *   - Stereo output: Left = Edge State Interface Output, Right = Bulk Phononic Leakage
 */
class TopologicalMetamaterial {
public:
    static constexpr size_t WIDTH = 32;
    static constexpr size_t HEIGHT = 16;
    static constexpr size_t TOTAL_NODES = WIDTH * HEIGHT;

    explicit TopologicalMetamaterial(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(std::clamp(sample_rate, 1000u, 384000u)) {
        init_lattice();
        reset();
    }

    void reset() noexcept {
        u_.fill(0.0f);
        u_prev_.fill(0.0f);
        u_next_.fill(0.0f);
    }

    /**
     * @brief Configure metamaterial bandgap and Dirac coupling.
     * @param inversion_contrast Sublattice mass perturbation \delta (controls topological bandgap)
     * @param dirac_velocity Dirac cone slope v_D (wavegroup speed)
     * @param damping Viscoelastic acoustic damping
     */
    void configure(float inversion_contrast = 0.45f, float dirac_velocity = 0.38f, float damping = 0.0005f) noexcept {
        delta_ = std::clamp(inversion_contrast, 0.0f, 0.85f);
        v_d_ = std::clamp(dirac_velocity, 0.1f, 0.48f);
        damping_ = std::clamp(damping, 0.00005f, 0.02f);
        init_lattice();
    }

    /**
     * @brief Strike the topological edge channel at the domain wall interface.
     * @param velocity Strike velocity
     */
    void strike_edge(float velocity = 0.95f) noexcept {
        constexpr size_t INTERFACE_Y = HEIGHT / 2;
        // Inject excitation at left end of interface (x = 2..4)
        for (size_t x = 1; x <= 4; ++x) {
            const size_t idx = INTERFACE_Y * WIDTH + x;
            const float diff = static_cast<float>(x) - 2.0f;
            const float pulse = velocity * std::exp(-std::abs(diff) * 0.5f);
            u_[idx] += pulse;
            u_prev_[idx] += pulse * 0.5f;
        }
    }

    /**
     * @brief Strike the bulk domain (inducing evanescent decay inside topological bandgap).
     */
    void strike_bulk(float velocity = 0.85f) noexcept {
        constexpr size_t BULK_Y = 3;
        constexpr size_t BULK_X = WIDTH / 2;
        const size_t idx = BULK_Y * WIDTH + BULK_X;
        u_[idx] += velocity;
        u_prev_[idx] += velocity * 0.5f;
    }

    /**
     * @brief Process one stereo sample.
     * Left channel: Topologically protected edge channel exit.
     * Right channel: Bulk acoustic radiation.
     */
    StereoFrame process() noexcept {
        const float rate_scale = static_cast<float>(DEFAULT_SAMPLE_RATE) / static_cast<float>(sample_rate_);
        const float effective_vd = std::min(v_d_ * rate_scale, 0.48f);
        const float c2 = effective_vd * effective_vd;
        const float effective_damping = damping_ * rate_scale;
        const float damp_factor = 1.0f / (1.0f + effective_damping);

        // 2D Finite Difference on hexagonal/staggered acoustic lattice
        for (size_t y = 1; y < HEIGHT - 1; ++y) {
            for (size_t x = 1; x < WIDTH - 1; ++x) {
                const size_t idx = y * WIDTH + x;

                // 2D Laplacian coupling to 4 nearest neighbors
                const float laplacian = u_[idx + 1] + u_[idx - 1] +
                                        u_[idx + WIDTH] + u_[idx - WIDTH] - 4.0f * u_[idx];

                // Topological mass potential restoring term V(x, y) * u
                const float m_term = mass_profile_[idx] * 0.08f * u_[idx] * rate_scale * rate_scale;

                const float updated = ((2.0f - effective_damping) * u_[idx] - u_prev_[idx] + c2 * laplacian - m_term) * damp_factor;
                u_next_[idx] = std::isfinite(updated) ? std::clamp(updated, -10.0f, 10.0f) : 0.0f;
            }
        }

        // Absorbing boundaries at outer edges
        for (size_t x = 0; x < WIDTH; ++x) {
            u_next_[x] = u_[WIDTH + x] * 0.85f;
            u_next_[(HEIGHT - 1) * WIDTH + x] = u_[(HEIGHT - 2) * WIDTH + x] * 0.85f;
        }
        for (size_t y = 0; y < HEIGHT; ++y) {
            u_next_[y * WIDTH] = u_[y * WIDTH + 1] * 0.85f;
            u_next_[y * WIDTH + (WIDTH - 1)] = u_[y * WIDTH + (WIDTH - 2)] * 0.85f;
        }

        // Stereo pickups:
        // Left: Edge state output at the right side of the interface
        constexpr size_t INTERFACE_Y = HEIGHT / 2;
        constexpr size_t EXIT_X = WIDTH - 4;
        const size_t EDGE_PICKUP = INTERFACE_Y * WIDTH + EXIT_X;

        // Right: Bulk domain pickup
        constexpr size_t BULK_Y = HEIGHT / 4;
        constexpr size_t BULK_X = WIDTH / 2;
        const size_t BULK_PICKUP = BULK_Y * WIDTH + BULK_X;

        const float out_edge = (u_next_[EDGE_PICKUP] - u_prev_[EDGE_PICKUP]) * 2.5f;
        const float out_bulk = (u_next_[BULK_PICKUP] - u_prev_[BULK_PICKUP]) * 2.5f;

        u_prev_ = u_;
        u_ = u_next_;

        return {std::clamp(out_edge, -1.0f, 1.0f), std::clamp(out_bulk, -1.0f, 1.0f)};
    }

    void process_block(std::span<StereoFrame> output) noexcept {
        for (auto& frame : output) {
            frame = process();
        }
    }

    [[nodiscard]] float total_energy() const noexcept {
        float e = 0.0f;
        for (size_t i = 0; i < TOTAL_NODES; ++i) {
            const float v = u_[i] - u_prev_[i];
            e += v * v;
        }
        return e;
    }

private:
    void init_lattice() noexcept {
        constexpr size_t INTERFACE_Y = HEIGHT / 2;
        for (size_t y = 0; y < HEIGHT; ++y) {
            for (size_t x = 0; x < WIDTH; ++x) {
                const size_t idx = y * WIDTH + x;
                // Sublattice parity: A sublattice if (x + y) even, B if odd
                const float parity = ((x + y) % 2 == 0) ? 1.0f : -1.0f;
                // Domain inversion: Upper half has \delta > 0, Lower half has \delta < 0
                const float domain_sign = (y < INTERFACE_Y) ? 1.0f : -1.0f;

                // Mass profile creates topological domain wall at y = INTERFACE_Y
                mass_profile_[idx] = domain_sign * parity * delta_;
            }
        }
    }

    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    float delta_{0.45f};
    float v_d_{0.38f};
    float damping_{0.0005f};

    std::array<float, TOTAL_NODES> mass_profile_{};
    std::array<float, TOTAL_NODES> u_{};
    std::array<float, TOTAL_NODES> u_prev_{};
    std::array<float, TOTAL_NODES> u_next_{};
};

} // namespace aether::engines
