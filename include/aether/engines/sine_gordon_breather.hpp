#pragma once

#include "../types.hpp"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <span>

namespace aether::engines {

/**
 * @brief Sine-Gordon Integrable Topological Soliton & Breather Acoustic Engine.
 *
 * Simulates the 1+1D nonlinear relativistic Sine-Gordon field equation:
 *   d^2\phi/dt^2 - c^2 d^2\phi/dx^2 + \omega_0^2 \sin(\phi) + \gamma d\phi/dt = 0
 *
 * Supports:
 *   - Symplectic Störmer-Verlet / Zabusky-Kruskal finite difference integration
 *   - Topological Kink-Antikink pair generation and collision dynamics
 *   - Localized pulsating Breather modes with subharmonic formant structures:
 *       \phi_B(x, t) = 4 \arctan[ (m/\omega) \sin(\omega t) / \cosh(m x) ]
 *   - Dual spatial pickup points yielding ultra-wide organic stereo soundscapes
 */
class SineGordonBreather {
public:
    static constexpr size_t GRID_SIZE = 128;

    explicit SineGordonBreather(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(sample_rate) {
        reset();
    }

    void reset() noexcept {
        phi_.fill(0.0f);
        phi_prev_.fill(0.0f);
        phi_next_.fill(0.0f);
        time_ = 0.0f;
    }

    /**
     * @brief Configure engine parameters.
     * @param wave_speed Spatial propagation velocity (Courant parameter)
     * @param potential_freq Nonlinear potential natural frequency \omega_0
     * @param damping Mechanical friction coefficient \gamma
     */
    void configure(float wave_speed = 0.42f, float potential_freq = 280.0f, float damping = 0.0008f) noexcept {
        c_ = std::clamp(wave_speed, 0.05f, 0.48f); // CFL stability bound < 0.5
        omega0_ = std::clamp(potential_freq, 20.0f, 4000.0f);
        damping_ = std::clamp(damping, 0.00005f, 0.02f);
    }

    /**
     * @brief Inject an exact analytical Breather localized state.
     * @param center Normalized position along the lattice [0, 1]
     * @param internal_freq Internal pulsation frequency \omega \in (0.1, 0.95)
     * @param amplitude Excitation scaling
     */
    void inject_breather(float center = 0.5f, float internal_freq = 0.75f, float amplitude = 1.0f) noexcept {
        const float w = std::clamp(internal_freq, 0.15f, 0.95f);
        const float m = std::sqrt(1.0f - w * w);
        const float x0 = center * static_cast<float>(GRID_SIZE);

        for (size_t i = 0; i < GRID_SIZE; ++i) {
            const float x = (static_cast<float>(i) - x0) * 0.18f;
            // Exact initial state at t = 0: \phi = 0, d\phi/dt = 4 m / cosh(m x)
            const float dphi_dt = amplitude * 4.0f * m / std::cosh(m * x);
            const float dt = 1.0f / static_cast<float>(sample_rate_);
            phi_[i] = 0.0f;
            phi_prev_[i] = -dphi_dt * dt; // backward step for velocity
        }
    }

    /**
     * @brief Inject a colliding Kink-Antikink pair.
     * @param separation Normalized distance between kink and antikink
     * @param velocity Relative approach speed \in (0.1, 0.9)
     */
    void inject_kink_antikink(float separation = 0.4f, float velocity = 0.6f) noexcept {
        const float v = std::clamp(velocity, 0.1f, 0.95f);
        const float gamma = 1.0f / std::sqrt(1.0f - v * v);
        const float d = separation * static_cast<float>(GRID_SIZE) * 0.5f;
        const float center = static_cast<float>(GRID_SIZE) * 0.5f;

        for (size_t i = 0; i < GRID_SIZE; ++i) {
            const float x = static_cast<float>(i) - center;
            // Superposition of Kink at -d and Antikink at +d
            const float arg_kink = gamma * (x + d) * 0.12f;
            const float arg_antikink = gamma * (x - d) * 0.12f;
            const float phi_k = 4.0f * std::atan(std::exp(arg_kink));
            const float phi_ak = 4.0f * std::atan(std::exp(-arg_antikink));
            phi_[i] = (phi_k - phi_ak);
            phi_prev_[i] = phi_[i];
        }
    }

    /**
     * @brief Strike the lattice with an impulsive localized force.
     * @param coord Normalized strike coordinate [0, 1]
     * @param velocity Strike velocity
     * @param width Spatial Gaussian footprint
     */
    void strike(float coord = 0.5f, float velocity = 0.8f, float width = 0.04f) noexcept {
        const float center = std::clamp(coord, 0.05f, 0.95f) * static_cast<float>(GRID_SIZE);
        const float inv_sigma2 = 1.0f / (2.0f * width * width * static_cast<float>(GRID_SIZE * GRID_SIZE));

        for (size_t i = 0; i < GRID_SIZE; ++i) {
            const float dx = static_cast<float>(i) - center;
            const float force = velocity * std::exp(-dx * dx * inv_sigma2);
            phi_[i] += force * 0.4f;
        }
    }

    /**
     * @brief Compute next time step and return stereo frame from dual spatial pickups.
     */
    StereoFrame process() noexcept {
        const float dt = 1.0f / static_cast<float>(sample_rate_);
        const float c2 = c_ * c_;
        const float omega0_rad = omega0_ * static_cast<float>(aether::constants::TWO_PI);
        const float dt2_omega0_sq = (omega0_rad * dt) * (omega0_rad * dt);
        const float damp_factor = 1.0f / (1.0f + damping_);

        // Fixed / absorbing boundary conditions at ends
        phi_next_[0] = 0.0f;
        phi_next_[GRID_SIZE - 1] = 0.0f;

        for (size_t i = 1; i < GRID_SIZE - 1; ++i) {
            // Spatial 2nd derivative (Laplacian in 1D)
            const float laplacian = phi_[i + 1] - 2.0f * phi_[i] + phi_[i - 1];

            // Sine-Gordon nonlinear potential restoring force: -\sin(\phi)
            const float nonlinear_force = -std::sin(phi_[i]) * dt2_omega0_sq;

            // Störmer-Verlet time-stepping with velocity damping
            const float acc = c2 * laplacian + nonlinear_force;
            phi_next_[i] = ((2.0f - damping_) * phi_[i] - phi_prev_[i] + acc) * damp_factor;
        }

        // Stereo pickups: Left at 38% of string, Right at 62%
        constexpr size_t PICKUP_L = static_cast<size_t>(GRID_SIZE * 0.38f);
        constexpr size_t PICKUP_R = static_cast<size_t>(GRID_SIZE * 0.62f);

        // Acoustic pressure is proportional to local velocity d\phi/dt
        const float out_l = (phi_next_[PICKUP_L] - phi_prev_[PICKUP_L]) * 0.5f;
        const float out_r = (phi_next_[PICKUP_R] - phi_prev_[PICKUP_R]) * 0.5f;

        // Shift time buffers
        phi_prev_ = phi_;
        phi_ = phi_next_;
        time_ += dt;

        return {std::clamp(out_l * 1.8f, -1.0f, 1.0f), std::clamp(out_r * 1.8f, -1.0f, 1.0f)};
    }

    void process_block(std::span<StereoFrame> output) noexcept {
        for (auto& frame : output) {
            frame = process();
        }
    }

    [[nodiscard]] float total_energy() const noexcept {
        float e_kinetic = 0.0f;
        float e_gradient = 0.0f;
        float e_potential = 0.0f;

        for (size_t i = 1; i < GRID_SIZE - 1; ++i) {
            const float v = (phi_[i] - phi_prev_[i]);
            const float grad = (phi_[i + 1] - phi_[i - 1]) * 0.5f;
            e_kinetic += 0.5f * v * v;
            e_gradient += 0.5f * c_ * c_ * grad * grad;
            e_potential += (1.0f - std::cos(phi_[i]));
        }
        return e_kinetic + e_gradient + e_potential;
    }

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    float c_{0.40f};
    float omega0_{260.0f};
    float damping_{0.0005f};
    float time_{0.0f};

    std::array<float, GRID_SIZE> phi_{};
    std::array<float, GRID_SIZE> phi_prev_{};
    std::array<float, GRID_SIZE> phi_next_{};
};

} // namespace aether::engines
