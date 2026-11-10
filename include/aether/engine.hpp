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
#include "engines/von_karman_plate.hpp"
#include "engines/pseudosphere_horn.hpp"
#include "engines/hamiltonian_chaos.hpp"
#include "engines/hyperbolic_string.hpp"
#include "engines/sine_gordon_breather.hpp"
#include "engines/acoustic_black_hole.hpp"
#include "engines/acoustic_wormhole.hpp"
#include "engines/topological_metamaterial.hpp"
#include "engines/kdv_soliton_lattice.hpp"
#include "dsp/fx/hyperbolic_spatializer_3d.hpp"
#include "polyphony.hpp"
#include <span>
#include <string_view>
#include <algorithm>

namespace aether {

enum class Preset {
    HyperbolicCosmicBell,    // Non-Euclidean Poincare FDTD resonance with exponential dispersion
    ChladniPlateGong,        // 2D Nodal thin elastic plate physical model
    ChaoticQuantumDrone,     // Lorenz/Clifford Strange Attractor non-linear harmonic bed
    HyperbolicAetherSpace,   // Hybrid hyperbolic waveguide with chaotic dynamic spatial excitation
    BowedCrystalPlate,       // Continuous Stribeck stick-slip friction bowed elastic plate
    VonKarmanTurbulentGong,  // Nonlinear thin plate large-deflection Duffing pitch glide & shimmering crescendo
    PseudosphereTractrixHorn,// Beltrami pseudosphere tractrix horn Webster flare solver with lip-reed excitation
    HenonHeilesChaos,        // Galactic axisymmetric gravitational Hamiltonian chaos with 4th-order Yoshida integration
    HyperbolicStringJawari,  // Nonlinear stiff Kirchhoff string with tension modulation & buzzing jawari obstacle
    SineGordonBreather,      // Integrable relativistic topological soliton & pulsating breather field
    AcousticBlackHole,       // Retarding acoustic metamaterial waveguide with Unruh sonic horizon & Hawking radiation
    AcousticWormhole,        // Multi-manifold Morris-Thorne throat coupling & quasinormal resonance
    TopologicalMetamaterial, // Valley-Hall phononic crystal with backscattering-immune Dirac edge states
    KdvSolitonLattice        // Toda completely integrable lattice with non-thermalizing solitary shocks
};

/**
 * @brief Master AetherDSP Sound Synthesis Engine.
 *
 * Provides real-time block processing, parameter morphing, microtonal scale tuning,
 * polyphonic voice management, and hyperbolic FDN reverberation.
 * Thread-safe for audio callbacks with strict zero-allocation real-time constraints.
 */
class Engine {
public:
    explicit Engine(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(sample_rate >= 8000 && sample_rate <= 192000 ? sample_rate : DEFAULT_SAMPLE_RATE),
          von_karman_(sample_rate_),
          pseudosphere_(sample_rate_),
          string_(sample_rate_),
          sine_gordon_(sample_rate_),
          acoustic_bh_(sample_rate_),
          wormhole_(sample_rate_),
          topological_(sample_rate_),
          soliton_lattice_(sample_rate_),
          spatializer3d_(sample_rate_),
          polyphony_(sample_rate_) {
        init();
    }

    void init() noexcept {
        filter_l_.configure(dsp::FilterType::Lowpass, 12000.0f, 0.707f, static_cast<float>(sample_rate_));
        filter_r_.configure(dsp::FilterType::Lowpass, 12000.0f, 0.707f, static_cast<float>(sample_rate_));
        env_.configure(0.005f, 0.8f, 0.7f, 1.2f, static_cast<float>(sample_rate_));
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
                env_.configure(0.05f, 0.1f, 1.0f, 0.5f, static_cast<float>(sample_rate_));
                env_.gate(true);
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

            case Preset::VonKarmanTurbulentGong:
                von_karman_.init_modes(90.0f, 1.2f, 0.7f);
                filter_l_.configure(dsp::FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
                reverb_.configure(0.85f, 0.20f, 0.30f);
                break;

            case Preset::PseudosphereTractrixHorn:
                pseudosphere_.reset();
                filter_l_.configure(dsp::FilterType::Lowpass, 12000.0f, 0.85f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 12000.0f, 0.85f, static_cast<float>(sample_rate_));
                reverb_.configure(0.80f, 0.28f, 0.25f);
                break;

            case Preset::HenonHeilesChaos:
                henon_heiles_.reset(0.14);
                henon_heiles_.set_time_scale(320.0);
                filter_l_.configure(dsp::FilterType::Lowpass, 5500.0f, 1.8f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 5500.0f, 1.8f, static_cast<float>(sample_rate_));
                reverb_.configure(0.90f, 0.18f, 0.38f);
                break;

            case Preset::HyperbolicStringJawari:
                string_.reset();
                string_.set_pitch(146.83f);
                filter_l_.configure(dsp::FilterType::Lowpass, 18000.0f, 0.707f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 18000.0f, 0.707f, static_cast<float>(sample_rate_));
                reverb_.configure(0.80f, 0.22f, 0.28f);
                break;

            case Preset::SineGordonBreather:
                sine_gordon_.reset();
                sine_gordon_.configure(0.42f, 220.0f, 0.0006f);
                sine_gordon_.inject_breather(0.5f, 0.72f, 1.2f);
                filter_l_.configure(dsp::FilterType::Lowpass, 14000.0f, 0.707f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 14000.0f, 0.707f, static_cast<float>(sample_rate_));
                reverb_.configure(0.85f, 0.22f, 0.30f);
                break;

            case Preset::AcousticBlackHole:
                acoustic_bh_.reset();
                acoustic_bh_.configure(2.2f, 0.12f, 0.008f, 0.04f);
                filter_l_.configure(dsp::FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
                reverb_.configure(0.88f, 0.25f, 0.32f);
                break;

            case Preset::AcousticWormhole:
                wormhole_.reset();
                wormhole_.configure(0.35f, 1.4f, 0.42f, 0.0006f);
                filter_l_.configure(dsp::FilterType::Lowpass, 14000.0f, 0.707f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 14000.0f, 0.707f, static_cast<float>(sample_rate_));
                reverb_.configure(0.86f, 0.24f, 0.30f);
                break;

            case Preset::TopologicalMetamaterial:
                topological_.reset();
                topological_.configure(0.45f, 0.38f, 0.0005f);
                filter_l_.configure(dsp::FilterType::Lowpass, 15000.0f, 0.707f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 15000.0f, 0.707f, static_cast<float>(sample_rate_));
                reverb_.configure(0.82f, 0.20f, 0.28f);
                break;

            case Preset::KdvSolitonLattice:
                soliton_lattice_.reset();
                soliton_lattice_.configure(1.8f, 340.0f, 0.0004f);
                filter_l_.configure(dsp::FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
                filter_r_.configure(dsp::FilterType::Lowpass, 16000.0f, 0.707f, static_cast<float>(sample_rate_));
                reverb_.configure(0.85f, 0.22f, 0.30f);
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

        if (current_preset_ == Preset::ChladniPlateGong || current_preset_ == Preset::HyperbolicAetherSpace || current_preset_ == Preset::BowedCrystalPlate) {
            chladni_.strike(strike_coord, velocity);
        }

        if (current_preset_ == Preset::VonKarmanTurbulentGong) {
            von_karman_.strike(strike_coord, velocity);
        }

        if (current_preset_ == Preset::PseudosphereTractrixHorn) {
            pseudosphere_.excite_throat(velocity * 0.8f);
        }

        if (current_preset_ == Preset::HenonHeilesChaos) {
            henon_heiles_.reset(0.11 + 0.045 * std::clamp(velocity, 0.1f, 1.0f));
        }

        if (current_preset_ == Preset::HyperbolicStringJawari) {
            string_.pluck(strike_coord.x * 0.4f + 0.5f, velocity, 0.08f);
        }

        if (current_preset_ == Preset::SineGordonBreather) {
            sine_gordon_.strike(strike_coord.x * 0.5f + 0.25f, velocity);
        }

        if (current_preset_ == Preset::AcousticBlackHole) {
            acoustic_bh_.strike(strike_coord.x * 0.4f + 0.5f, velocity);
        }

        if (current_preset_ == Preset::AcousticWormhole) {
            wormhole_.strike_universe_a(strike_coord.x * 0.4f + 0.4f, velocity);
        }

        if (current_preset_ == Preset::TopologicalMetamaterial) {
            topological_.strike_edge(velocity);
        }

        if (current_preset_ == Preset::KdvSolitonLattice) {
            soliton_lattice_.strike(strike_coord.x * 0.4f + 0.3f, velocity);
        }
    }

    /**
     * @brief Trigger microtonal note degree via current tuning system.
     */
    void note_on_degree(int degree, float velocity = 0.85f, Vec2 strike_coord = {0.3f, 0.4f}) noexcept {
        const float freq = tuning_.get_frequency(degree, 220.0f);
        if (current_preset_ == Preset::ChladniPlateGong) {
            chladni_.configure(freq, 1.15f, static_cast<float>(sample_rate_));
        } else if (current_preset_ == Preset::VonKarmanTurbulentGong) {
            von_karman_.init_modes(freq * 0.5f, 1.2f, 0.7f);
        } else if (current_preset_ == Preset::HyperbolicStringJawari) {
            string_.set_pitch(freq);
        } else if (current_preset_ == Preset::SineGordonBreather) {
            sine_gordon_.configure(0.42f, freq, 0.0006f);
            sine_gordon_.inject_breather(0.5f, 0.72f, 1.0f);
        } else if (current_preset_ == Preset::KdvSolitonLattice) {
            soliton_lattice_.configure(1.8f, freq * 1.5f, 0.0004f);
        }
        note_on(strike_coord, velocity);
    }

    void note_off() noexcept {
        env_.gate(false);
    }

    /**
     * @brief Trigger polyphonic note using microtonal tuning scale.
     */
    void poly_note_on(int midi_note, float velocity = 0.85f) noexcept {
        const float freq = tuning_.get_frequency(midi_note - 69, 440.0f);
        polyphony_.note_on(midi_note, freq, velocity);
    }

    /**
     * @brief Release polyphonic note.
     */
    void poly_note_off(int midi_note) noexcept {
        polyphony_.note_off(midi_note);
    }

    /**
     * @brief Turn off all polyphonic voices.
     */
    void all_notes_off() noexcept {
        polyphony_.all_notes_off();
        note_off();
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
                    const auto pl = chladni_.step({-0.4f, -0.4f}, {0.4f, 0.4f});
                    const float friction_force = exciter_.compute_force(0.25f, 1.2f, (pl.left + pl.right) * 0.5f);
                    chladni_.strike({0.35f, 0.45f}, friction_force * 0.015f);
                    sample = pl * 0.85f;
                    break;
                }

                case Preset::VonKarmanTurbulentGong: {
                    sample = von_karman_.step();
                    break;
                }

                case Preset::PseudosphereTractrixHorn: {
                    sample = pseudosphere_.step();
                    break;
                }

                case Preset::HenonHeilesChaos: {
                    sample = henon_heiles_.step(static_cast<float>(sample_rate_));
                    break;
                }

                case Preset::HyperbolicStringJawari: {
                    sample = string_.step();
                    break;
                }

                case Preset::SineGordonBreather: {
                    sample = sine_gordon_.process();
                    break;
                }

                case Preset::AcousticBlackHole: {
                    sample = acoustic_bh_.process();
                    break;
                }

                case Preset::AcousticWormhole: {
                    sample = wormhole_.process();
                    break;
                }

                case Preset::TopologicalMetamaterial: {
                    sample = topological_.process();
                    break;
                }

                case Preset::KdvSolitonLattice: {
                    sample = soliton_lattice_.process();
                    break;
                }
            }

            // Apply monophonic envelope to the monophonic synthesizer output
            if (env_.is_active()) {
                const float env_gain = env_.process();
                sample = sample * env_gain;
            } else {
                sample = StereoFrame{0.0f, 0.0f};
            }

            // Mix in active polyphonic chord voices if any
            if (polyphony_.active_voices() > 0) {
                sample = sample + polyphony_.step();
            }

            // Master lowpass filter
            sample.left = filter_l_.process(sample.left);
            sample.right = filter_r_.process(sample.right);

            // Hyperbolic FDN Reverb
            sample = reverb_.process(sample);

            // Monotonic soft-clipping saturation (never inverts phase/sign for loud inputs)
            auto soft_clip = [](Sample x) noexcept -> Sample {
                if (x <= -1.0f) return -1.0f;
                if (x >= 1.0f) return 1.0f;
                return x * (1.5f - 0.5f * x * x);
            };

            frame.left = soft_clip(sample.left);
            frame.right = soft_clip(sample.right);
        }
    }

    [[nodiscard]] engines::HyperbolicWaveguide& waveguide() noexcept { return waveguide_; }
    [[nodiscard]] engines::ChladniPlate& chladni() noexcept { return chladni_; }
    [[nodiscard]] engines::ChaoticOscillator& chaos() noexcept { return chaos_; }
    [[nodiscard]] engines::VonKarmanPlate& von_karman() noexcept { return von_karman_; }
    [[nodiscard]] engines::PseudosphereHorn& pseudosphere() noexcept { return pseudosphere_; }
    [[nodiscard]] engines::HenonHeilesOscillator& henon_heiles() noexcept { return henon_heiles_; }
    [[nodiscard]] engines::HyperbolicString& string() noexcept { return string_; }
    [[nodiscard]] engines::SineGordonBreather& sine_gordon() noexcept { return sine_gordon_; }
    [[nodiscard]] engines::AcousticBlackHole& acoustic_bh() noexcept { return acoustic_bh_; }
    [[nodiscard]] engines::AcousticWormhole& wormhole() noexcept { return wormhole_; }
    [[nodiscard]] engines::TopologicalMetamaterial& topological() noexcept { return topological_; }
    [[nodiscard]] engines::KdvSolitonLattice& soliton_lattice() noexcept { return soliton_lattice_; }
    [[nodiscard]] dsp::fx::HyperbolicSpatializer3D& spatializer3d() noexcept { return spatializer3d_; }
    [[nodiscard]] PolyphonyManager<8>& polyphony() noexcept { return polyphony_; }
    [[nodiscard]] dsp::TuningSystem& tuning() noexcept { return tuning_; }
    [[nodiscard]] dsp::ADSREnvelope& envelope() noexcept { return env_; }
    [[nodiscard]] const dsp::ADSREnvelope& envelope() const noexcept { return env_; }
    [[nodiscard]] dsp::fx::HyperbolicReverb& reverb() noexcept { return reverb_; }
    [[nodiscard]] uint32_t sample_rate() const noexcept { return sample_rate_; }

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    Preset current_preset_{Preset::HyperbolicCosmicBell};

    engines::HyperbolicWaveguide waveguide_;
    engines::ChladniPlate chladni_;
    engines::ChaoticOscillator chaos_;
    engines::StickSlipExciter exciter_;
    engines::VonKarmanPlate von_karman_;
    engines::PseudosphereHorn pseudosphere_;
    engines::HenonHeilesOscillator henon_heiles_;
    engines::HyperbolicString string_;
    engines::SineGordonBreather sine_gordon_;
    engines::AcousticBlackHole acoustic_bh_;
    engines::AcousticWormhole wormhole_;
    engines::TopologicalMetamaterial topological_;
    engines::KdvSolitonLattice soliton_lattice_;
    dsp::fx::HyperbolicSpatializer3D spatializer3d_;
    PolyphonyManager<8> polyphony_;

    dsp::BiquadFilter filter_l_;
    dsp::BiquadFilter filter_r_;
    dsp::ADSREnvelope env_;
    dsp::fx::HyperbolicReverb reverb_;
    dsp::TuningSystem tuning_{dsp::TuningSystem::PredefinedScale::BohlenPierce};

    uint32_t chaos_step_count_{0};
};

} // namespace aether
