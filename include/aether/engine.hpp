#pragma once

#include "types.hpp"
#include "ring_buffer.hpp"
#include "dsp/biquad.hpp"
#include "dsp/envelope.hpp"
#include "dsp/tuning.hpp"
#include "dsp/scala_catalog.hpp"
#include "dsp/fx/hyperbolic_reverb.hpp"
#include "engines/hyperbolic_waveguide.hpp"
#include "engines/chladni_plate.hpp"
#include "engines/chaotic_oscillator.hpp"
#include "engines/stick_slip_exciter.hpp"
#include <span>
#include <string_view>
#include <algorithm>

namespace aether {

enum class Preset {
    HyperbolicCosmicBell,  // Non-Euclidean Poincare FDTD resonance with exponential dispersion
    ChladniPlateGong,      // 2D Nodal thin elastic plate physical model
    ChaoticQuantumDrone,   // Lorenz/Clifford Strange Attractor non-linear harmonic bed
    HyperbolicAetherSpace, // Hybrid hyperbolic waveguide with chaotic dynamic spatial excitation
    BowedCrystalPlate      // Continuous Stribeck stick-slip friction bowed elastic plate
};

/**
 * @brief Master AetherDSP Sound Synthesis Engine.
 *
 * Provides real-time block processing, parameter morphing, microtonal scale tuning,
 * and hyperbolic FDN reverberation.
 * Thread-safe for audio callbacks with strict zero-allocation real-time constraints.
 */
class Engine {
public:
    explicit Engine(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(sample_rate) {
        init();
    }

    void init() noexcept {
        filter_l_.configure(dsp::FilterType::Lowpass, 12000.0f, 0.707f, static_cast<float>(sample_rate_));
        filter_r_.configure(dsp::FilterType::Lowpass, 12000.0f, 0.707f, static_cast<float>(sample_rate_));
        env_.configure(0.005f, 0.8f, 0.0f, 1.5f, static_cast<float>(sample_rate_));
        reverb_.configure(0.82f, 0.25f, 0.28f);
        load_preset(Preset::HyperbolicCosmicBell);
    }

    void load_preset(Preset preset) noexcept {
        current_preset_ = preset;
        switch (preset) {
            case Preset::HyperbolicCosmicBell:
                waveguide_.configure(0.48f, 0.0025f, 1.2f);
                filter_l_.configure(dsp::FilterType::Lowpass, 14000.0f, 0.8f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 14000.0f, 0.8f, static_cast<float>(sample_rate_));
                reverb_.configure(0.85f, 0.25f, 0.32f);
                break;

            case Preset::ChladniPlateGong:
                chladni_.configure(180.0f, 1.15f, static_cast<float>(sample_rate_));
                filter_l_.configure(dsp::FilterType::Lowpass, 9500.0f, 1.0f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 9500.0f, 1.0f, static_cast<float>(sample_rate_));
                reverb_.configure(0.88f, 0.35f, 0.25f);
                break;

            case Preset::ChaoticQuantumDrone:
                chaos_.configure(engines::AttractorType::Lorenz, 0.85f);
                filter_l_.configure(dsp::FilterType::Lowpass, 4500.0f, 2.5f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 4500.0f, 2.5f, static_cast<float>(sample_rate_));
                reverb_.configure(0.92f, 0.15f, 0.40f);
                break;

            case Preset::HyperbolicAetherSpace:
                waveguide_.configure(0.38f, 0.0018f, 1.6f);
                chaos_.configure(engines::AttractorType::Clifford, 0.2f);
                filter_l_.configure(dsp::FilterType::Lowpass, 11000.0f, 1.2f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 11000.0f, 1.2f, static_cast<float>(sample_rate_));
                reverb_.configure(0.90f, 0.20f, 0.35f);
                break;

            case Preset::BowedCrystalPlate:
                chladni_.configure(320.0f, 0.95f, static_cast<float>(sample_rate_));
                exciter_.configure(0.85f, 0.30f, 3.5f);
                filter_l_.configure(dsp::FilterType::Lowpass, 15000.0f, 0.9f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 15000.0f, 0.9f, static_cast<float>(sample_rate_));
                reverb_.configure(0.94f, 0.12f, 0.42f);
                break;
        }
    }

    /**
     * @brief Trigger note with excitation coordinates.
     */
    void note_on(Vec2 strike_coord = {0.2f, 0.3f}, float velocity = 0.85f) noexcept {
        env_.reset();
        env_.gate(true);

        if (current_preset_ == Preset::HyperbolicCosmicBell || current_preset_ == Preset::HyperbolicAetherSpace) {
            waveguide_.strike(strike_coord, velocity, 0.15f);
        }

        if (current_preset_ == Preset::ChladniPlateGong || current_preset_ == Preset::HyperbolicAetherSpace) {
            chladni_.strike(strike_coord, velocity);
        }
    }

    /**
     * @brief Trigger microtonal note degree via current tuning system.
     */
    void note_on_degree(int degree, float velocity = 0.85f, Vec2 strike_coord = {0.3f, 0.4f}) noexcept {
        const float freq = tuning_.get_frequency(degree, 220.0f);
        if (current_preset_ == Preset::ChladniPlateGong) {
            chladni_.configure(freq, 1.15f, static_cast<float>(sample_rate_));
        }
        note_on(strike_coord, velocity);
    }

    void note_off() noexcept {
        env_.gate(false);
    }

    /**
     * @brief Real-time audio rendering block process.
     * ZERO heap allocation, strictly noexcept, AVX2-accelerated.
     */
    void process_block(std::span<StereoFrame> output) noexcept {
        for (auto& frame : output) {
            StereoFrame sample{0.0f, 0.0f};

            switch (current_preset_) {
                case Preset::HyperbolicCosmicBell: {
                    sample = waveguide_.step({-0.45f, 0.25f}, {0.45f, -0.25f});
                    break;
                }

                case Preset::ChladniPlateGong: {
                    sample = chladni_.step({-0.5f, -0.5f}, {0.5f, 0.5f});
                    break;
                }

                case Preset::ChaoticQuantumDrone: {
                    sample = chaos_.step(static_cast<float>(sample_rate_));
                    break;
                }

                case Preset::HyperbolicAetherSpace: {
                    const auto chaos_mod = chaos_.step(static_cast<float>(sample_rate_));
                    if (chaos_step_count_++ % 32 == 0) {
                        const Vec2 perturb{chaos_mod.left * 0.3f, chaos_mod.right * 0.3f};
                        waveguide_.strike(perturb, 0.08f, 0.08f);
                    }
                    const auto wg = waveguide_.step({-0.5f, 0.3f}, {0.5f, -0.3f});
                    const auto pl = chladni_.step({-0.3f, -0.3f}, {0.3f, 0.3f});
                    sample = wg * 0.7f + pl * 0.3f;
                    break;
                }

                case Preset::BowedCrystalPlate: {
                    // Continuous stick-slip friction feedback loop
                    const auto pl = chladni_.step({-0.4f, -0.4f}, {0.4f, 0.4f});
                    const float friction_force = exciter_.compute_force(0.25f, 1.2f, (pl.left + pl.right) * 0.5f);
                    chladni_.strike({0.35f, 0.45f}, friction_force * 0.015f);
                    sample = pl * 0.85f;
                    break;
                }
            }

            // Master lowpass filter
            sample.left = filter_l_.process(sample.left);
            sample.right = filter_r_.process(sample.right);

            // Hyperbolic FDN Reverb
            sample = reverb_.process(sample);

            // Soft-clipping saturation
            auto soft_clip = [](Sample x) noexcept -> Sample {
                return std::clamp(x * (1.5f - 0.5f * x * x), -1.0f, 1.0f);
            };

            frame.left = soft_clip(sample.left);
            frame.right = soft_clip(sample.right);
        }
    }

    [[nodiscard]] engines::HyperbolicWaveguide& waveguide() noexcept { return waveguide_; }
    [[nodiscard]] engines::ChladniPlate& chladni() noexcept { return chladni_; }
    [[nodiscard]] engines::ChaoticOscillator& chaos() noexcept { return chaos_; }
    [[nodiscard]] dsp::TuningSystem& tuning() noexcept { return tuning_; }
    [[nodiscard]] dsp::fx::HyperbolicReverb& reverb() noexcept { return reverb_; }
    [[nodiscard]] uint32_t sample_rate() const noexcept { return sample_rate_; }

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    Preset current_preset_{Preset::HyperbolicCosmicBell};

    engines::HyperbolicWaveguide waveguide_;
    engines::ChladniPlate chladni_;
    engines::ChaoticOscillator chaos_;
    engines::StickSlipExciter exciter_;
    dsp::BiquadFilter filter_l_;
    dsp::BiquadFilter filter_r_;
    dsp::ADSREnvelope env_;
    dsp::fx::HyperbolicReverb reverb_;
    dsp::TuningSystem tuning_{dsp::TuningSystem::PredefinedScale::BohlenPierce};

    uint32_t chaos_step_count_{0};
};

} // namespace aether
