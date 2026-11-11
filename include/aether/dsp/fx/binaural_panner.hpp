#pragma once

#include "../../types.hpp"
#include "../biquad.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace aether::dsp::fx {

/**
 * @brief 3D Binaural Head-Related Spatializer (Woodworth ITD + Head Shadow ILD).
 *
 * Implements spherical head acoustics for true 360-degree binaural headphone spatialization:
 * - Interaural Time Difference (ITD): Delta_t = (a / c) * (theta + sin(theta))
 * - Interaural Level Difference (ILD): Frequency-dependent acoustic shadow filter
 */
class BinauralPanner {
public:
    static constexpr float HEAD_RADIUS_M = 0.0875f; // Average human head radius (8.75 cm)
    static constexpr float SPEED_OF_SOUND_MPS = 343.0f; // Speed of sound in dry air at 20C
    static constexpr size_t MAX_DELAY_SAMPLES = 256; // High sample rate head ITD buffer

    explicit BinauralPanner(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(static_cast<float>(std::clamp(sample_rate, 1000u, 384000u))) {
        delay_l_.resize(MAX_DELAY_SAMPLES, 0.0f);
        delay_r_.resize(MAX_DELAY_SAMPLES, 0.0f);
        shadow_l_.configure(FilterType::Lowpass, 4500.0f, 0.707f, sample_rate_);
        shadow_r_.configure(FilterType::Lowpass, 4500.0f, 0.707f, sample_rate_);
    }

    void reset() noexcept {
        std::fill(delay_l_.begin(), delay_l_.end(), 0.0f);
        std::fill(delay_r_.begin(), delay_r_.end(), 0.0f);
        write_idx_ = 0;
        shadow_l_.reset();
        shadow_r_.reset();
    }

    /**
     * @brief Set 3D spatial sound source position.
     * @param azimuth_deg Angle in horizontal plane (-180 to +180 deg, 0 = directly ahead, +90 = right, -90 = left)
     * @param distance_m Distance in meters (0.5 to 10.0m)
     */
    void set_position(float azimuth_deg, float distance_m = 1.0f) noexcept {
        if (!std::isfinite(azimuth_deg)) azimuth_deg = 0.0f;
        if (!std::isfinite(distance_m)) distance_m = 1.0f;
        const float theta_rad = azimuth_deg * static_cast<float>(constants::PI / 180.0);
        distance_m = std::clamp(distance_m, 0.2f, 100.0f);

        // Distance attenuation: 1 / (1 + d^2)
        dist_gain_ = 1.0f / (1.0f + 0.5f * distance_m * distance_m);

        // Woodworth's spherical head ITD model:
        // Project incidence angle from median sagittal plane into [0, pi/2]
        // ensuring physical acoustic symmetry between front (0 deg) and rear (180 deg)
        const float pi = static_cast<float>(constants::PI);
        float abs_t = std::fmod(std::abs(theta_rad), 2.0f * pi);
        if (abs_t > pi) abs_t = 2.0f * pi - abs_t;
        const float theta_inc = (abs_t > 0.5f * pi) ? (pi - abs_t) : abs_t;

        const float sin_t = std::sin(theta_inc);
        const float max_itd_sec = (HEAD_RADIUS_M / SPEED_OF_SOUND_MPS) * (theta_inc + sin_t);
        const float delay_samples = max_itd_sec * sample_rate_;

        const float sin_full = std::sin(theta_rad);
        if (sin_full >= 0.0f) {
            // Sound is on the RIGHT: Right ear leading, Left ear delayed and shadowed
            delay_samples_l_ = std::clamp(delay_samples, 0.0f, static_cast<float>(MAX_DELAY_SAMPLES - 2));
            delay_samples_r_ = 0.0f;
            shadow_amount_l_ = sin_t * 0.75f;
            shadow_amount_r_ = 0.0f;
        } else {
            // Sound is on the LEFT: Left ear leading, Right ear delayed and shadowed
            delay_samples_l_ = 0.0f;
            delay_samples_r_ = std::clamp(delay_samples, 0.0f, static_cast<float>(MAX_DELAY_SAMPLES - 2));
            shadow_amount_l_ = 0.0f;
            shadow_amount_r_ = sin_t * 0.75f;
        }
    }

    [[nodiscard]] StereoFrame process(float mono_in) noexcept {
        if (!std::isfinite(mono_in)) {
            mono_in = 0.0f;
        }

        // Write to fractional delay ring buffers
        delay_l_[write_idx_] = mono_in;
        delay_r_[write_idx_] = mono_in;

        // Read fractional delays via linear interpolation
        const float out_l = read_fractional(delay_l_, delay_samples_l_);
        const float out_r = read_fractional(delay_r_, delay_samples_r_);

        write_idx_ = (write_idx_ + 1) % MAX_DELAY_SAMPLES;

        // Apply Head Shadowing (attenuation of high frequencies on opposite ear)
        const float shadowed_l = shadow_l_.process(out_l);
        const float shadowed_r = shadow_r_.process(out_r);

        float final_l = (out_l * (1.0f - shadow_amount_l_) + shadowed_l * shadow_amount_l_) * dist_gain_;
        float final_r = (out_r * (1.0f - shadow_amount_r_) + shadowed_r * shadow_amount_r_) * dist_gain_;

        if (!std::isfinite(final_l)) final_l = 0.0f;
        if (!std::isfinite(final_r)) final_r = 0.0f;

        return {final_l, final_r};
    }

private:
    float sample_rate_{static_cast<float>(DEFAULT_SAMPLE_RATE)};
    std::vector<float> delay_l_{};
    std::vector<float> delay_r_{};
    size_t write_idx_{0};

    float delay_samples_l_{0.0f};
    float delay_samples_r_{0.0f};
    float shadow_amount_l_{0.0f};
    float shadow_amount_r_{0.0f};
    float dist_gain_{1.0f};

    BiquadFilter shadow_l_;
    BiquadFilter shadow_r_;

    [[nodiscard]] float read_fractional(const std::vector<float>& buf, float delay_smp) const noexcept {
        const float r_idx = static_cast<float>(write_idx_) - delay_smp;
        float norm_idx = std::fmod(r_idx, static_cast<float>(MAX_DELAY_SAMPLES));
        if (norm_idx < 0.0f) norm_idx += static_cast<float>(MAX_DELAY_SAMPLES);

        const size_t i0 = static_cast<size_t>(norm_idx) % MAX_DELAY_SAMPLES;
        const size_t i1 = (i0 + 1) % MAX_DELAY_SAMPLES;
        const float frac = norm_idx - std::floor(norm_idx);

        return buf[i0] * (1.0f - frac) + buf[i1] * frac;
    }
};

} // namespace aether::dsp::fx
