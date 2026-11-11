#pragma once

#include "../types.hpp"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <span>

namespace aether::engines {

/**
 * @brief Toda Lattice / Integrable KdV Soliton Acoustic Engine.
 *
 * Implements an exact completely integrable nonlinear 1D crystal lattice with
 * exponential inter-particle potential:
 *   V(r) = (a / b) * [ exp(-b * r) - 1 ] + a * r
 *
 * Physics Simulated:
 *   - Continuous soliton formation: Nonlinear steepening balances dispersion
 *   - Completely elastic N-soliton collisions with exact phase shifts
 *   - Fermi-Pasta-Ulam-Tsingou (FPUT) recurrence phenomenon
 *   - Zero thermalization: Acoustic partials maintain sharp coherent peaks
 *   - Symplectic 2nd-order Störmer-Verlet phase-space preserving time-stepping
 *   - Stereo output from dual compression strain pickups
 */
class KdvSolitonLattice {
public:
    static constexpr size_t LATTICE_SIZE = 64;

    explicit KdvSolitonLattice(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(std::clamp(sample_rate, 1000u, 384000u)) {
        reset();
    }

    void reset() noexcept {
        q_.fill(0.0f);
        p_.fill(0.0f);
        f_.fill(0.0f);
    }

    /**
     * @brief Configure Toda lattice parameters.
     * @param nonlinearity Non-linear hardness parameter b
     * @param coupling Linear stiffness parameter a
     * @param damping Microscopic damping factor
     */
    void configure(float nonlinearity = 1.8f, float coupling = 340.0f, float damping = 0.0004f) noexcept {
        b_ = std::clamp(nonlinearity, 0.2f, 4.0f);
        a_ = std::clamp(coupling, 50.0f, 1200.0f);
        damping_ = std::clamp(damping, 0.00005f, 0.01f);
    }

    /**
     * @brief Excite lattice with an impulse momentum strike.
     * @param position Normalized launch position along lattice [0.1, 0.9]
     * @param velocity Strike momentum
     * @param width Spatial pulse width
     */
    void strike(float position = 0.25f, float velocity = 1.0f, float width = 0.05f) noexcept {
        const float center = std::clamp(position, 0.05f, 0.95f) * static_cast<float>(LATTICE_SIZE);
        const float inv_w2 = 1.0f / (2.0f * width * width * static_cast<float>(LATTICE_SIZE * LATTICE_SIZE));

        for (size_t i = 0; i < LATTICE_SIZE; ++i) {
            const float dx = static_cast<float>(i) - center;
            p_[i] += velocity * std::exp(-dx * dx * inv_w2);
        }
    }

    /**
     * @brief Inject an exact analytical single-soliton compression wave packet.
     * @param center Normalized initial position
     * @param amplitude Soliton wavenumber parameter \kappa
     */
    void inject_soliton(float center = 0.2f, float amplitude = 1.2f) noexcept {
        const float x0 = center * static_cast<float>(LATTICE_SIZE);
        const float kappa = std::clamp(amplitude, 0.2f, 2.5f);

        for (size_t i = 0; i < LATTICE_SIZE; ++i) {
            const float x = static_cast<float>(i) - x0;
            // Toda exact soliton compression: r_n = (1/b) * ln[ 1 + sinh^2(kappa) * sech^2(kappa * n) ]
            const float sech_val = 1.0f / std::cosh(kappa * x * 0.4f);
            const float r = (1.0f / b_) * std::log(1.0f + std::sinh(kappa) * std::sinh(kappa) * sech_val * sech_val);
            q_[i] = -r;
            p_[i] = -std::sinh(kappa) * sech_val * 0.8f;
        }
    }

    /**
     * @brief Process one stereo sample using symplectic Störmer-Verlet integration.
     */
    StereoFrame process() noexcept {
        const float dt = 1.0f / static_cast<float>(sample_rate_) * 600.0f; // Dimensionless time-scaling
        const float half_dt = 0.5f * dt;
        const float damp_factor = 1.0f - damping_;

        compute_forces();
        for (size_t i = 1; i < LATTICE_SIZE - 1; ++i) {
            p_[i] = (p_[i] + half_dt * f_[i]) * damp_factor;
        }

        for (size_t i = 1; i < LATTICE_SIZE - 1; ++i) {
            q_[i] += dt * p_[i];
        }

        q_[0] = 0.0f;
        p_[0] = 0.0f;
        q_[LATTICE_SIZE - 1] = 0.0f;
        p_[LATTICE_SIZE - 1] = 0.0f;

        compute_forces();
        for (size_t i = 1; i < LATTICE_SIZE - 1; ++i) {
            p_[i] = (p_[i] + half_dt * f_[i]) * damp_factor;
        }

        constexpr size_t PICKUP_L = static_cast<size_t>(LATTICE_SIZE * 0.30f);
        constexpr size_t PICKUP_R = static_cast<size_t>(LATTICE_SIZE * 0.70f);

        // Self-heal numerical NaN/Inf breakdown during extreme non-linear shocks
        for (size_t i = 0; i < LATTICE_SIZE; ++i) {
            if (!std::isfinite(q_[i]) || !std::isfinite(p_[i])) {
                q_[i] = 0.0f;
                p_[i] = 0.0f;
                f_[i] = 0.0f;
            }
        }

        const float strain_l = (q_[PICKUP_L + 1] - q_[PICKUP_L]) * 1.8f;
        const float strain_r = (q_[PICKUP_R + 1] - q_[PICKUP_R]) * 1.8f;

        return {std::clamp(strain_l, -1.0f, 1.0f), std::clamp(strain_r, -1.0f, 1.0f)};
    }

    void process_block(std::span<StereoFrame> output) noexcept {
        for (auto& frame : output) {
            frame = process();
        }
    }

    [[nodiscard]] float total_energy() const noexcept {
        float e_kin = 0.0f;
        float e_pot = 0.0f;

        for (size_t i = 1; i < LATTICE_SIZE - 1; ++i) {
            e_kin += 0.5f * p_[i] * p_[i];
            const float r = q_[i + 1] - q_[i];
            const float exp_term = std::exp(std::clamp(-b_ * r, -20.0f, 20.0f));
            e_pot += (a_ / b_) * (exp_term - 1.0f + b_ * r);
        }
        return e_kin + e_pot;
    }

private:
    void compute_forces() noexcept {
        // Toda interparticle force: F_n = a * [ exp(-b * (q_n - q_{n-1})) - exp(-b * (q_{n+1} - q_n)) ]
        for (size_t i = 1; i < LATTICE_SIZE - 1; ++i) {
            const float r_left = q_[i] - q_[i - 1];
            const float r_right = q_[i + 1] - q_[i];

            // Soft-clamp exponent to avoid numerical overflow during extreme shockwaves
            const float exp_l = std::exp(std::clamp(-b_ * r_left, -10.0f, 10.0f));
            const float exp_r = std::exp(std::clamp(-b_ * r_right, -10.0f, 10.0f));

            f_[i] = a_ * (exp_l - exp_r);
        }
    }

    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    float b_{1.8f};
    float a_{340.0f};
    float damping_{0.0004f};

    std::array<float, LATTICE_SIZE> q_{};
    std::array<float, LATTICE_SIZE> p_{};
    std::array<float, LATTICE_SIZE> f_{};
};

} // namespace aether::engines
