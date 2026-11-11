#pragma once

#include "../types.hpp"
#include <cmath>
#include <functional>

namespace aether::math {

/**
 * @brief 4th-Order Symplectic Yoshida Integrator for Separable Hamiltonian Systems.
 *
 * Exactly preserves the symplectic 2-form dp ^ dq and phase-space volume,
 * guaranteeing zero secular energy drift over millions of continuous audio samples.
 *
 * Hamiltonian: H(q, p) = T(p) + V(q) = (1/2) * p^2 + V(q)
 * Equations:   dq/dt = p,   dp/dt = -dV/dq = F(q)
 *
 * Reference: H. Yoshida, "Construction of higher order symplectic integrators",
 *            Phys. Lett. A 150 (1990) 262-268.
 */
class Yoshida4thIntegrator {
public:
    // Yoshida 4th-order weighting constants
    static constexpr double W1 = 1.35120719195965763405;          // 1 / (2 - 2^(1/3))
    static constexpr double W0 = -1.70241438391931526810;         // - 2^(1/3) / (2 - 2^(1/3))

    // Position drift coefficients c_i
    static constexpr double C1 = W1 / 2.0;
    static constexpr double C2 = (W0 + W1) / 2.0;
    static constexpr double C3 = (W0 + W1) / 2.0;
    static constexpr double C4 = W1 / 2.0;

    // Momentum kick coefficients d_i
    static constexpr double D1 = W1;
    static constexpr double D2 = W0;
    static constexpr double D3 = W1;

    struct State {
        double q{0.0}; // Position / Generalized coordinate
        double p{0.0}; // Momentum / Velocity
    };

    /**
     * @brief Perform one 4th-order symplectic time step.
     * @tparam ForceFunc Callable with signature `double(double q)` returning F(q) = -dV/dq
     */
    template <typename ForceFunc>
    static void step(State& state, double dt, ForceFunc&& compute_force) noexcept {
        if (!std::isfinite(state.q) || !std::isfinite(state.p)) {
            state.q = 0.0;
            state.p = 0.0;
            return;
        }

        // Stage 1
        state.q += C1 * dt * state.p;
        state.p += D1 * dt * compute_force(state.q);

        // Stage 2
        state.q += C2 * dt * state.p;
        state.p += D2 * dt * compute_force(state.q);

        // Stage 3
        state.q += C3 * dt * state.p;
        state.p += D3 * dt * compute_force(state.q);

        // Stage 4
        state.q += C4 * dt * state.p;

        if (!std::isfinite(state.q) || !std::isfinite(state.p)) {
            state.q = 0.0;
            state.p = 0.0;
        }
    }

    /**
     * @brief Compute total mechanical energy: E = (1/2)*p^2 + V(q).
     */
    template <typename PotentialFunc>
    [[nodiscard]] static double total_energy(const State& state, PotentialFunc&& compute_potential) noexcept {
        if (!std::isfinite(state.q) || !std::isfinite(state.p)) return 0.0;
        return 0.5 * state.p * state.p + compute_potential(state.q);
    }
};

} // namespace aether::math
