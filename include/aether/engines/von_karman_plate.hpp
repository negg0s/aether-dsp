#pragma once

#include "../types.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace aether::engines {

/**
 * @brief 2D Nonlinear Large-Deflection Thin Plate Resonator (Föppl-von Kármán Dynamics).
 *
 * Simulates large-deflection transverse plate vibrations where geometric stretching
 * of the midplane introduces cubic modal cross-coupling and amplitude-dependent
 * frequency shifts (pitch-glide and shimmering crescendo effect in orchestral gongs).
 *
 * Equations of motion in modal coordinates:
 *   ddot(q_k) + 2*gamma_k*dot(q_k) + omega_k^2 * q_k + beta_k * q_k^3 + sum_j Gamma_kj * q_k * q_j^2 = f_k(t)
 */
class VonKarmanPlate {
public:
    struct Mode {
        int m{1};
        int n{1};
        float frequency{110.0f};
        float damping{0.003f};
        float q{0.0f};       // Modal displacement
        float v{0.0f};       // Modal velocity
        float beta{200.0f};  // Self-nonlinearity coefficient (pitch glide)
        float coupling{50.0f}; // Cross-modal energy transfer coefficient
    };

    explicit VonKarmanPlate(uint32_t sample_rate = 48000)
        : sample_rate_(std::clamp(sample_rate, 1000u, 384000u)), dt_(1.0f / static_cast<float>(std::clamp(sample_rate, 1000u, 384000u))) {
        init_modes(110.0f, 1.2f, 0.6f);
    }

    /**
     * @brief Initialize modal lattice.
     * @param fundamental Base pitch in Hz (e.g. 80.0 Hz - 220.0 Hz)
     * @param stiffness Plate stiffness / thickness parameter
     * @param nonlinearity Strength of von Kármán nonlinear modal coupling [0.0 = linear, 1.0 = heavy gong]
     */
    void init_modes(float fundamental, float stiffness = 1.0f, float nonlinearity = 0.5f) {
        fundamental_ = std::clamp(fundamental, 20.0f, 10000.0f);
        stiffness_ = std::clamp(stiffness, 0.05f, 20.0f);
        nonlinearity_ = std::clamp(nonlinearity, 0.0f, 2.0f);
        modes_.clear();

        // Generate 24 analytical biharmonic modal partials
        constexpr int MAX_M = 5;
        constexpr int MAX_N = 5;

        for (int m = 1; m <= MAX_M; ++m) {
            for (int n = 1; n <= MAX_N; ++n) {
                if (m == 1 && n == 1) continue; // Skip static boundary mode

                // Inharmonic biharmonic plate frequency scaling: f ~ sqrt(m^4 + 2*m^2*n^2 + n^4)
                const float m2 = static_cast<float>(m * m);
                const float n2 = static_cast<float>(n * n);
                const float freq = fundamental * std::sqrt(m2 * m2 + 2.0f * m2 * n2 * stiffness_ + n2 * n2 * stiffness_ * stiffness_);

                if (freq < sample_rate_ * 0.45f) {
                    // High-frequency modes have higher material internal damping
                    const float damp = 0.0015f + 0.000008f * freq;
                    // Nonlinear coefficient scales with mode frequency
                    const float beta = 150.0f * nonlinearity_ * (1.0f + 0.001f * freq);
                    const float coupling = 80.0f * nonlinearity_;

                    modes_.push_back(Mode{
                        m, n, freq, damp, 0.0f, 0.0f, beta, coupling
                    });
                }
            }
        }
    }

    /**
     * @brief Strike plate at normalized coordinates (sx, sy) in [-1, 1].
     */
    void strike(Vec2 pos, float velocity, float strike_duration_sec = 0.002f) {
        pos.x = std::clamp(pos.x, -0.95f, 0.95f);
        pos.y = std::clamp(pos.y, -0.95f, 0.95f);

        for (auto& mode : modes_) {
            const float phi_x = std::sin(mode.m * std::numbers::pi_v<float> * (pos.x + 1.0f) * 0.5f);
            const float phi_y = std::sin(mode.n * std::numbers::pi_v<float> * (pos.y + 1.0f) * 0.5f);
            const float spatial_gain = phi_x * phi_y;

            mode.v += velocity * spatial_gain * 80.0f;
        }

        strike_impulse_ = velocity;
        strike_samples_left_ = static_cast<int>(strike_duration_sec * sample_rate_);
    }

    /**
     * @brief Advance physical plate dynamics by one sample frame using Symplectic Velocity-Verlet with 2x substepping.
     */
    StereoFrame step() noexcept {
        const size_t num_modes = modes_.size();
        const float dt_sub = dt_ * 0.5f;

        for (int sub = 0; sub < 2; ++sub) {
            float low_mode_energy = 0.0f;
            for (size_t i = 0; i < std::min<size_t>(6, num_modes); ++i) {
                low_mode_energy += modes_[i].q * modes_[i].q;
            }

            for (size_t i = 0; i < num_modes; ++i) {
                auto& m = modes_[i];
                const float omega = m.frequency * (2.0f * std::numbers::pi_v<float>);

                const float q2 = m.q * m.q;
                const float f_duffing = m.beta * q2 * m.q;

                float f_cascade = 0.0f;
                if (i > 6 && nonlinearity_ > 0.01f) {
                    f_cascade = m.coupling * low_mode_energy * m.q;
                }

                const float accel = -(omega * omega) * m.q - 2.0f * m.damping * omega * m.v - f_duffing - f_cascade;

                m.v += std::clamp(accel, -200000.0f, 200000.0f) * dt_sub;
                m.v = std::clamp(m.v, -500.0f, 500.0f);
                m.q += m.v * dt_sub;
                m.q = std::clamp(m.q, -10.0f, 10.0f);

                if (!std::isfinite(m.q) || !std::isfinite(m.v)) {
                    m.q = 0.0f;
                    m.v = 0.0f;
                }
            }
        }

        float out_l = 0.0f;
        float out_r = 0.0f;
        for (size_t i = 0; i < num_modes; ++i) {
            const auto& m = modes_[i];
            const float phi_l = std::sin(m.m * std::numbers::pi_v<float> * 0.3f) * std::sin(m.n * std::numbers::pi_v<float> * 0.65f);
            const float phi_r = std::sin(m.m * std::numbers::pi_v<float> * 0.7f) * std::sin(m.n * std::numbers::pi_v<float> * 0.35f);

            out_l += m.q * phi_l;
            out_r += m.q * phi_r;
        }

        const float sat_l = std::tanh(out_l * 0.25f);
        const float sat_r = std::tanh(out_r * 0.25f);

        return StereoFrame{sat_l, sat_r};
    }

    [[nodiscard]] const std::vector<Mode>& modes() const noexcept { return modes_; }
    [[nodiscard]] float fundamental() const noexcept { return fundamental_; }

    void reset() noexcept {
        for (auto& m : modes_) {
            m.q = 0.0f;
            m.v = 0.0f;
        }
        strike_samples_left_ = 0;
    }

private:
    uint32_t sample_rate_{48000};
    float dt_{1.0f / 48000.0f};
    float fundamental_{110.0f};
    float stiffness_{1.2f};
    float nonlinearity_{0.6f};
    float strike_impulse_{0.0f};
    int strike_samples_left_{0};
    std::vector<Mode> modes_;
};

} // namespace aether::engines
