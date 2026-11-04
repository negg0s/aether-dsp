#pragma once

#include "../types.hpp"
#include <cmath>
#include <algorithm>

namespace aether::engines {

/**
 * @brief Non-Linear Stick-Slip Friction Exciter (Bowed Physical Modeling).
 *
 * Implements physical modeling of a violin bow or singing bowl mallet contacting
 * an elastic plate or resonator surface.
 * Friction force is modeled via the classical Stribeck non-linear curve:
 *    F_friction(v_rel) = F_N * [ mu_d + (mu_s - mu_d) * exp(-alpha * |v_rel|) ] * sgn(v_rel)
 *
 * Induces sustained autonomous limit-cycle oscillations, subharmonic bifurcations,
 * and crystalline multiphonic overtones without sample playback.
 */
class StickSlipExciter {
public:
    constexpr StickSlipExciter() noexcept = default;

    /**
     * @brief Configure bow physical friction parameters.
     * @param static_friction mu_s (typically 0.6 - 0.9)
     * @param dynamic_friction mu_d (typically 0.2 - 0.4)
     * @param stribeck_decay alpha velocity decay factor
     */
    void configure(float static_friction = 0.75f, float dynamic_friction = 0.35f, float stribeck_decay = 4.0f) noexcept {
        mu_s_ = static_friction;
        mu_d_ = dynamic_friction;
        alpha_ = stribeck_decay;
    }

    /**
     * @brief Compute non-linear friction force exerted on resonator surface.
     * @param bow_velocity Velocity of the bow / mallet (m/s)
     * @param bow_force Normal downward pressing force F_N (Newtons, 0.1 to 5.0)
     * @param surface_velocity Instantaneous velocity of the plate/waveguide at contact point
     * @return Applied instantaneous excitation force
     */
    [[nodiscard]] inline float compute_force(float bow_velocity, float bow_force, float surface_velocity) noexcept {
        const float v_rel = bow_velocity - surface_velocity;
        const float abs_v = std::abs(v_rel);

        // Stribeck non-linear friction coefficient
        const float mu = mu_d_ + (mu_s_ - mu_d_) * std::exp(-alpha_ * abs_v);

        // Hyperbolic tangent smoothing to prevent numerical discontinuity at v_rel = 0
        const float smoothed_sgn = std::tanh(v_rel * 25.0f);

        return bow_force * mu * smoothed_sgn;
    }

private:
    float mu_s_{0.75f};
    float mu_d_{0.35f};
    float alpha_{4.0f};
};

} // namespace aether::engines
