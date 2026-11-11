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
        sample_rate = std::clamp(sample_rate, 1000.0f, 384000.0f);
        cutoff_hz = std::clamp(cutoff_hz, 10.0f, sample_rate * 0.49f);
        q = std::max(0.1f, q);

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

        const float inv_a0 = 1.0f / a0;
        b0_ = b0 * inv_a0;
        b1_ = b1 * inv_a0;
        b2_ = b2 * inv_a0;
        a1_ = a1 * inv_a0;
        a2_ = a2 * inv_a0;
    }

    [[nodiscard]] inline Sample process(Sample in) noexcept {
        if (!std::isfinite(in)) in = 0.0f;
        // Direct Form II Transposed execution
        const Sample out = b0_ * in + s1_;
        s1_ = b1_ * in - a1_ * out + s2_;
        s2_ = b2_ * in - a2_ * out;
        if (!std::isfinite(s1_) || !std::isfinite(s2_)) {
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
