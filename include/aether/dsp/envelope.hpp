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
        sample_rate_ = sample_rate;
        sustain_level_ = std::clamp(sustain_level, 0.0f, 1.0f);

        attack_rate_ = calculate_rate(attack_sec, 0.0f, 1.0f);
        decay_rate_ = calculate_rate(decay_sec, 1.0f, sustain_level_);
        release_rate_ = calculate_rate(release_sec, sustain_level_, 0.0f);
    }

    void gate(bool on) noexcept {
        if (on) {
            stage_ = EnvelopeStage::Attack;
        } else if (stage_ != EnvelopeStage::Idle) {
            stage_ = EnvelopeStage::Release;
        }
    }

    void reset() noexcept {
        stage_ = EnvelopeStage::Idle;
        value_ = 0.0f;
    }

    [[nodiscard]] inline Sample process() noexcept {
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
