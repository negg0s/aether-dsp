#pragma once

#include "../types.hpp"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <span>

namespace aether::engines {

/**
 * @brief Relativistic Acoustic Wormhole Engine (Morris-Thorne Acoustic Geometry).
 *
 * Simulates wave propagation across two distinct Riemannian manifolds (Universe A and Universe B)
 * connected by a narrow acoustic throat of radius b_0 with continuity of acoustic pressure and flux:
 *   ds^2 = -c^2 dt^2 + dr^2 + (b_0^2 + r^2) d\theta^2
 *
 * Physics Simulated:
 *   - Manifold A (Poincare Disk or Upper Flare): Wavefronts travel toward throat r -> 0
 *   - Throat Horizon: Resonant throat cavity exhibiting Quasinormal Mode (QNM) ringing
 *   - Manifold B (Lower Universe): Acoustic energy tunnels through throat into second universe
 *   - Bidirectional symplectic acoustic energy flux conservation:
 *       \Phi_{A \to B} = T(\omega) \Phi_A,  \Phi_{\text{refl}} = R(\omega) \Phi_A
 *   - Dual stereo outputs: Left channel = Universe A, Right channel = Universe B
 */
class AcousticWormhole {
public:
    static constexpr size_t UNIVERSE_GRID = 64;
    static constexpr size_t THROAT_NODES = 16;

    explicit AcousticWormhole(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(std::clamp(sample_rate, 1000u, 384000u)) {
        reset();
    }

    void reset() noexcept {
        u_a_.fill(0.0f);
        u_a_prev_.fill(0.0f);
        u_a_next_.fill(0.0f);

        u_b_.fill(0.0f);
        u_b_prev_.fill(0.0f);
        u_b_next_.fill(0.0f);

        throat_.fill(0.0f);
        throat_prev_.fill(0.0f);
        throat_next_.fill(0.0f);
    }

    /**
     * @brief Configure wormhole physical parameters.
     * @param throat_radius Throat opening radius b_0 (controls coupling bandwidth)
     * @param throat_stiffness Restoring stiffness of throat geometry
     * @param wave_speed Propagation speed in asymptotic universes (CFL bounded)
     * @param damping Asymptotic viscoelastic dissipation
     */
    void configure(float throat_radius = 0.35f, float throat_stiffness = 1.4f,
                   float wave_speed = 0.42f, float damping = 0.0006f) noexcept {
        b0_ = std::clamp(throat_radius, 0.05f, 0.90f);
        throat_stiffness_ = std::clamp(throat_stiffness, 0.2f, 4.0f);
        c_ = std::clamp(wave_speed, 0.05f, 0.48f);
        damping_ = std::clamp(damping, 0.00005f, 0.02f);
    }

    /**
     * @brief Excite Universe A with an acoustic pulse.
     * @param position Normalized radial coordinate in Universe A [0.1, 0.9]
     * @param velocity Strike velocity
     */
    void strike_universe_a(float position = 0.65f, float velocity = 0.9f) noexcept {
        const float pos = std::clamp(position, 0.1f, 0.9f) * static_cast<float>(UNIVERSE_GRID);
        for (size_t i = 0; i < UNIVERSE_GRID; ++i) {
            const float dx = static_cast<float>(i) - pos;
            const float pulse = velocity * std::exp(-dx * dx * 0.12f);
            u_a_[i] += pulse;
            u_a_prev_[i] += pulse * 0.5f;
        }
    }

    /**
     * @brief Excite Universe B directly.
     */
    void strike_universe_b(float position = 0.65f, float velocity = 0.9f) noexcept {
        const float pos = std::clamp(position, 0.1f, 0.9f) * static_cast<float>(UNIVERSE_GRID);
        for (size_t i = 0; i < UNIVERSE_GRID; ++i) {
            const float dx = static_cast<float>(i) - pos;
            const float pulse = velocity * std::exp(-dx * dx * 0.12f);
            u_b_[i] += pulse;
            u_b_prev_[i] += pulse * 0.5f;
        }
    }

    /**
     * @brief Compute next time step across both universes and the wormhole throat.
     * Returns StereoFrame {Universe A, Universe B}.
     */
    StereoFrame process() noexcept {
        const float rate_scale = static_cast<float>(DEFAULT_SAMPLE_RATE) / static_cast<float>(sample_rate_);
        const float effective_c = std::min(c_ * rate_scale, 0.48f);
        const float c2 = effective_c * effective_c;
        const float effective_damping = damping_ * rate_scale;
        const float damp_factor = 1.0f / (1.0f + effective_damping);

        // Outer asymptotic open/absorbing boundaries at infinity (index UNIVERSE_GRID - 1)
        u_a_next_[UNIVERSE_GRID - 1] = u_a_[UNIVERSE_GRID - 2] * 0.94f;
        u_b_next_[UNIVERSE_GRID - 1] = u_b_[UNIVERSE_GRID - 2] * 0.94f;

        for (size_t i = 1; i < UNIVERSE_GRID - 1; ++i) {
            const float r = static_cast<float>(i);
            const float r_factor = 1.0f + 0.5f * (b0_ * b0_) / (1.0f + r * r);
            const float laplacian = (u_a_[i + 1] - 2.0f * u_a_[i] + u_a_[i - 1]) * r_factor;
            const float val_a = ((2.0f - effective_damping) * u_a_[i] - u_a_prev_[i] + c2 * laplacian) * damp_factor;
            u_a_next_[i] = std::isfinite(val_a) ? std::clamp(val_a, -10.0f, 10.0f) : 0.0f;
        }

        for (size_t i = 1; i < UNIVERSE_GRID - 1; ++i) {
            const float r = static_cast<float>(i);
            const float r_factor = 1.0f + 0.5f * (b0_ * b0_) / (1.0f + r * r);
            const float laplacian = (u_b_[i + 1] - 2.0f * u_b_[i] + u_b_[i - 1]) * r_factor;
            const float val_b = ((2.0f - effective_damping) * u_b_[i] - u_b_prev_[i] + c2 * laplacian) * damp_factor;
            u_b_next_[i] = std::isfinite(val_b) ? std::clamp(val_b, -10.0f, 10.0f) : 0.0f;
        }

        // Throat boundaries couple to Universe A (u_a_[0]) and Universe B (u_b_[0])
        const float laplacian_t0 = throat_[1] - 2.0f * throat_[0] + u_a_[0] * b0_;
        throat_next_[0] = ((2.0f - damping_) * throat_[0] - throat_prev_[0] + c2 * laplacian_t0) * damp_factor;

        for (size_t k = 1; k < THROAT_NODES - 1; ++k) {
            const float laplacian = throat_[k + 1] - 2.0f * throat_[k] + throat_[k - 1];
            const float qnm_restore = -throat_stiffness_ * 0.05f * throat_[k];
            throat_next_[k] = (2.0f * throat_[k] - throat_prev_[k] + c2 * laplacian + qnm_restore) * 0.9995f;
        }

        const float laplacian_tend = u_b_[0] * b0_ - 2.0f * throat_[THROAT_NODES - 1] + throat_[THROAT_NODES - 2];
        throat_next_[THROAT_NODES - 1] = ((2.0f - damping_) * throat_[THROAT_NODES - 1] - throat_prev_[THROAT_NODES - 1] + c2 * laplacian_tend) * damp_factor;

        const float laplacian_a0 = u_a_[1] - 2.0f * u_a_[0] + throat_[0] * b0_;
        u_a_next_[0] = ((2.0f - damping_) * u_a_[0] - u_a_prev_[0] + c2 * laplacian_a0) * damp_factor;

        const float laplacian_b0 = u_b_[1] - 2.0f * u_b_[0] + throat_[THROAT_NODES - 1] * b0_;
        u_b_next_[0] = ((2.0f - damping_) * u_b_[0] - u_b_prev_[0] + c2 * laplacian_b0) * damp_factor;

        u_a_prev_ = u_a_;
        u_a_ = u_a_next_;

        u_b_prev_ = u_b_;
        u_b_ = u_b_next_;

        throat_prev_ = throat_;
        throat_ = throat_next_;

        constexpr size_t PICKUP = static_cast<size_t>(UNIVERSE_GRID * 0.45f);
        const float out_a = (u_a_[PICKUP] - u_a_prev_[PICKUP]) * 2.2f;
        const float out_b = (u_b_[PICKUP] - u_b_prev_[PICKUP]) * 2.2f;

        return {std::clamp(out_a, -1.0f, 1.0f), std::clamp(out_b, -1.0f, 1.0f)};
    }

    void process_block(std::span<StereoFrame> output) noexcept {
        for (auto& frame : output) {
            frame = process();
        }
    }

    [[nodiscard]] float total_energy() const noexcept {
        float e = 0.0f;
        for (size_t i = 0; i < UNIVERSE_GRID; ++i) {
            const float va = u_a_[i] - u_a_prev_[i];
            const float vb = u_b_[i] - u_b_prev_[i];
            e += va * va + vb * vb;
        }
        for (size_t k = 0; k < THROAT_NODES; ++k) {
            const float vt = throat_[k] - throat_prev_[k];
            e += vt * vt;
        }
        return e;
    }

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    float b0_{0.35f};
    float throat_stiffness_{1.4f};
    float c_{0.42f};
    float damping_{0.0006f};

    std::array<float, UNIVERSE_GRID> u_a_{};
    std::array<float, UNIVERSE_GRID> u_a_prev_{};
    std::array<float, UNIVERSE_GRID> u_a_next_{};

    std::array<float, UNIVERSE_GRID> u_b_{};
    std::array<float, UNIVERSE_GRID> u_b_prev_{};
    std::array<float, UNIVERSE_GRID> u_b_next_{};

    std::array<float, THROAT_NODES> throat_{};
    std::array<float, THROAT_NODES> throat_prev_{};
    std::array<float, THROAT_NODES> throat_next_{};
};

} // namespace aether::engines
