#pragma once

#include "../types.hpp"
#include "../math/runge_kutta.hpp"
#include <cmath>
#include <algorithm>

namespace aether::engines {

enum class AttractorType {
    Clifford,
    Lorenz,
    Rossler
};

/**
 * @brief Continuous Non-Linear Strange Attractor Audio Oscillator & Modulator.
 *
 * Integrates non-linear differential equations using 4th-order Runge-Kutta.
 * The chaotic state trajectories generate evolving deterministic organic soundscapes
 * and continuous modulation signals without any stochastic/random noise or AI.
 */
class ChaoticOscillator {
public:
    ChaoticOscillator() noexcept {
        reset();
    }

    constexpr void reset() noexcept {
        state_ = {0.1f, 0.0f, 0.0f};
        prev_in_l_ = 0.0f;
        prev_out_l_ = 0.0f;
        prev_in_r_ = 0.0f;
        prev_out_r_ = 0.0f;
    }

    void configure(AttractorType type, float speed = 1.0f, float parameter_a = -1.4f, float parameter_b = 1.6f) noexcept {
        type_ = type;
        speed_ = std::clamp(speed, 0.01f, 20.0f);
        param_a_ = parameter_a;
        param_b_ = parameter_b;
    }

    [[nodiscard]] StereoFrame step(float sample_rate = static_cast<float>(DEFAULT_SAMPLE_RATE)) noexcept {
        sample_rate = std::clamp(sample_rate, 1000.0f, 384000.0f);
        const float dt = (speed_ * 440.0f) / sample_rate;

        switch (type_) {
            case AttractorType::Lorenz: {
                // Lorenz attractor parameters: sigma=10, rho=28, beta=8/3
                constexpr float sigma = 10.0f;
                constexpr float rho = 28.0f;
                constexpr float beta = 8.0f / 3.0f;

                state_ = math::RungeKutta4::step(state_, dt * 0.05f, [](Vec3 s) -> Vec3 {
                    return {
                        sigma * (s.y - s.x),
                        s.x * (rho - s.z) - s.y,
                        s.x * s.y - beta * s.z
                    };
                });
                break;
            }

            case AttractorType::Rossler: {
                // Rossler attractor: a=0.2, b=0.2, c=5.7
                constexpr float a = 0.2f;
                constexpr float b = 0.2f;
                constexpr float c = 5.7f;

                state_ = math::RungeKutta4::step(state_, dt * 0.05f, [](Vec3 s) -> Vec3 {
                    return {
                        -s.y - s.z,
                        s.x + a * s.y,
                        b + s.z * (s.x - c)
                    };
                });
                break;
            }

            case AttractorType::Clifford: {
                // Discrete Clifford mapping interpolated
                const float nx = std::sin(param_a_ * state_.y) + param_c_ * std::cos(param_a_ * state_.x);
                const float ny = std::sin(param_b_ * state_.x) + param_d_ * std::cos(param_b_ * state_.y);
                const float nz = (nx + ny) * 0.5f;

                const float alpha = std::clamp(dt * 2.0f, 0.001f, 0.8f);
                state_.x += alpha * (nx - state_.x);
                state_.y += alpha * (ny - state_.y);
                state_.z += alpha * (nz - state_.z);
                break;
            }
        }

        // Normalize state-space coordinates into audio range [-1.0, 1.0]
        Sample left = 0.0f;
        Sample right = 0.0f;

        if (type_ == AttractorType::Lorenz) {
            left = std::clamp(state_.x / 20.0f, -1.0f, 1.0f);
            right = std::clamp(state_.y / 25.0f, -1.0f, 1.0f);
        } else if (type_ == AttractorType::Rossler) {
            left = std::clamp(state_.x / 12.0f, -1.0f, 1.0f);
            right = std::clamp(state_.y / 12.0f, -1.0f, 1.0f);
        } else {
            left = std::clamp(state_.x / 2.0f, -1.0f, 1.0f);
            right = std::clamp(state_.y / 2.0f, -1.0f, 1.0f);
        }

        // High-pass DC offset blocker for both channels (y[n] = x[n] - x[n-1] + R * y[n-1])
        const float raw_l = left;
        const float raw_r = right;
        left = raw_l - prev_in_l_ + 0.995f * prev_out_l_;
        right = raw_r - prev_in_r_ + 0.995f * prev_out_r_;

        // Self-heal numerical breakdown in state and filter registers
        if (!std::isfinite(left) || !std::isfinite(right)) {
            reset();
            left = 0.0f;
            right = 0.0f;
        }

        prev_in_l_ = raw_l;
        prev_out_l_ = left;
        prev_in_r_ = raw_r;
        prev_out_r_ = right;

        return {std::clamp(left, -1.0f, 1.0f), std::clamp(right, -1.0f, 1.0f)};
    }

    [[nodiscard]] Vec3 state() const noexcept {
        return state_;
    }

private:
    AttractorType type_{AttractorType::Clifford};
    Vec3 state_{0.1f, 0.0f, 0.0f};
    float speed_{1.0f};
    float param_a_{-1.4f};
    float param_b_{1.6f};
    float param_c_{1.0f};
    float param_d_{0.7f};
    float prev_in_l_{0.0f};
    float prev_out_l_{0.0f};
    float prev_in_r_{0.0f};
    float prev_out_r_{0.0f};
};

} // namespace aether::engines
