#pragma once

#include "../types.hpp"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <span>
#include <random>

namespace aether::engines {

/**
 * @brief Acoustic Black Hole (ABH) & Sonic Horizon Waveguide Synthesizer.
 *
 * Implements a retarding acoustic metamaterial waveguide where local phase velocity
 * and acoustic impedance taper according to a power-law toward a sonic event horizon:
 *   c(x) = c_0 * (x / L)^\alpha + c_min
 *
 * Physics Simulated:
 *   - Asymptotic wave slowing (v_g -> 0 as x -> x_horizon)
 *   - Spatial wave compression (k(x) -> infinity)
 *   - Non-reflective impedance absorption (Acoustic Black Hole effect)
 *   - Stimulated acoustic Hawking-Unruh thermal phonon emission at horizon
 *   - Doppler shift across supersonic flow boundary (c_eff = c_sound - v_flow)
 */
class AcousticBlackHole {
public:
    static constexpr size_t LATTICE_SIZE = 96;

    explicit AcousticBlackHole(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(sample_rate) {
        init_lattice();
        reset();
    }

    void reset() noexcept {
        u_.fill(0.0f);
        u_prev_.fill(0.0f);
        u_next_.fill(0.0f);
        thermal_phase_ = 0.0f;
    }

    /**
     * @brief Configure Acoustic Black Hole profile.
     * @param power_law Tapering exponent \alpha (typically 2.0 to 3.0)
     * @param horizon_pos Normalized position of horizon [0.05, 0.5]
     * @param hawking_coupling Strength of thermal phonon emission at horizon
     * @param terminal_damping Viscoelastic damping layer near horizon tip
     */
    void configure(float power_law = 2.2f, float horizon_pos = 0.12f,
                   float hawking_coupling = 0.008f, float terminal_damping = 0.04f) noexcept {
        alpha_ = std::clamp(power_law, 1.2f, 4.0f);
        horizon_pos_ = std::clamp(horizon_pos, 0.02f, 0.40f);
        hawking_coupling_ = std::clamp(hawking_coupling, 0.0f, 0.05f);
        terminal_damping_ = std::clamp(terminal_damping, 0.001f, 0.15f);
        init_lattice();
    }

    /**
     * @brief Excite waveguide with incident acoustic pulse at entry or throat.
     * @param position Normalized launch position [0.2, 0.95]
     * @param velocity Strike velocity
     */
    void strike(float position = 0.85f, float velocity = 0.9f) noexcept {
        const float pos_idx = std::clamp(position, 0.1f, 0.95f) * static_cast<float>(LATTICE_SIZE);
        const size_t center = static_cast<size_t>(pos_idx);

        for (size_t i = 0; i < LATTICE_SIZE; ++i) {
            const float dx = static_cast<float>(i) - static_cast<float>(center);
            const float pulse = velocity * std::exp(-dx * dx * 0.15f);
            u_[i] += pulse;
            u_prev_[i] += pulse * 0.5f;
        }
    }

    /**
     * @brief Process one stereo sample.
     * Left channel: Outer asymptotic radiation (Hawking/reflected wave).
     * Right channel: Near-horizon compressed acoustic field.
     */
    StereoFrame process() noexcept {
        const size_t horizon_idx = static_cast<size_t>(horizon_pos_ * static_cast<float>(LATTICE_SIZE));

        // Fixed / open boundary at entrance (x = LATTICE_SIZE - 1)
        u_next_[LATTICE_SIZE - 1] = u_[LATTICE_SIZE - 2] * 0.88f;

        // Black hole horizon boundary (x = 0): asymptotic acoustic sink
        u_next_[0] = u_[1] * 0.15f; // Strong anechoic termination

        // Interior FDTD wave propagation with spatially varying wave speed c[i]
        for (size_t i = 1; i < LATTICE_SIZE - 1; ++i) {
            const float c_local = c_profile_[i];
            const float d_local = damping_profile_[i];

            // 1D spatial derivative with impedance grading: d/dx ( c(x)^2 du/dx )
            const float du_dx_right = (u_[i + 1] - u_[i]);
            const float du_dx_left  = (u_[i] - u_[i - 1]);
            const float c_mid_r = 0.5f * (c_profile_[i + 1] + c_local);
            const float c_mid_l = 0.5f * (c_local + c_profile_[i - 1]);
            
            const float flux = (c_mid_r * c_mid_r * du_dx_right - c_mid_l * c_mid_l * du_dx_left);

            // Thermal Hawking-Unruh phonon fluctuation injection near horizon
            float hawking_noise = 0.0f;
            if (i <= horizon_idx + 4) {
                // Pseudo-random thermal fluctuation scaled by local gradient
                hawking_noise = hawking_coupling_ * grad_profile_[i] * pseudo_random();
            }

            const float denom = 1.0f / (1.0f + d_local);
            u_next_[i] = ((2.0f - d_local) * u_[i] - u_prev_[i] + flux + hawking_noise) * denom;
        }

        // Stereo pickups:
        // Left: Far-field outward traveling wave (entrance)
        constexpr size_t OUT_PICKUP = static_cast<size_t>(LATTICE_SIZE * 0.82f);
        // Right: Near-horizon event boundary
        const size_t HORIZON_PICKUP = std::max<size_t>(1, horizon_idx + 2);

        const float out_l = (u_next_[OUT_PICKUP] - u_prev_[OUT_PICKUP]) * 1.5f;
        const float out_r = (u_next_[HORIZON_PICKUP] - u_prev_[HORIZON_PICKUP]) * 2.2f;

        u_prev_ = u_;
        u_ = u_next_;

        return {std::clamp(out_l, -1.0f, 1.0f), std::clamp(out_r, -1.0f, 1.0f)};
    }

    void process_block(std::span<StereoFrame> output) noexcept {
        for (auto& frame : output) {
            frame = process();
        }
    }

private:
    void init_lattice() noexcept {
        const float c_max = 0.45f;  // Maximum Courant speed at entrance
        const float c_min = 0.035f; // Retarded speed near horizon

        for (size_t i = 0; i < LATTICE_SIZE; ++i) {
            const float norm_x = static_cast<float>(i) / static_cast<float>(LATTICE_SIZE - 1);
            
            // Power-law profile: c(x) = (c_max - c_min) * x^\alpha + c_min
            c_profile_[i] = (c_max - c_min) * std::pow(norm_x, alpha_) + c_min;

            // Spatial velocity gradient dc/dx (governs Hawking acoustic temperature)
            grad_profile_[i] = alpha_ * (c_max - c_min) * std::pow(std::max(norm_x, 0.01f), alpha_ - 1.0f);

            // Damping profile: exponential viscoelastic dissipation concentrated at horizon tip (x -> 0)
            damping_profile_[i] = 0.0003f + terminal_damping_ * std::exp(-norm_x * 12.0f);
        }
    }

    float pseudo_random() noexcept {
        // Fast deterministic LCG generator for real-time zero-heap thermal fluctuations
        rng_state_ = rng_state_ * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(rng_state_)) / 2147483648.0f;
    }

    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    float alpha_{2.2f};
    float horizon_pos_{0.12f};
    float hawking_coupling_{0.008f};
    float terminal_damping_{0.04f};
    float thermal_phase_{0.0f};
    uint32_t rng_state_{0x1337BEEFu};

    std::array<float, LATTICE_SIZE> c_profile_{};
    std::array<float, LATTICE_SIZE> grad_profile_{};
    std::array<float, LATTICE_SIZE> damping_profile_{};

    std::array<float, LATTICE_SIZE> u_{};
    std::array<float, LATTICE_SIZE> u_prev_{};
    std::array<float, LATTICE_SIZE> u_next_{};
};

} // namespace aether::engines
