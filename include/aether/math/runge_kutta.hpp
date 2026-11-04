#pragma once

#include "../types.hpp"
#include <concepts>

namespace aether::math {

/**
 * @brief High-precision 4th-Order Runge-Kutta (RK4) ODE numerical integrator.
 */
class RungeKutta4 {
public:
    template <typename SystemFunc>
    requires requires(SystemFunc f, Vec3 state, float dt) {
        { f(state) } -> std::same_as<Vec3>;
    }
    [[nodiscard]] static inline Vec3 step(Vec3 current, float dt, SystemFunc&& system) noexcept {
        const Vec3 k1 = system(current);
        const Vec3 k2 = system(current + k1 * (0.5f * dt));
        const Vec3 k3 = system(current + k2 * (0.5f * dt));
        const Vec3 k4 = system(current + k3 * dt);

        return current + (k1 + k2 * 2.0f + k3 * 2.0f + k4) * (dt / 6.0f);
    }
};

} // namespace aether::math
