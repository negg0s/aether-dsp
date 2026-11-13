#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Low-Pass Gate (LPG) & Vactrol Simulator
// Models the classic Buchla 292 vactrol (LED + LDR photocell) response:
// simultaneous low-pass filtering and amplitude attenuation with sluggish natural decay ringing.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackLowPassGate {
public:
    enum class Mode : uint8_t {
        Both = 0, // VCA + VCF simultaneously (classic Buchla LPG sound)
        VCA,      // Amplitude only
        VCF       // Filter only
    };

    EurorackLowPassGate() {
        reset();
    }

    void reset() noexcept {
        vactrol_state_ = 0;
        filter_state_ = 0;
        control_mv_ = 0;
        mode_ = Mode::Both;
        vactrol_decay_coeff_ = math::FixedQ15::from_float(0.005f); // sluggish slow vactrol release
    }

    void set_mode(Mode mode) noexcept {
        mode_ = mode;
    }

    // Set vactrol sluggishness/decay tail (0.001 = very slow vintage, 0.05 = fast modern)
    void set_decay(math::FixedQ15 decay) noexcept {
        vactrol_decay_coeff_ = decay;
    }

    // Process a single audio sample given current control voltage (0 to 5000 mV)
    math::FixedQ15 process(math::FixedQ15 audio_in, int16_t cv_mv) noexcept {
        // Vactrol simulation: LED turns on fast, LDR resistance decays slowly
        int32_t target_vactrol = (static_cast<int32_t>(cv_mv) * 32767) / 5000;
        if (target_vactrol > 32767) target_vactrol = 32767;
        if (target_vactrol < 0) target_vactrol = 0;

        int32_t delta = target_vactrol - vactrol_state_;
        if (delta > 0) {
            // Fast attack: LED illuminates LDR rapidly
            vactrol_state_ += (delta * 26214) >> 15; // ~0.8 attack speed
        } else {
            // Slow sluggish decay: photocell phosphor persistence
            vactrol_state_ += (delta * vactrol_decay_coeff_.raw) >> 15;
        }

        if (vactrol_state_ > 32767) vactrol_state_ = 32767;
        if (vactrol_state_ < 0) vactrol_state_ = 0;

        int64_t sig = audio_in.raw;

        // Apply VCF component: 1-pole low-pass filter whose cutoff tracks vactrol brightness
        if (mode_ == Mode::Both || mode_ == Mode::VCF) {
            int64_t g = vactrol_state_; // cutoff follows vactrol
            int64_t diff = sig - filter_state_;
            filter_state_ += (diff * g) >> 15;
            sig = filter_state_;
        }

        // Apply VCA component: amplitude modulation
        if (mode_ == Mode::Both || mode_ == Mode::VCA) {
            sig = (sig * vactrol_state_) >> 15;
        }

        if (sig > 32767) sig = 32767;
        if (sig < -32768) sig = -32768;

        return math::FixedQ15(static_cast<int16_t>(sig));
    }

    [[nodiscard]] math::FixedQ15 vactrol_level() const noexcept {
        return math::FixedQ15(static_cast<int16_t>(vactrol_state_));
    }

private:
    int32_t vactrol_state_{0};
    int32_t filter_state_{0};
    int16_t control_mv_{0};
    Mode mode_{Mode::Both};
    math::FixedQ15 vactrol_decay_coeff_{math::FixedQ15::from_float(0.005f)};
};

} // namespace aether::embedded
