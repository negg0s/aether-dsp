#pragma once

#include "../../types.hpp"
#include "../biquad.hpp"
#include <cmath>
#include <algorithm>
#include <numbers>

namespace aether::dsp::fx {

/**
 * @brief 3D Non-Euclidean Poincaré Ball (H^3) Binaural Acoustic Spatializer.
 *
 * Implements sound propagation on a 3D Riemannian manifold of constant
 * negative Gaussian curvature K = -1 (Poincaré Ball model B^3 = {p in R^3 : ||p|| < 1}).
 *
 * Key Physical Properties:
 *   - Hyperbolic geodesic distance:
 *       d_H(p0, p1) = arcosh(1 + 2*||p0 - p1||^2 / ((1 - ||p0||^2)*(1 - ||p1||^2)))
 *   - Exponential wavefront dissipation:
 *       Spherical area in H^3: A(d_H) = 4*pi*sinh^2(d_H)
 *       Intensity decay: I(d_H) = 1 / (sinh^2(d_H) + eps)
 *   - Conformal angle preservation with Woodworth spherical head binaural ITD/ILD.
 */
class HyperbolicSpatializer3D {
public:
    struct Position3D {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};

        [[nodiscard]] float norm_squared() const noexcept {
            return x * x + y * y + z * z;
        }

        [[nodiscard]] float norm() const noexcept {
            return std::sqrt(norm_squared());
        }
    };

    explicit HyperbolicSpatializer3D(uint32_t sample_rate = 48000)
        : sample_rate_(std::clamp(sample_rate, 1000u, 384000u)), dt_(1.0f / static_cast<float>(std::clamp(sample_rate, 1000u, 384000u))) {
        // Initialize head shadow lowpass filters
        filter_l_.configure(FilterType::Lowpass, 14000.0f, 0.707f, static_cast<float>(sample_rate_));
        filter_r_.configure(FilterType::Lowpass, 14000.0f, 0.707f, static_cast<float>(sample_rate_));
        delay_line_l_.assign(MAX_DELAY_SAMPLES, 0.0f);
        delay_line_r_.assign(MAX_DELAY_SAMPLES, 0.0f);
    }

    /**
     * @brief Compute geodesic distance in Poincare Ball H^3.
     */
    [[nodiscard]] static float geodesic_distance(Position3D p0, Position3D p1) noexcept {
        const float r0_sq = std::min(p0.norm_squared(), 0.98f);
        const float r1_sq = std::min(p1.norm_squared(), 0.98f);

        const float dx = p1.x - p0.x;
        const float dy = p1.y - p0.y;
        const float dz = p1.z - p0.z;
        const float diff_sq = dx * dx + dy * dy + dz * dz;

        const float denom = (1.0f - r0_sq) * (1.0f - r1_sq);
        const float arg = 1.0f + (2.0f * diff_sq) / std::max(denom, 1e-6f);

        return std::acosh(std::max(arg, 1.0f));
    }

    /**
     * @brief Set position of observer and source in Poincare Ball (||p|| < 1.0).
     */
    void set_source_position(Position3D source_pos, Position3D listener_pos = {0.0f, 0.0f, 0.0f}) noexcept {
        if (!std::isfinite(source_pos.x) || !std::isfinite(source_pos.y) || !std::isfinite(source_pos.z)) {
            source_pos = {0.0f, 0.0f, 0.0f};
        }
        if (!std::isfinite(listener_pos.x) || !std::isfinite(listener_pos.y) || !std::isfinite(listener_pos.z)) {
            listener_pos = {0.0f, 0.0f, 0.0f};
        }

        source_ = source_pos;
        listener_ = listener_pos;

        // Ensure positions stay within the open unit ball B^3
        clamp_to_ball(source_);
        clamp_to_ball(listener_);

        // Compute H^3 geodesic distance
        geodesic_dist_ = geodesic_distance(listener_, source_);

        // Hyperbolic sound intensity attenuation: I ~ 1 / sinh^2(d_H)
        const float sinh_d = std::sinh(std::max(geodesic_dist_, 0.25f));
        gain_ = std::clamp(1.0f / (sinh_d * sinh_d), 0.0001f, 1.5f);

        // Compute conformal azimuth and elevation
        const float dx = source_.x - listener_.x;
        const float dy = source_.y - listener_.y;
        const float dz = source_.z - listener_.z;

        azimuth_rad_ = std::atan2(dy, dx);
        const float horiz_dist = std::sqrt(dx * dx + dy * dy);
        elevation_rad_ = std::atan2(dz, std::max(horiz_dist, 1e-4f));

        // Woodworth spherical head model ITD with front/rear acoustic symmetry:
        constexpr float HEAD_RADIUS_M = 0.0875f;
        constexpr float SPEED_OF_SOUND = 343.0f;

        const float pi = static_cast<float>(constants::PI);
        float abs_az = std::fmod(std::abs(azimuth_rad_), 2.0f * pi);
        if (abs_az > pi) abs_az = 2.0f * pi - abs_az;
        const float theta_inc = (abs_az > 0.5f * pi) ? (pi - abs_az) : abs_az;

        const float sin_az = std::sin(theta_inc);
        const float itd_sec = (HEAD_RADIUS_M / SPEED_OF_SOUND) * (sin_az + theta_inc * 0.5f);

        const float delay_samples = itd_sec * sample_rate_;
        const float sin_full = std::sin(azimuth_rad_);
        if (sin_full >= 0.0f) {
            // Source is to the left: right ear is delayed and shadowed
            delay_l_ = 2.0f;
            delay_r_ = 2.0f + delay_samples;
            filter_l_.configure(FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
            // Head shadow lowpass: 3 kHz to 8 kHz depending on angle
            const float shadow_cutoff = 16000.0f - (16000.0f - 3200.0f) * sin_az;
            filter_r_.configure(FilterType::Lowpass, shadow_cutoff, 0.707f, static_cast<float>(sample_rate_));
        } else {
            // Source is to the right: left ear is delayed and shadowed
            delay_l_ = 2.0f + delay_samples;
            delay_r_ = 2.0f;
            const float shadow_cutoff = 16000.0f - (16000.0f - 3200.0f) * sin_az;
            filter_l_.configure(FilterType::Lowpass, shadow_cutoff, 0.707f, static_cast<float>(sample_rate_));
            filter_r_.configure(FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
        }
    }

    /**
     * @brief Process mono audio input into 3D Non-Euclidean Binaural Stereo output.
     */
    [[nodiscard]] StereoFrame process(float mono_in) noexcept {
        if (!std::isfinite(mono_in)) {
            mono_in = 0.0f;
        }

        // Write to ring delay lines
        delay_line_l_[write_idx_] = mono_in;
        delay_line_r_[write_idx_] = mono_in;

        // Read with fractional linear interpolation
        const float sample_l = read_interpolated(delay_line_l_, delay_l_);
        const float sample_r = read_interpolated(delay_line_r_, delay_r_);

        write_idx_ = (write_idx_ + 1) % MAX_DELAY_SAMPLES;

        // Apply acoustic head-shadow filtering and hyperbolic distance attenuation
        float out_l = filter_l_.process(sample_l) * gain_;
        float out_r = filter_r_.process(sample_r) * gain_;

        if (!std::isfinite(out_l)) out_l = 0.0f;
        if (!std::isfinite(out_r)) out_r = 0.0f;

        return StereoFrame{out_l, out_r};
    }

    [[nodiscard]] float geodesic_distance() const noexcept { return geodesic_dist_; }
    [[nodiscard]] float attenuation_gain() const noexcept { return gain_; }

private:
    static constexpr size_t MAX_DELAY_SAMPLES = 256;

    static void clamp_to_ball(Position3D& p) noexcept {
        const float r2 = p.norm_squared();
        if (r2 >= 0.96f) {
            const float scale = 0.95f / std::sqrt(r2);
            p.x *= scale;
            p.y *= scale;
            p.z *= scale;
        }
    }

    [[nodiscard]] float read_interpolated(const std::vector<float>& buf, float delay_smp) const noexcept {
        const float read_pos = static_cast<float>(write_idx_) - delay_smp;
        float norm_idx = std::fmod(read_pos, static_cast<float>(MAX_DELAY_SAMPLES));
        if (norm_idx < 0.0f) norm_idx += static_cast<float>(MAX_DELAY_SAMPLES);
        if (!std::isfinite(norm_idx)) norm_idx = 0.0f;

        const size_t i0 = static_cast<size_t>(norm_idx) % MAX_DELAY_SAMPLES;
        const size_t i1 = (i0 + 1) % MAX_DELAY_SAMPLES;
        const float frac = norm_idx - std::floor(norm_idx);

        return buf[i0] * (1.0f - frac) + buf[i1] * frac;
    }

    uint32_t sample_rate_{48000};
    float dt_{1.0f / 48000.0f};

    Position3D source_{0.4f, 0.3f, 0.0f};
    Position3D listener_{0.0f, 0.0f, 0.0f};

    float geodesic_dist_{0.0f};
    float gain_{1.0f};
    float azimuth_rad_{0.0f};
    float elevation_rad_{0.0f};

    float delay_l_{2.0f};
    float delay_r_{2.0f};
    size_t write_idx_{0};

    std::vector<float> delay_line_l_;
    std::vector<float> delay_line_r_;
    BiquadFilter filter_l_;
    BiquadFilter filter_r_;
};

} // namespace aether::dsp::fx
