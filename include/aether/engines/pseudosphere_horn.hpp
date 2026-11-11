#pragma once

#include "../types.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace aether::engines {

/**
 * @brief Beltrami Pseudosphere Tractrix Horn Acoustic Physical Modeling Synthesizer.
 *
 * Models acoustic wave propagation inside an axisymmetric horn surface of constant
 * negative Gaussian curvature K = -1 (Beltrami Pseudosphere / Tractrix manifold).
 *
 * Solves the Webster Horn Wave Equation:
 *   d^2 p / dz^2 + (S'(z)/S(z)) * (dp/dz) = (1/c^2) * (d^2 p / dt^2)
 * where the cross-sectional area profile S(z) = pi * r(z)^2 follows the tractrix flare:
 *   r(z) = r_bell * sech(z / a)
 * Cutoff frequency: f_c = c / (2 * pi * a)
 */
class PseudosphereHorn {
public:
    static constexpr size_t NUM_CELLS = 48;

    struct Config {
        float horn_length_m{0.65f};     // Horn physical length (meters)
        float throat_radius_m{0.008f};  // Throat radius (entrance)
        float bell_radius_m{0.12f};     // Bell radius (mouth)
        float curvature_a{0.22f};       // Characteristic pseudosphere curvature radius
        float damping{0.0006f};         // Visco-thermal tube wall loss
        float speed_of_sound{343.0f};   // m/s
    };

    explicit PseudosphereHorn(uint32_t sample_rate = 48000)
        : PseudosphereHorn(sample_rate, Config{}) {}

    PseudosphereHorn(uint32_t sample_rate, Config cfg)
        : sample_rate_(std::clamp(sample_rate, 1000u, 384000u)),
          dt_(1.0f / static_cast<float>(std::clamp(sample_rate, 1000u, 384000u))),
          config_(cfg) {
        init_geometry();
    }

    void init_geometry() {
        config_.horn_length_m = std::clamp(config_.horn_length_m, 0.05f, 5.0f);
        config_.curvature_a = std::clamp(config_.curvature_a, 0.01f, 2.0f);
        config_.throat_radius_m = std::clamp(config_.throat_radius_m, 0.001f, 0.5f);
        config_.bell_radius_m = std::clamp(config_.bell_radius_m, config_.throat_radius_m + 0.001f, 2.0f);
        config_.speed_of_sound = std::clamp(config_.speed_of_sound, 50.0f, 1500.0f);
        config_.damping = std::clamp(config_.damping, 0.0f, 0.5f);

        dz_ = config_.horn_length_m / static_cast<float>(NUM_CELLS - 1);
        c_ = config_.speed_of_sound;
        const float raw_courant = (c_ * dt_) / dz_;
        substeps_ = static_cast<size_t>(std::ceil(raw_courant / 0.70f));
        if (substeps_ < 1) substeps_ = 1;
        courant_ = (c_ * (dt_ / static_cast<float>(substeps_))) / dz_;

        area_.resize(NUM_CELLS);
        p_current_.assign(NUM_CELLS, 0.0f);
        p_previous_.assign(NUM_CELLS, 0.0f);
        p_next_.assign(NUM_CELLS, 0.0f);

        for (size_t i = 0; i < NUM_CELLS; ++i) {
            const float z = i * dz_;
            const float u = (config_.horn_length_m - z) / config_.curvature_a;
            const float sech_u = 1.0f / std::cosh(std::clamp(u, 0.0f, 6.0f));
            const float r = config_.throat_radius_m + (config_.bell_radius_m - config_.throat_radius_m) * sech_u;
            area_[i] = std::max(1e-7f, std::numbers::pi_v<float> * r * r);
        }

        bell_radiation_state_ = 0.0f;
    }

    /**
     * @brief Excite the throat of the pseudosphere horn with acoustic pressure.
     */
    void excite_throat(float pressure_pulse) {
        p_current_[0] += pressure_pulse;
        p_previous_[0] += pressure_pulse * 0.5f;
    }

    /**
     * @brief Continuous non-linear lip-reed excitation at the horn throat.
     */
    void blow(float lip_pressure, float lip_frequency, float dt) {
        lip_phase_ += lip_frequency * dt * 2.0f * std::numbers::pi_v<float>;
        if (lip_phase_ >= 2.0f * std::numbers::pi_v<float>) {
            lip_phase_ -= 2.0f * std::numbers::pi_v<float>;
        }

        // Non-linear Bernoulli lip-aperture opening model
        const float back_pressure = p_current_[0];
        const float delta_p = lip_pressure - back_pressure;
        const float opening = std::max(0.0f, std::sin(lip_phase_) + 0.3f * delta_p);
        const float flow = 0.6f * opening * std::sqrt(std::abs(delta_p)) * (delta_p >= 0.0f ? 1.0f : -1.0f);

        p_current_[0] += flow * 12.0f;
    }

    /**
     * @brief Step the acoustic finite-difference Webster wave solver by one sample.
     */
    StereoFrame step() noexcept {
        const float r2 = courant_ * courant_;
        const float gamma = std::clamp(1.0f - (config_.damping / static_cast<float>(substeps_)), 0.0f, 1.0f);

        for (size_t sub = 0; sub < substeps_; ++sub) {
            // Interior Webster horn finite-difference update:
            for (size_t i = 1; i < NUM_CELLS - 1; ++i) {
                const float s_mid_plus  = 0.5f * (area_[i] + area_[i + 1]);
                const float s_mid_minus = 0.5f * (area_[i] + area_[i - 1]);

                const float spatial_deriv = (s_mid_plus * (p_current_[i + 1] - p_current_[i]) -
                                             s_mid_minus * (p_current_[i] - p_current_[i - 1])) / area_[i];

                p_next_[i] = (2.0f * p_current_[i] - p_previous_[i] * gamma + r2 * spatial_deriv) * gamma;
            }

            p_next_[0] = p_current_[1] * 0.98f;

            const float p_bell = p_current_[NUM_CELLS - 2];
            const float radiation_cutoff = 0.25f / static_cast<float>(substeps_);
            bell_radiation_state_ = bell_radiation_state_ * (1.0f - radiation_cutoff) + p_bell * radiation_cutoff;
            p_next_[NUM_CELLS - 1] = -(p_bell - bell_radiation_state_) * 0.85f;

            std::copy(p_current_.begin(), p_current_.end(), p_previous_.begin());
            std::copy(p_next_.begin(), p_next_.end(), p_current_.begin());
        }

        const float radiated_pressure = (p_current_[NUM_CELLS - 1] - p_previous_[NUM_CELLS - 1]) * 4.0f;
        const float out_l = radiated_pressure * 0.85f + p_current_[0] * 0.15f;
        const float out_r = radiated_pressure * 0.70f + p_current_[NUM_CELLS / 2] * 0.30f;

        return StereoFrame{
            std::tanh(out_l * 0.4f),
            std::tanh(out_r * 0.4f)
        };
    }

    void reset() noexcept {
        std::fill(p_current_.begin(), p_current_.end(), 0.0f);
        std::fill(p_previous_.begin(), p_previous_.end(), 0.0f);
        std::fill(p_next_.begin(), p_next_.end(), 0.0f);
        bell_radiation_state_ = 0.0f;
        lip_phase_ = 0.0f;
    }

    [[nodiscard]] float bell_cutoff_frequency() const noexcept {
        return config_.speed_of_sound / (2.0f * std::numbers::pi_v<float> * config_.curvature_a);
    }

private:
    uint32_t sample_rate_{48000};
    float dt_{1.0f / 48000.0f};
    Config config_;
    float dz_{0.01f};
    float c_{343.0f};
    float courant_{0.5f};
    size_t substeps_{1};
    float bell_radiation_state_{0.0f};
    float lip_phase_{0.0f};

    std::vector<float> area_;
    std::vector<float> p_current_;
    std::vector<float> p_previous_;
    std::vector<float> p_next_;
};

} // namespace aether::engines
