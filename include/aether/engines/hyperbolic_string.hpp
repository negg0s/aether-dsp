#pragma once

#include "../types.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace aether::engines {

/**
 * @brief Nonlinear Stiff Kirchhoff String with Hyperbolic Jawari Buzzing Bridge.
 *
 * Implements a 1D FDTD solver for the nonlinear Kirchhoff wave equation with
 * dynamic tension modulation and bilateral unilateral contact with a curved
 * hyperbolic bridge obstacle (the iconic buzzing sitar/tanpura "Jawari" effect).
 *
 * Governing Equation:
 *   rho * d^2 y / dt^2 = T(t) * d^2 y / dx^2 - E*I * d^4 y / dx^4 - 2*b1 * dy/dt + F_contact
 * Dynamic Tension:
 *   T(t) = T_0 + (E*A / 2L) * int_0^L (dy/dx)^2 dx
 * Jawari Hyperbolic Obstacle:
 *   h(x) = -h_bridge * sech(x / a_jawari)
 *   F_contact(x, y) = K_contact * max(0, h(x) - y)^1.5
 */
class HyperbolicString {
public:
    static constexpr size_t NUM_POINTS = 64;

    struct Config {
        float length_m{0.85f};           // String physical length (meters)
        float fundamental_hz{146.83f};   // Base pitch (D3 = 146.83 Hz)
        float stiffness{0.00015f};       // Bending stiffness E*I / (rho*A)
        float tension_nonlinearity{2.5f};// Strength of pitch-glide tension modulation
        float damping_freq_indep{0.001f};// Air damping b1
        float damping_freq_dep{0.0005f}; // Visco-elastic internal damping b2
        float jawari_height{0.0008f};    // Bridge clearance height
        float jawari_curvature{0.18f};   // Characteristic curvature scale of hyperbolic jawari
        float jawari_stiffness{45000.0f};// Penalty spring constant of bridge contact
    };

    explicit HyperbolicString(uint32_t sample_rate = 48000)
        : HyperbolicString(sample_rate, Config{}) {}

    HyperbolicString(uint32_t sample_rate, Config cfg)
        : sample_rate_(std::clamp(sample_rate, 1000u, 384000u)),
          dt_(1.0f / static_cast<float>(std::clamp(sample_rate, 1000u, 384000u))),
          config_(cfg) {
        config_.length_m = std::clamp(config_.length_m, 0.05f, 10.0f);
        config_.jawari_curvature = std::max(0.001f, config_.jawari_curvature);
        config_.fundamental_hz = std::clamp(config_.fundamental_hz, 30.0f, 1200.0f);
        init_grid();
    }

    void init_grid() {
        dx_ = config_.length_m / (NUM_POINTS - 1);
        y_current_.assign(NUM_POINTS, 0.0f);
        y_previous_.assign(NUM_POINTS, 0.0f);
        y_next_.assign(NUM_POINTS, 0.0f);
        jawari_profile_.resize(NUM_POINTS);

        for (size_t i = 0; i < NUM_POINTS; ++i) {
            const float x = i * dx_;
            const float u = x / config_.jawari_curvature;
            const float sech_u = 1.0f / std::cosh(std::clamp(u, 0.0f, 10.0f));
            jawari_profile_[i] = -config_.jawari_height * (1.0f - sech_u);
        }

        update_parameters();
    }

    void set_pitch(float hz) noexcept {
        config_.fundamental_hz = std::clamp(hz, 30.0f, 1200.0f);
        update_parameters();
    }

    void update_parameters() noexcept {
        c0_ = 2.0f * config_.length_m * config_.fundamental_hz;
        const float raw_courant = (c0_ * dt_) / dx_;
        // CFL condition: sub-step so that Courant number per sub-step is <= 0.65
        substeps_ = static_cast<size_t>(std::ceil(raw_courant / 0.65f));
        if (substeps_ < 1) substeps_ = 1;
        if (substeps_ > 8) substeps_ = 8;
        courant_ = (c0_ * (dt_ / static_cast<float>(substeps_))) / dx_;
    }

    /**
     * @brief Strike or pluck the string at normalized position in (0, 1).
     * @param position Normalized string position [0.0 = nut, 1.0 = bridge]
     * @param velocity Pluck force / initial displacement
     * @param width Spatial width of pluck triangle
     */
    void pluck(float position = 0.22f, float velocity = 0.85f, float width = 0.08f) noexcept {
        const float pos_m = std::clamp(position, 0.05f, 0.95f) * config_.length_m;
        const float w_m = std::clamp(width, 0.02f, 0.3f) * config_.length_m;
        const float amp = velocity * 0.005f; // Physical displacement in meters

        for (size_t i = 1; i < NUM_POINTS - 1; ++i) {
            const float x = i * dx_;
            const float dist = std::abs(x - pos_m);
            if (dist < w_m) {
                // Raised cosine pluck profile
                const float shape = 0.5f * (1.0f + std::cos(std::numbers::pi_v<float> * dist / w_m));
                y_current_[i] = amp * shape;
                y_previous_[i] = y_current_[i];
            }
        }
    }

    /**
     * @brief Step the nonlinear Kirchhoff-Jawari string by one sample.
     */
    StereoFrame step() noexcept {
        const float dt_sub = dt_ / static_cast<float>(substeps_);
        const float b1 = config_.damping_freq_indep;
        const float b2 = config_.damping_freq_dep;
        const float inv_denom = 1.0f / (1.0f + b1 * dt_sub);

        for (size_t sub = 0; sub < substeps_; ++sub) {
            // 1. Calculate string tension modulation integral: int (dy/dx)^2 dx
            float gradient_energy = 0.0f;
            for (size_t i = 0; i < NUM_POINTS - 1; ++i) {
                const float slope = (y_current_[i + 1] - y_current_[i]) / dx_;
                gradient_energy += slope * slope * dx_;
            }
            gradient_energy = std::clamp(gradient_energy, 0.0f, 10.0f);

            // Nonlinear wave velocity: c^2 = c0^2 * (1 + kappa * gradient_energy)
            const float c2 = c0_ * c0_ * (1.0f + config_.tension_nonlinearity * gradient_energy);
            const float r2 = (c2 * dt_sub * dt_sub) / (dx_ * dx_);

            // Bending stiffness coefficient: mu = E*I / (rho*A) * dt^2 / dx^4
            const float mu = (config_.stiffness * dt_sub * dt_sub) / (dx_ * dx_ * dx_ * dx_);

            // 2. Interior FDTD update for stiff string
            for (size_t i = 2; i < NUM_POINTS - 2; ++i) {
                const float d2y = y_current_[i + 1] - 2.0f * y_current_[i] + y_current_[i - 1];
                const float d4y = y_current_[i + 2] - 4.0f * y_current_[i + 1] + 6.0f * y_current_[i]
                                  - 4.0f * y_current_[i - 1] + y_current_[i - 2];

                const float damp_term = (2.0f * b2 * dt_sub / (dx_ * dx_)) *
                    ((y_current_[i + 1] - 2.0f * y_current_[i] + y_current_[i - 1]) -
                     (y_previous_[i + 1] - 2.0f * y_previous_[i] + y_previous_[i - 1]));

                float y_new = (2.0f * y_current_[i] - y_previous_[i] * (1.0f - b1 * dt_sub)
                               + r2 * d2y - mu * d4y + damp_term) * inv_denom;

                const float penetration = jawari_profile_[i] - y_new;
                if (penetration > 0.0f) {
                    const float contact_force = config_.jawari_stiffness * penetration * std::sqrt(penetration);
                    y_new += (contact_force * dt_sub * dt_sub);
                }

                y_next_[i] = std::isfinite(y_new) ? std::clamp(y_new, -0.05f, 0.05f) : 0.0f;
            }

            y_next_[0] = 0.0f;
            y_next_[NUM_POINTS - 1] = 0.0f;
            y_next_[1] = y_next_[2] * 0.5f;
            y_next_[NUM_POINTS - 2] = y_next_[NUM_POINTS - 3] * 0.5f;

            for (size_t i = 0; i < NUM_POINTS; ++i) {
                if (!std::isfinite(y_next_[i])) {
                    y_next_[i] = 0.0f;
                }
            }

            std::copy(y_current_.begin(), y_current_.end(), y_previous_.begin());
            std::copy(y_next_.begin(), y_next_.end(), y_current_.begin());
        }

        constexpr size_t PICKUP_L = NUM_POINTS / 6;
        constexpr size_t PICKUP_R = NUM_POINTS / 2 + 4;

        const float out_l = y_current_[PICKUP_L] * 80.0f;
        const float out_r = y_current_[PICKUP_R] * 80.0f;

        return StereoFrame{
            std::tanh(out_l),
            std::tanh(out_r)
        };
    }

    void reset() noexcept {
        std::fill(y_current_.begin(), y_current_.end(), 0.0f);
        std::fill(y_previous_.begin(), y_previous_.end(), 0.0f);
        std::fill(y_next_.begin(), y_next_.end(), 0.0f);
    }

    [[nodiscard]] const std::vector<float>& state() const noexcept { return y_current_; }
    [[nodiscard]] const std::vector<float>& jawari_profile() const noexcept { return jawari_profile_; }

private:
    uint32_t sample_rate_{48000};
    float dt_{1.0f / 48000.0f};
    float dx_{0.01f};
    float c0_{300.0f};
    float courant_{0.5f};
    size_t substeps_{1};
    Config config_;

    std::vector<float> y_current_;
    std::vector<float> y_previous_;
    std::vector<float> y_next_;
    std::vector<float> jawari_profile_;
};

} // namespace aether::engines
