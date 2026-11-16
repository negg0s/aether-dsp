#pragma once

#include "../types.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace aether::dsp {

enum class FilterType {
    Lowpass,
    Highpass,
    Bandpass,
    Notch,
    Allpass
};

/**
 * @brief Direct Form II Transposed Biquad Filter.
 * Direct Form II Transposed implementation with parameter smoothing.
 */
class BiquadFilter {
public:
    constexpr BiquadFilter() noexcept = default;

    void reset() noexcept {
        s1_ = 0.0f;
        s2_ = 0.0f;
    }

    void configure(FilterType type, float cutoff_hz, float q, float sample_rate = static_cast<float>(DEFAULT_SAMPLE_RATE)) noexcept {
        if (!std::isfinite(sample_rate) || sample_rate <= 0.0f) {
            sample_rate = static_cast<float>(DEFAULT_SAMPLE_RATE);
        }
        sample_rate = std::clamp(sample_rate, 1000.0f, 384000.0f);

        if (!std::isfinite(cutoff_hz) || cutoff_hz <= 0.0f) {
            cutoff_hz = 1000.0f;
        }
        cutoff_hz = std::clamp(cutoff_hz, 10.0f, sample_rate * 0.49f);

        if (!std::isfinite(q) || q <= 0.0f) {
            q = 0.707f;
        }
        q = std::clamp(q, 0.05f, 100.0f);

        const float omega = static_cast<float>(2.0 * constants::PI) * cutoff_hz / sample_rate;
        const float sin_w = std::sin(omega);
        const float cos_w = std::cos(omega);
        const float alpha = sin_w / (2.0f * q);

        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

        switch (type) {
            case FilterType::Lowpass:
                b0 = (1.0f - cos_w) * 0.5f;
                b1 = 1.0f - cos_w;
                b2 = (1.0f - cos_w) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_w;
                a2 = 1.0f - alpha;
                break;
            case FilterType::Highpass:
                b0 = (1.0f + cos_w) * 0.5f;
                b1 = -(1.0f + cos_w);
                b2 = (1.0f + cos_w) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_w;
                a2 = 1.0f - alpha;
                break;
            case FilterType::Bandpass:
                b0 = alpha;
                b1 = 0.0f;
                b2 = -alpha;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_w;
                a2 = 1.0f - alpha;
                break;
            case FilterType::Notch:
                b0 = 1.0f;
                b1 = -2.0f * cos_w;
                b2 = 1.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_w;
                a2 = 1.0f - alpha;
                break;
            case FilterType::Allpass:
                b0 = 1.0f - alpha;
                b1 = -2.0f * cos_w;
                b2 = 1.0f + alpha;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_w;
                a2 = 1.0f - alpha;
                break;
        }

        if (std::abs(a0) < 1e-15f || !std::isfinite(a0)) {
            // Unstable or degenerate configuration: reset to bypass
            b0_ = 1.0f;
            b1_ = 0.0f;
            b2_ = 0.0f;
            a1_ = 0.0f;
            a2_ = 0.0f;
            return;
        }

        const float inv_a0 = 1.0f / a0;
        const float norm_b0 = b0 * inv_a0;
        const float norm_b1 = b1 * inv_a0;
        const float norm_b2 = b2 * inv_a0;
        const float norm_a1 = a1 * inv_a0;
        const float norm_a2 = a2 * inv_a0;

        // Stability triangle check for 2nd order IIR: |a2| < 1 and |a1| < 1 + a2
        if (!std::isfinite(norm_a1) || !std::isfinite(norm_a2) ||
            std::abs(norm_a2) >= 1.0f || std::abs(norm_a1) >= (1.0f + norm_a2)) {
            b0_ = 1.0f;
            b1_ = 0.0f;
            b2_ = 0.0f;
            a1_ = 0.0f;
            a2_ = 0.0f;
            reset();
            return;
        }

        b0_ = norm_b0;
        b1_ = norm_b1;
        b2_ = norm_b2;
        a1_ = norm_a1;
        a2_ = norm_a2;
    }

    [[nodiscard]] inline Sample process(Sample in) noexcept {
        if (!std::isfinite(in)) in = 0.0f;
        // Direct Form II Transposed execution
        const Sample out = b0_ * in + s1_;
        s1_ = b1_ * in - a1_ * out + s2_;
        s2_ = b2_ * in - a2_ * out;
        if (!std::isfinite(out) || !std::isfinite(s1_) || !std::isfinite(s2_)) {
            reset();
            return 0.0f;
        }
        if (std::abs(s1_) < 1e-15f) s1_ = 0.0f;
        if (std::abs(s2_) < 1e-15f) s2_ = 0.0f;
        return out;
    }

private:
    float b0_{1.0f}, b1_{0.0f}, b2_{0.0f};
    float a1_{0.0f}, a2_{0.0f};
    float s1_{0.0f}, s2_{0.0f};
};

} // namespace aether::dsp
