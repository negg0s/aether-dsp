#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <algorithm>

namespace aether::math {

enum class FixedEnvelopeStage : uint8_t {
    Idle,
    Attack,
    Decay,
    Sustain,
    Release
};

/**
 * @brief Zero-FPU Fixed-point Q15 ADSR Envelope Generator.
 * Executes on embedded microcontrollers with sample-accurate linear/concave segments.
 */
class FixedADSREnvelope {
public:
    constexpr FixedADSREnvelope() noexcept = default;

    void configure(uint32_t attack_samples, uint32_t decay_samples, FixedQ15 sustain_level, uint32_t release_samples) noexcept {
        attack_rate_ = calculate_rate(attack_samples, FixedQ15(0), FixedQ15::from_float(0.999f));
        decay_rate_ = calculate_rate(decay_samples, FixedQ15::from_float(0.999f), sustain_level);
        sustain_level_ = sustain_level;
        release_samples_ = std::max<uint32_t>(1, release_samples);
    }

    void gate(bool on) noexcept {
        if (on) {
            stage_ = FixedEnvelopeStage::Attack;
        } else if (stage_ != FixedEnvelopeStage::Idle) {
            stage_ = FixedEnvelopeStage::Release;
            release_rate_ = calculate_rate(release_samples_, val_, FixedQ15(0));
            if (release_rate_.raw <= 0) {
                stage_ = FixedEnvelopeStage::Idle;
                val_.raw = 0;
            }
        }
    }

    void reset() noexcept {
        stage_ = FixedEnvelopeStage::Idle;
        val_.raw = 0;
    }

    [[nodiscard]] inline FixedQ15 process() noexcept {
        switch (stage_) {
            case FixedEnvelopeStage::Idle:
                val_.raw = 0;
                break;

            case FixedEnvelopeStage::Attack: {
                const int32_t next = static_cast<int32_t>(val_.raw) + static_cast<int32_t>(attack_rate_.raw);
                if (next >= 32767) {
                    val_.raw = 32767;
                    stage_ = FixedEnvelopeStage::Decay;
                } else {
                    val_.raw = static_cast<int16_t>(next);
                }
                break;
            }

            case FixedEnvelopeStage::Decay: {
                const int32_t next = static_cast<int32_t>(val_.raw) - static_cast<int32_t>(decay_rate_.raw);
                if (next <= sustain_level_.raw) {
                    val_.raw = sustain_level_.raw;
                    stage_ = FixedEnvelopeStage::Sustain;
                } else {
                    val_.raw = static_cast<int16_t>(next);
                }
                break;
            }

            case FixedEnvelopeStage::Sustain:
                val_.raw = sustain_level_.raw;
                break;

            case FixedEnvelopeStage::Release: {
                const int32_t next = static_cast<int32_t>(val_.raw) - static_cast<int32_t>(release_rate_.raw);
                if (next <= 0) {
                    val_.raw = 0;
                    stage_ = FixedEnvelopeStage::Idle;
                } else {
                    val_.raw = static_cast<int16_t>(next);
                }
                break;
            }
        }
        return val_;
    }

    [[nodiscard]] constexpr bool is_active() const noexcept {
        return stage_ != FixedEnvelopeStage::Idle;
    }

    [[nodiscard]] constexpr FixedEnvelopeStage stage() const noexcept {
        return stage_;
    }

private:
    FixedEnvelopeStage stage_{FixedEnvelopeStage::Idle};
    FixedQ15 val_{0};
    FixedQ15 sustain_level_{FixedQ15::from_float(0.7f)};
    FixedQ15 attack_rate_{FixedQ15(100)};
    FixedQ15 decay_rate_{FixedQ15(50)};
    FixedQ15 release_rate_{FixedQ15(30)};
    uint32_t release_samples_{1000};

    [[nodiscard]] static constexpr FixedQ15 calculate_rate(uint32_t samples, FixedQ15 from, FixedQ15 to) noexcept {
        samples = samples > 0 ? samples : 1;
        const int32_t from_val = static_cast<int32_t>(from.raw);
        const int32_t to_val = static_cast<int32_t>(to.raw);
        const int32_t diff = to_val >= from_val ? (to_val - from_val) : (from_val - to_val);
        const int32_t rate = diff / static_cast<int32_t>(samples);
        const int32_t clamped_rate = rate > 32767 ? 32767 : (rate > 0 ? rate : 1);
        return FixedQ15(static_cast<int16_t>(clamped_rate));
    }
};

} // namespace aether::math
