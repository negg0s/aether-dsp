#pragma once

#include "../types.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace aether::engines {

struct PlateMode {
    uint8_t m{1};
    uint8_t n{1};
    float frequency{220.0f};
    float amplitude{0.0f};
    float phase{0.0f};
    float decay_rate{0.001f};
    float spatial_coupling{1.0f};
};

/**
 * @brief 2D Kirchhoff-Love Elastic Thin Plate Modal Synthesizer (Chladni Plate).
 *
 * Implements physical modeling of a vibrating 2D elastic square plate with free boundary conditions.
 * The biharmonic equation:
 *     D * nabla^4(w) + rho * h * (d^2 w / dt^2) = 0
 * yields inharmonic mode frequencies:
 *     f_{m,n} = (pi / 2) * sqrt(D / (rho * h)) * ((m / Lx)^2 + (n / Ly)^2)
 *
 * Nodal lines where w_{m,n}(x,y) = 0 produce the iconic geometric Chladni sand patterns.
 */
class ChladniPlate {
public:
    static constexpr size_t MAX_MODES = 32;

    ChladniPlate() noexcept {
        init_modes(440.0f, 1.0f);
    }

    /**
     * @brief Configure plate physical stiffness, material, and fundamental pitch.
     * @param fundamental_freq Base frequency f_11 in Hz
     * @param stiffness Plate stiffness parameter D (alters inharmonic dispersion)
     * @param sample_rate Audio sample rate
     */
    void configure(float fundamental_freq, float stiffness = 1.0f, float sample_rate = static_cast<float>(DEFAULT_SAMPLE_RATE)) noexcept {
        sample_rate_ = sample_rate;
        init_modes(fundamental_freq, stiffness);
    }

    /**
     * @brief Excite plate at normalized coordinates (x, y) in [-1, 1].
     */
    void strike(Vec2 strike_coord, float velocity = 0.9f) noexcept {
        const float x = std::clamp(strike_coord.x, -0.99f, 0.99f);
        const float y = std::clamp(strike_coord.y, -0.99f, 0.99f);

        for (auto& mode : modes_) {
            // Chladni modal shape: cos(m*pi*x)*cos(n*pi*y) - cos(n*pi*x)*cos(m*pi*y)
            const float mx = mode.m * static_cast<float>(constants::PI) * (x * 0.5f + 0.5f);
            const float my = mode.n * static_cast<float>(constants::PI) * (y * 0.5f + 0.5f);
            const float nx = mode.n * static_cast<float>(constants::PI) * (x * 0.5f + 0.5f);
            const float ny = mode.m * static_cast<float>(constants::PI) * (y * 0.5f + 0.5f);

            const float shape = std::cos(mx) * std::cos(my) - std::cos(nx) * std::cos(ny);
            mode.spatial_coupling = shape;
            mode.amplitude = velocity * std::abs(shape) / (1.0f + 0.2f * (mode.m + mode.n));
        }
    }

    /**
     * @brief Evaluate modal field displacement at point (x, y) at current time.
     */
    [[nodiscard]] float evaluate_field(Vec2 coord) const noexcept {
        const float x = std::clamp(coord.x, -0.99f, 0.99f);
        const float y = std::clamp(coord.y, -0.99f, 0.99f);
        float total = 0.0f;

        for (const auto& mode : modes_) {
            if (mode.amplitude < 1e-4f) continue;

            const float mx = mode.m * static_cast<float>(constants::PI) * (x * 0.5f + 0.5f);
            const float my = mode.n * static_cast<float>(constants::PI) * (y * 0.5f + 0.5f);
            const float nx = mode.n * static_cast<float>(constants::PI) * (x * 0.5f + 0.5f);
            const float ny = mode.m * static_cast<float>(constants::PI) * (y * 0.5f + 0.5f);

            const float shape = std::cos(mx) * std::cos(my) - std::cos(nx) * std::cos(ny);
            total += shape * mode.amplitude * std::cos(mode.phase);
        }
        return total;
    }

    /**
     * @brief Process one audio frame.
     */
    [[nodiscard]] StereoFrame step(Vec2 pickup_l = {-0.6f, -0.6f}, Vec2 pickup_r = {0.6f, 0.6f}) noexcept {
        Sample left = 0.0f;
        Sample right = 0.0f;

        const float inv_sr = 1.0f / sample_rate_;
        const float two_pi = static_cast<float>(constants::TWO_PI);

        for (auto& mode : modes_) {
            if (mode.amplitude < 1e-6f) continue;

            const float osc = std::sin(mode.phase);
            const float current_sample = osc * mode.amplitude;

            // Compute spatial pickup coupling
            const float ml_x = mode.m * static_cast<float>(constants::PI) * (pickup_l.x * 0.5f + 0.5f);
            const float ml_y = mode.n * static_cast<float>(constants::PI) * (pickup_l.y * 0.5f + 0.5f);
            const float nl_x = mode.n * static_cast<float>(constants::PI) * (pickup_l.x * 0.5f + 0.5f);
            const float nl_y = mode.m * static_cast<float>(constants::PI) * (pickup_l.y * 0.5f + 0.5f);
            const float shape_l = std::cos(ml_x) * std::cos(ml_y) - std::cos(nl_x) * std::cos(nl_y);

            const float mr_x = mode.m * static_cast<float>(constants::PI) * (pickup_r.x * 0.5f + 0.5f);
            const float mr_y = mode.n * static_cast<float>(constants::PI) * (pickup_r.y * 0.5f + 0.5f);
            const float nr_x = mode.n * static_cast<float>(constants::PI) * (pickup_r.x * 0.5f + 0.5f);
            const float nr_y = mode.m * static_cast<float>(constants::PI) * (pickup_r.y * 0.5f + 0.5f);
            const float shape_r = std::cos(mr_x) * std::cos(mr_y) - std::cos(nr_x) * std::cos(nr_y);

            left += current_sample * shape_l;
            right += current_sample * shape_r;

            // Advance phase and decay
            mode.phase += two_pi * mode.frequency * inv_sr;
            if (mode.phase > two_pi) {
                mode.phase -= two_pi;
            }
            mode.amplitude *= (1.0f - mode.decay_rate);
        }

        return {left * 0.35f, right * 0.35f};
    }

    [[nodiscard]] const std::array<PlateMode, MAX_MODES>& modes() const noexcept {
        return modes_;
    }

private:
    float sample_rate_{static_cast<float>(DEFAULT_SAMPLE_RATE)};
    std::array<PlateMode, MAX_MODES> modes_{};

    void init_modes(float fundamental_freq, float stiffness) noexcept {
        size_t idx = 0;
        // Generate (m, n) eigenmode lattice
        for (uint8_t m = 1; m <= 6 && idx < MAX_MODES; ++m) {
            for (uint8_t n = 1; n <= 6 && idx < MAX_MODES; ++n) {
                if (m == n) continue; // Pure diagonals vanish in anti-symmetric Chladni formula

                PlateMode mode;
                mode.m = m;
                mode.n = n;

                // Plate dispersion equation: f_{m,n} ~ fundamental * ((m^2 + n^2) / 5.0)^stiffness
                const float m2n2 = static_cast<float>(m * m + n * n);
                mode.frequency = fundamental_freq * std::pow(m2n2 / 5.0f, 0.95f * stiffness);

                // Higher modes decay faster (air damping + internal thermoelastic friction)
                mode.decay_rate = 0.00015f * (1.0f + 0.08f * m2n2);
                mode.amplitude = 0.0f;
                mode.phase = 0.0f;

                if (mode.frequency < sample_rate_ * 0.48f) {
                    modes_[idx++] = mode;
                }
            }
        }
        // Fill remaining with disabled modes
        while (idx < MAX_MODES) {
            modes_[idx++] = PlateMode{};
        }
    }
};

} // namespace aether::engines
