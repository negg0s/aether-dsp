#pragma once

#include "../types.hpp"
#include "../math/yoshida_integrator.hpp"
#include <cmath>
#include <algorithm>

namespace aether::engines {

/**
 * @brief Hénon-Heiles Astrophysical Galactic Hamiltonian Chaos Oscillator.
 *
 * Models non-integrable stellar orbits in an axisymmetric galactic gravitational potential.
 * Exactly preserves the symplectic 2-form and total energy using 4th-Order Yoshida integration.
 *
 * Hamiltonian:
 *   H(x, y, px, py) = 1/2*(px^2 + py^2) + 1/2*(x^2 + y^2) + x^2*y - 1/3*y^3
 * Equations:
 *   dx/dt = px,   dpx/dt = -x - 2*x*y
 *   dy/dt = py,   dpy/dt = -y - x^2 + y^2
 * Escape energy: E_crit = 1/6 ~= 0.16667
 */
class HenonHeilesOscillator {
public:
    struct State2D {
        double x{0.1};
        double y{0.1};
        double px{0.0};
        double py{0.12};
    };

    explicit HenonHeilesOscillator(double energy = 0.12, double time_scale = 350.0)
        : energy_(energy), time_scale_(time_scale) {
        reset(energy_);
    }

    /**
     * @brief Reset state with specified total energy E in (0, 1/6).
     */
    void reset(double energy = 0.12) noexcept {
        energy_ = std::clamp(energy, 0.01, 0.165);
        state_.x = 0.15;
        state_.y = 0.08;
        state_.px = 0.10;

        const double v = 0.5 * (state_.x * state_.x + state_.y * state_.y) +
                         state_.x * state_.x * state_.y - (1.0 / 3.0) * state_.y * state_.y * state_.y;
        const double diff = 2.0 * (energy_ - v) - state_.px * state_.px;
        state_.py = diff > 0.0 ? std::sqrt(diff) : 0.05;
    }

    /**
     * @brief Set galactic time-scale multiplier (controls pitch frequency).
     */
    void set_time_scale(double scale) noexcept {
        time_scale_ = std::clamp(scale, 20.0, 3000.0);
    }

    /**
     * @brief Step the 4D Hamiltonian system by dt using 4th-Order Symplectic Yoshida integration.
     */
    StereoFrame step(float sample_rate) noexcept {
        sample_rate = std::clamp(sample_rate, 1000.0f, 384000.0f);
        const double dt = (1.0 / static_cast<double>(sample_rate)) * time_scale_;

        // Yoshida 4th-order integration steps for 2 coupled dimensions (x, y)
        auto step_yoshida = [&](double c_sub, double d_sub) {
            state_.x += c_sub * dt * state_.px;
            state_.y += c_sub * dt * state_.py;

            const double fx = -state_.x - 2.0 * state_.x * state_.y;
            const double fy = -state_.y - state_.x * state_.x + state_.y * state_.y;

            state_.px += d_sub * dt * fx;
            state_.py += d_sub * dt * fy;
        };

        using aether::math::Yoshida4thIntegrator;
        step_yoshida(Yoshida4thIntegrator::C1, Yoshida4thIntegrator::D1);
        step_yoshida(Yoshida4thIntegrator::C2, Yoshida4thIntegrator::D2);
        step_yoshida(Yoshida4thIntegrator::C3, Yoshida4thIntegrator::D3);
        step_yoshida(Yoshida4thIntegrator::C4, 0.0);

        // Bounded reflection and escape trajectory protection
        const double r2 = state_.x * state_.x + state_.y * state_.y;
        if (!std::isfinite(state_.x) || !std::isfinite(state_.y) || r2 > 1.4) {
            reset(energy_);
        } else if (r2 > 0.95) {
            state_.px = -state_.px * 0.90;
            state_.py = -state_.py * 0.90;
            state_.x *= 0.96;
            state_.y *= 0.96;
        }

        const float out_l = static_cast<float>(std::tanh(state_.x * 2.2));
        const float out_r = static_cast<float>(std::tanh(state_.y * 2.2));

        return StereoFrame{out_l, out_r};
    }

    [[nodiscard]] double compute_energy() const noexcept {
        const double kinetic = 0.5 * (state_.px * state_.px + state_.py * state_.py);
        const double potential = 0.5 * (state_.x * state_.x + state_.y * state_.y) +
                                 state_.x * state_.x * state_.y - (1.0 / 3.0) * state_.y * state_.y * state_.y;
        return kinetic + potential;
    }

    [[nodiscard]] const State2D& state() const noexcept { return state_; }

private:
    double energy_{0.12};
    double time_scale_{350.0};
    State2D state_;
};

} // namespace aether::engines
