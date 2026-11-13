#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Wave Terrain Morphing Macro-Oscillator
// Multi-algorithm macro-oscillator core (similar to Mutable Instruments Braids / Plaits):
// Integrates 4 synthesis models:
// 0: Virtual Analog Dual Saw (Supersaw with detuning)
// 1: Variable Timbre Phase Distortion (CZ resonant peak)
// 2: Harmonic Additive Formant Cluster
// 3: 2-Operator Phase Modulation (FM metallic bell)
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackMacroOscillator {
public:
    enum class Model : uint8_t {
        VirtualAnalogSaw = 0,
        PhaseDistortion,
        HarmonicCluster,
        FrequencyModulation
    };

    EurorackMacroOscillator() {
        reset();
    }

    void reset() noexcept {
        phase1_ = 0;
        phase2_ = 0;
        phase_inc_ = 1000000; // ~440 Hz
        timbre_ = math::FixedQ15::from_float(0.5f);
        morph_ = math::FixedQ15::from_float(0.5f);
        model_ = Model::VirtualAnalogSaw;
    }

    void configure(Model model, uint32_t phase_inc, math::FixedQ15 timbre, math::FixedQ15 morph) noexcept {
        model_ = model;
        phase_inc_ = phase_inc;
        timbre_ = timbre;
        morph_ = morph;
    }

    // Process a single audio sample based on selected macro synthesis engine
    math::FixedQ15 process() noexcept {
        phase1_ += phase_inc_;
        // Phase 2 slightly detuned by morph for unison / carrier
        uint32_t detune_inc = phase_inc_ + ((phase_inc_ >> 7) * morph_.raw >> 15);
        phase2_ += detune_inc;

        int32_t out_sig = 0;

        switch (model_) {
            case Model::VirtualAnalogSaw: {
                // Dual detuned saw waves
                int32_t saw1 = static_cast<int32_t>(phase1_ >> 16) - 32768;
                int32_t saw2 = static_cast<int32_t>(phase2_ >> 16) - 32768;
                // Timbre controls pulse/saw mix
                int32_t pulse = (phase1_ < 0x80000000) ? 24000 : -24000;
                int32_t blended_saw = (saw1 + saw2) >> 1;
                out_sig = ((blended_saw * (32767 - timbre_.raw)) >> 15) + ((pulse * timbre_.raw) >> 15);
                break;
            }

            case Model::PhaseDistortion: {
                // Casio CZ-style resonant phase distortion:
                // Non-linear warp of phase ramp creates resonant formant peaks
                uint32_t p = phase1_;
                uint32_t inflection = 0x20000000 + (static_cast<uint32_t>(timbre_.raw) << 15);
                uint32_t warped_phase = 0;
                if (p < inflection) {
                    warped_phase = static_cast<uint32_t>((static_cast<uint64_t>(p) << 31) / inflection);
                } else {
                    warped_phase = 0x80000000 + static_cast<uint32_t>((static_cast<uint64_t>(p - inflection) << 31) / (0xFFFFFFFF - inflection + 1));
                }
                out_sig = approx_sin(warped_phase >> 16);
                break;
            }

            case Model::HarmonicCluster: {
                // Additive harmonics: f, 2f, 3f weighted by timbre and morph
                int32_t h1 = approx_sin(phase1_ >> 16);
                int32_t h2 = approx_sin((phase1_ * 2) >> 16);
                int32_t h3 = approx_sin((phase1_ * 3) >> 16);
                out_sig = (h1 >> 1) + ((h2 * timbre_.raw) >> 16) + ((h3 * morph_.raw) >> 16);
                break;
            }

            case Model::FrequencyModulation: {
                // 2-Op Phase Modulation: Modulator phase2 modulates Carrier phase1
                int32_t modulator = approx_sin(phase2_ >> 16);
                int32_t mod_depth = (modulator * timbre_.raw) >> 12; // modulation index
                uint32_t carrier_phase = phase1_ + (static_cast<uint32_t>(mod_depth) << 16);
                out_sig = approx_sin(carrier_phase >> 16);
                break;
            }
        }

        if (out_sig > 32767) out_sig = 32767;
        if (out_sig < -32768) out_sig = -32768;

        return math::FixedQ15(static_cast<int16_t>(out_sig));
    }

private:
    static int32_t approx_sin(uint32_t phase16) noexcept {
        int32_t tri = 0;
        if (phase16 < 32768) {
            tri = static_cast<int32_t>(phase16) - 16384;
        } else {
            tri = 49152 - static_cast<int32_t>(phase16);
        }
        int32_t abs_tri = (tri < 0) ? -tri : tri;
        return (2 * tri) - ((tri * abs_tri) >> 14);
    }

    uint32_t phase1_{0};
    uint32_t phase2_{0};
    uint32_t phase_inc_{1000000};
    math::FixedQ15 timbre_{math::FixedQ15::from_float(0.5f)};
    math::FixedQ15 morph_{math::FixedQ15::from_float(0.5f)};
    Model model_{Model::VirtualAnalogSaw};
};

} // namespace aether::embedded
