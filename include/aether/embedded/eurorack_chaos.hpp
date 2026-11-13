#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Chaos Attractor (Rossler / Lorenz fixed-point)
// Generates bounded aperiodic chaotic control voltages (X, Y, Z) for modular modulation.
// Fixed-point Euler-symplectic integrator in Q15 with zero dynamic memory allocation.
class EurorackChaosAttractor {
public:
    EurorackChaosAttractor() {
        reset();
    }

    void reset() noexcept {
        x_ = 3200;  // 0.1 in Q15
        y_ = 0;
        z_ = 0;
        dt_q15_ = math::FixedQ15::from_float(0.04f); // integration step size
    }

    void set_speed(math::FixedQ15 dt) noexcept {
        dt_q15_ = dt;
    }

    struct ChaosOutput {
        math::FixedQ15 x{0};
        math::FixedQ15 y{0};
        math::FixedQ15 z{0};
    };

    // Step the Rossler chaotic differential equations:
    // dx/dt = -y - z
    // dy/dt = x + a * y (where a ~ 0.2)
    // dz/dt = b + z * (x - c) (where b ~ 0.2, c ~ 5.7)
    // Scaled to fixed-point [-1.0, 1.0] range
    ChaosOutput step() noexcept {
        // dx = (-y - z/2) * dt
        int64_t dx = (-(x_ + y_) * dt_q15_.raw) >> 15;
        // dy = (x + y/4) * dt
        int64_t dy = ((x_ + (y_ >> 2)) * dt_q15_.raw) >> 15;
        // dz = nonlinear coupling
        int64_t coupling = (static_cast<int64_t>(x_) * z_) >> 15;
        int64_t dz = ((3000 + coupling - (z_ >> 1)) * dt_q15_.raw) >> 15;

        x_ += static_cast<int32_t>(dx);
        y_ += static_cast<int32_t>(dy);
        z_ += static_cast<int32_t>(dz);

        // Boundedness clamping for embedded modular stability
        if (x_ > 30000) x_ = 30000;
        if (x_ < -30000) x_ = -30000;
        if (y_ > 30000) y_ = 30000;
        if (y_ < -30000) y_ = -30000;
        if (z_ > 30000) z_ = 30000;
        if (z_ < -30000) z_ = -30000;

        return {
            math::FixedQ15(static_cast<int16_t>(x_)),
            math::FixedQ15(static_cast<int16_t>(y_)),
            math::FixedQ15(static_cast<int16_t>(z_))
        };
    }

private:
    int32_t x_{3200};
    int32_t y_{0};
    int32_t z_{0};
    math::FixedQ15 dt_q15_{math::FixedQ15::from_float(0.04f)};
};

} // namespace aether::embedded
