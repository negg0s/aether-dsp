#pragma once

#include "../types.hpp"
#include <cmath>
#include <algorithm>

namespace aether::dsp {

enum class EnvelopeStage {
    Idle,
    Attack,
    Decay,
    Sustain,
    Release
};

/**
 * @brief High-precision sample-accurate Exponential ADSR Envelope.
 */
class ADSREnvelope {
public:
    constexpr ADSREnvelope() noexcept = default;

    void configure(float attack_sec, float decay_sec, float sustain_level, float release_sec, float sample_rate = static_cast<float>(DEFAULT_SAMPLE_RATE)) noexcept {
        sample_rate_ = std::clamp(sample_rate, 1000.0f, 384000.0f);
        sustain_level_ = std::clamp(sustain_level, 0.0f, 1.0f);
        attack_sec_ = std::max(0.0001f, attack_sec);
        decay_sec_ = std::max(0.0001f, decay_sec);
        release_sec_ = std::max(0.0001f, release_sec);

        attack_rate_ = calculate_rate(attack_sec_, 0.0f, 1.0f);
        decay_rate_ = calculate_rate(decay_sec_, 1.0f, sustain_level_);
        release_rate_ = calculate_rate(release_sec_, std::max(0.01f, sustain_level_), 0.0f);
    }

    void gate(bool on) noexcept {
        if (on) {
            stage_ = EnvelopeStage::Attack;
        } else if (stage_ != EnvelopeStage::Idle) {
            stage_ = EnvelopeStage::Release;
            const float samples = release_sec_ * sample_rate_;
            release_rate_ = (samples > 0.0f) ? (value_ / samples) : value_;
            if (release_rate_ <= 0.0f) {
                stage_ = EnvelopeStage::Idle;
                value_ = 0.0f;
            }
        }
    }

    void reset() noexcept {
        stage_ = EnvelopeStage::Idle;
        value_ = 0.0f;
    }

    [[nodiscard]] inline Sample process() noexcept {
        if (!std::isfinite(value_)) {
            value_ = 0.0f;
            stage_ = EnvelopeStage::Idle;
            return 0.0f;
        }

        switch (stage_) {
            case EnvelopeStage::Idle:
                value_ = 0.0f;
                break;

            case EnvelopeStage::Attack:
                value_ += attack_rate_;
                if (value_ >= 1.0f) {
                    value_ = 1.0f;
                    stage_ = EnvelopeStage::Decay;
                }
                break;

            case EnvelopeStage::Decay:
                value_ -= decay_rate_;
                if (value_ <= sustain_level_) {
                    value_ = sustain_level_;
                    stage_ = EnvelopeStage::Sustain;
                }
                break;

            case EnvelopeStage::Sustain:
                value_ = sustain_level_;
                break;

            case EnvelopeStage::Release:
                value_ -= release_rate_;
                if (value_ <= 0.0f) {
                    value_ = 0.0f;
                    stage_ = EnvelopeStage::Idle;
                }
                break;
        }
        return value_;
    }

    [[nodiscard]] constexpr bool is_active() const noexcept {
        return stage_ != EnvelopeStage::Idle;
    }

    [[nodiscard]] constexpr EnvelopeStage stage() const noexcept {
        return stage_;
    }

private:
    float sample_rate_{static_cast<float>(DEFAULT_SAMPLE_RATE)};
    EnvelopeStage stage_{EnvelopeStage::Idle};
    float value_{0.0f};
    float sustain_level_{0.7f};
    float attack_sec_{0.005f};
    float decay_sec_{0.8f};
    float release_sec_{1.2f};
    float attack_rate_{0.01f};
    float decay_rate_{0.005f};
    float release_rate_{0.001f};

    [[nodiscard]] float calculate_rate(float time_sec, float from, float to) const noexcept {
        time_sec = std::max(0.001f, time_sec);
        const float samples = time_sec * sample_rate_;
        return std::abs(to - from) / samples;
    }
};

} // namespace aether::dsp
