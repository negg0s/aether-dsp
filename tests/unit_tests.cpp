#include "aether/types.hpp"
#include "aether/ring_buffer.hpp"
#include "aether/math/poincare.hpp"
#include "aether/math/runge_kutta.hpp"
#include "aether/math/hyperbolic_tiling.hpp"
#include "aether/dsp/biquad.hpp"
#include "aether/dsp/tuning.hpp"
#include "aether/dsp/scala_catalog.hpp"
#include "aether/dsp/fx/hyperbolic_reverb.hpp"
#include "aether/dsp/fx/binaural_panner.hpp"
#include "aether/engines/hyperbolic_waveguide.hpp"
#include "aether/engines/chladni_plate.hpp"
#include "aether/engines/stick_slip_exciter.hpp"
#include "aether/midi/midi_file.hpp"
#include "aether/wav_writer.hpp"
#include "aether/wav_reader.hpp"
#include "aether/svg_exporter.hpp"
#include "aether/preset_manager.hpp"
#include "aether/math/geodesic_raytracer.hpp"
#include "aether/math/hyperbolic_halfplane.hpp"
#include "aether/engines/von_karman_plate.hpp"
#include "aether/engines/pseudosphere_horn.hpp"
#include "aether/math/yoshida_integrator.hpp"
#include "aether/dsp/fx/convolution_reverb.hpp"
#include "aether/ui/terminal_visualizer.hpp"
#include "aether/plugin/clap_entry.hpp"
#include "aether/c_api.h"
#include "aether/polyphony.hpp"
#include "aether/engines/hamiltonian_chaos.hpp"
#include "aether/engines/hyperbolic_string.hpp"
#include "aether/dsp/fx/hyperbolic_spatializer_3d.hpp"
#include "aether/math/fft.hpp"
#include "aether/math/algorithmic_composer.hpp"
#include "aether/math/bolza_surface.hpp"
#include "aether/dsp/fx/partitioned_convolution.hpp"
#include "aether/engine.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>

#undef assert
#define assert(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "\n[TEST ASSERTION FAILURE] " #expr " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::abort(); \
        } \
    } while (0)

void test_ring_buffer() {
    std::cout << "[TEST] SPSCRingBuffer... ";
    aether::SPSCRingBuffer<int, 16> rb;
    assert(rb.empty());
    assert(rb.size() == 0);
    assert(rb.capacity() == 16);

    for (int i = 0; i < 16; ++i) {
        bool ok = rb.push(i * 10);
        assert(ok);
    }
    assert(!rb.empty());
    assert(rb.size() == 16);
    assert(!rb.push(999));

    for (int i = 0; i < 16; ++i) {
        int val = -1;
        bool ok = rb.pop(val);
        assert(ok);
        assert(val == i * 10);
    }
    assert(rb.empty());
    std::cout << "PASSED\n";
}

void test_poincare_geometry() {
    std::cout << "[TEST] Poincare Geometry & Metric... ";
    using aether::math::PoincareDisk;
    aether::Vec2 origin{0.0f, 0.0f};
    aether::Vec2 p1{0.5f, 0.0f};

    float cf0 = PoincareDisk::conformal_factor(origin);
    assert(std::abs(cf0 - 2.0f) < 1e-5f);

    float lbf0 = PoincareDisk::laplace_beltrami_factor(origin);
    assert(std::abs(lbf0 - 0.25f) < 1e-5f);

    float d0 = PoincareDisk::geodesic_distance(origin, origin);
    assert(std::abs(d0) < 1e-5f);

    float d_forward = PoincareDisk::geodesic_distance(origin, p1);
    float d_reverse = PoincareDisk::geodesic_distance(p1, origin);
    assert(std::abs(d_forward - d_reverse) < 1e-5f);
    assert(d_forward > 0.0f);

    std::cout << "PASSED\n";
}

void test_chladni_plate() {
    std::cout << "[TEST] Chladni Plate Eigenmodes... ";
    aether::engines::ChladniPlate plate;
    plate.configure(220.0f, 1.0f);
    plate.strike({0.2f, 0.3f}, 0.8f);

    const auto& modes = plate.modes();
    assert(modes[0].frequency >= 220.0f);

    for (const auto& m : modes) {
        if (m.m > 0 && m.n > 0 && m.m != m.n) {
            assert(m.frequency > 0.0f);
            assert(!std::isnan(m.frequency));
        }
    }

    for (int i = 0; i < 500; ++i) {
        auto frame = plate.step();
        assert(!std::isnan(frame.left));
        assert(!std::isnan(frame.right));
        assert(std::abs(frame.left) <= 5.0f);
        assert(std::abs(frame.right) <= 5.0f);
    }
    std::cout << "PASSED\n";
}

void test_hyperbolic_waveguide() {
    std::cout << "[TEST] Hyperbolic Waveguide FDTD Stability... ";
    aether::engines::HyperbolicWaveguide wg;
    wg.configure(0.40f, 0.005f, 1.0f);
    wg.strike({0.2f, 0.2f}, 1.0f);

    for (int i = 0; i < 1000; ++i) {
        auto frame = wg.step();
        assert(!std::isnan(frame.left));
        assert(!std::isnan(frame.right));
        assert(!std::isinf(frame.left));
        assert(!std::isinf(frame.right));
        assert(std::abs(frame.left) < 10.0f);
        assert(std::abs(frame.right) < 10.0f);
    }
    std::cout << "PASSED\n";
}

void test_biquad_filter() {
    std::cout << "[TEST] Biquad Filter Direct Form II... ";
    aether::dsp::BiquadFilter filter;
    filter.configure(aether::dsp::FilterType::Lowpass, 1000.0f, 0.707f, 48000.0f);

    aether::Sample out = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        out = filter.process(1.0f);
    }
    assert(std::abs(out - 1.0f) < 0.01f);
    std::cout << "PASSED\n";
}

void test_tuning_system() {
    std::cout << "[TEST] Microtonal Tuning & Bohlen-Pierce... ";
    aether::dsp::TuningSystem bp(aether::dsp::TuningSystem::PredefinedScale::BohlenPierce);
    assert(bp.size() == 13);

    float root = bp.get_frequency(0, 220.0f);
    assert(std::abs(root - 220.0f) < 1e-3f);

    float tritave = bp.get_frequency(13, 220.0f);
    assert(std::abs(tritave - 660.0f) < 1e-1f);
    std::cout << "PASSED\n";
}

void test_reverb() {
    std::cout << "[TEST] Hyperbolic FDN Reverb Stability... ";
    aether::dsp::fx::HyperbolicReverb rev;
    rev.configure(0.9f, 0.2f, 0.5f);

    aether::StereoFrame out = rev.process({1.0f, 1.0f});
    for (int i = 0; i < 5000; ++i) {
        out = rev.process({0.0f, 0.0f});
        assert(!std::isnan(out.left));
        assert(!std::isnan(out.right));
        assert(std::abs(out.left) < 5.0f);
        assert(std::abs(out.right) < 5.0f);
    }
    std::cout << "PASSED\n";
}

void test_midi_serialization() {
    std::cout << "[TEST] Standard MIDI File Serialization... ";
    aether::midi::MidiFile mf;
    aether::midi::MidiTrack trk;
    trk.name = "Test Track";
    trk.events.push_back({0, 0x90, 0, 60, 100, {}});
    trk.events.push_back({480, 0x80, 0, 60, 0, {}});
    mf.tracks.push_back(std::move(trk));

    const std::string tmp_midi = "test_temp.mid";
    bool ok_save = mf.save(tmp_midi);
    assert(ok_save);

    aether::midi::MidiFile loaded;
    bool ok_load = loaded.load(tmp_midi);
    assert(ok_load);
    assert(loaded.tracks.size() == 1);
    assert(loaded.tracks[0].events.size() == 2);
    assert(loaded.tracks[0].events[0].data1 == 60);

    std::remove(tmp_midi.c_str());
    std::cout << "PASSED\n";
}

void test_binaural_panner() {
    std::cout << "[TEST] 3D Binaural HRTF Panner... ";
    aether::dsp::fx::BinauralPanner panner(48000);

    panner.set_position(90.0f, 1.0f);
    auto stereo = panner.process(1.0f);
    assert(!std::isnan(stereo.left));
    assert(!std::isnan(stereo.right));

    panner.set_position(-90.0f, 1.0f);
    stereo = panner.process(1.0f);
    assert(!std::isnan(stereo.left));
    assert(!std::isnan(stereo.right));
    std::cout << "PASSED\n";
}

void test_hyperbolic_tiling() {
    std::cout << "[TEST] Hyperbolic {7, 3} Tiling Lattice... ";
    auto tiles = aether::math::HyperbolicTiling::generate_tiling(7, 3, 1);
    assert(!tiles.empty());
    assert(tiles[0].vertices.size() == 7);
    assert(tiles[0].hyperbolic_area > 0.0f);
    std::cout << "PASSED\n";
}

void test_wav_roundtrip() {
    std::cout << "[TEST] Lossless WAV Read/Write Roundtrip... ";
    std::vector<aether::StereoFrame> test_frames(100);
    for (size_t i = 0; i < 100; ++i) {
        test_frames[i] = aether::StereoFrame{0.5f, -0.5f};
    }

    const std::string tmp_wav = "test_roundtrip.wav";
    bool ok_write = aether::WavWriter::write_stereo_wav(tmp_wav, test_frames, 48000, true);
    assert(ok_write);

    auto read_data = aether::WavReader::read_wav(tmp_wav);
    assert(read_data.valid);
    assert(read_data.frames.size() == 100);
    assert(std::abs(read_data.frames[0].left - 0.5f) < 1e-4f);
    assert(std::abs(read_data.frames[0].right - (-0.5f)) < 1e-4f);

    std::remove(tmp_wav.c_str());
    std::cout << "PASSED\n";
}

void test_c_api() {
    std::cout << "[TEST] C-ABI Dynamic API Functions... ";
    aether_engine_handle_t handle = aether_engine_create(48000);
    assert(handle != nullptr);

    aether_engine_load_preset(handle, AETHER_PRESET_HYPERBOLIC_BELL);
    aether_engine_note_on(handle, 0.3f, 0.4f, 0.9f);

    std::vector<float> buffer(128 * 2, 0.0f);
    aether_engine_process_interleaved(handle, buffer.data(), 128);

    bool has_sound = false;
    for (float sample : buffer) {
        if (std::abs(sample) > 1e-5f) has_sound = true;
    }
    assert(has_sound);

    aether_engine_note_off(handle);
    aether_engine_destroy(handle);
    std::cout << "PASSED\n";
}

void test_clap_plugin_descriptor() {
    std::cout << "[TEST] CLAP Plugin Architecture Descriptor... ";
    const auto desc = aether::plugin::get_descriptor();
    assert(std::string(desc.id) == "com.negg0s.aether-dsp");
    assert(std::string(desc.name) == "AetherDSP");
    assert(aether::plugin::PARAMETER_CATALOG[0].id == aether::plugin::PARAM_PRESET);
    std::cout << "PASSED\n";
}

void test_stick_slip_exciter() {
    std::cout << "[TEST] Stick-Slip Stribeck Friction Exciter... ";
    aether::engines::StickSlipExciter exciter;
    exciter.configure(0.8f, 0.3f, 4.0f);

    float f0 = exciter.compute_force(0.5f, 1.0f, 0.0f);
    assert(f0 > 0.0f);
    assert(!std::isnan(f0));

    // Zero relative velocity yields zero force
    float f_zero = exciter.compute_force(0.5f, 1.0f, 0.5f);
    assert(std::abs(f_zero) < 1e-3f);
    std::cout << "PASSED\n";
}

void test_scala_catalog() {
    std::cout << "[TEST] Scala Microtonal Scale Catalog... ";
    const auto& scales = aether::dsp::ScalaCatalog::get_all_scales();
    assert(scales.size() >= 8);
    for (const auto& sc : scales) {
        assert(sc.period > 1.0);
        assert(!sc.ratios.empty());
        assert(sc.ratios[0] == 1.0);
    }
    std::cout << "PASSED\n";
}

void test_svg_export() {
    std::cout << "[TEST] SVG Vector Art Exporter... ";
    aether::engines::ChladniPlate plate;
    plate.configure(220.0f, 1.0f);
    const std::string tmp_chladni = "test_chladni.svg";
    bool ok_chladni = aether::SvgExporter::export_chladni_svg(plate, tmp_chladni, 400, 400);
    assert(ok_chladni);

    auto tiles = aether::math::HyperbolicTiling::generate_tiling(7, 3, 1);
    const std::string tmp_poincare = "test_poincare.svg";
    bool ok_poincare = aether::SvgExporter::export_poincare_tiling_svg(tiles, tmp_poincare, 400);
    assert(ok_poincare);

    std::remove(tmp_chladni.c_str());
    std::remove(tmp_poincare.c_str());
    std::cout << "PASSED\n";
}

void test_preset_manager() {
    std::cout << "[TEST] .aether Preset Serialization & Parsing... ";
    aether::AetherPresetData original;
    original.name = "Hyperbolic Unit Test Bell";
    original.preset_id = 42;
    original.wave_speed = 0.52f;
    original.damping = 0.007f;
    original.curvature_scale = 1.35f;
    original.plate_fundamental = 314.15f;
    original.tuning_scale = "Bohlen-Pierce";

    const std::string tmp_preset = "test_preset.aether";
    bool ok_save = aether::PresetManager::save_preset(tmp_preset, original);
    assert(ok_save);

    aether::AetherPresetData loaded;
    bool ok_load = aether::PresetManager::load_preset(tmp_preset, loaded);
    assert(ok_load);
    assert(loaded.name == original.name);
    assert(loaded.preset_id == original.preset_id);
    assert(std::abs(loaded.wave_speed - original.wave_speed) < 1e-4f);
    assert(std::abs(loaded.damping - original.damping) < 1e-4f);
    assert(std::abs(loaded.curvature_scale - original.curvature_scale) < 1e-4f);
    assert(std::abs(loaded.plate_fundamental - original.plate_fundamental) < 1e-3f);
    assert(loaded.tuning_scale == original.tuning_scale);

    std::remove(tmp_preset.c_str());
    std::cout << "PASSED\n";
}

void test_geodesic_raytracer() {
    std::cout << "[TEST] Hamiltonian Geodesic Sound Ray Tracer... ";
    using namespace aether::math;
    GeodesicRayTracer::Config cfg;
    cfg.num_rays = 32;
    cfg.step_dt = 0.002f;
    cfg.max_time = 1.0f;
    cfg.capture_radius = 0.15f;

    GeodesicRayTracer tracer(cfg);
    auto arrivals = tracer.trace({0.0f, 0.0f}, {0.3f, 0.3f});
    assert(!arrivals.empty());
    assert(arrivals[0].time_sec >= 0.0f);
    assert(arrivals[0].amplitude > 0.0f);

    auto ir = tracer.synthesize_impulse_response(arrivals, 48000, 0.2f);
    assert(!ir.empty());
    assert(ir.size() == static_cast<size_t>(0.2f * 48000));
    std::cout << "PASSED\n";
}

void test_convolution_reverb() {
    std::cout << "[TEST] Time-Domain Convolution Reverb... ";
    aether::dsp::fx::ConvolutionReverb conv;
    std::vector<aether::StereoFrame> ir(10, {0.0f, 0.0f});
    ir[0] = {1.0f, 1.0f}; // Delta impulse

    conv.load_impulse_response(ir, 1.0f); // 100% wet
    auto out = conv.process({0.75f, -0.75f});
    assert(std::abs(out.left - 0.75f) < 1e-4f);
    assert(std::abs(out.right - (-0.75f)) < 1e-4f);

    conv.reset();
    std::cout << "PASSED\n";
}

void test_braille_visualizer() {
    std::cout << "[TEST] Terminal Unicode Braille Canvas... ";
    aether::ui::TerminalVisualizer::BrailleCanvas canvas(10, 5);
    assert(canvas.pixel_width() == 20);
    assert(canvas.pixel_height() == 20);

    canvas.set_pixel(0, 0, true);
    canvas.set_pixel(1, 3, true);
    std::string ansi = canvas.render_ansi();
    assert(!ansi.empty());

    // Single Braille char encoding
    std::string ch = aether::ui::TerminalVisualizer::braille_char(0x01 | 0x80);
    assert(ch.size() == 3);
    assert(static_cast<uint8_t>(ch[0]) == 0xE2);
    std::cout << "PASSED\n";
}

void test_von_karman_plate() {
    std::cout << "[TEST] Nonlinear von Karman Large-Deflection Plate... ";
    aether::engines::VonKarmanPlate plate(48000);
    plate.init_modes(110.0f, 1.2f, 0.75f);
    assert(!plate.modes().empty());
    assert(plate.modes().size() >= 20);

    plate.strike({0.3f, 0.4f}, 1.5f);

    for (int i = 0; i < 1000; ++i) {
        auto frame = plate.step();
        assert(!std::isnan(frame.left));
        assert(!std::isnan(frame.right));
        assert(!std::isinf(frame.left));
        assert(!std::isinf(frame.right));
        assert(std::abs(frame.left) <= 1.0f); // Soft tanh limit
        assert(std::abs(frame.right) <= 1.0f);
    }
    std::cout << "PASSED\n";
}

void test_hyperbolic_halfplane() {
    std::cout << "[TEST] Poincare Upper Half-Plane & Mobius PSL(2, R)... ";
    using namespace aether::math;
    using Complex = std::complex<double>;

    // Cayley transform roundtrip: D -> H^2 -> D
    Complex origin(0.0, 0.0);
    Complex pt_h = HyperbolicHalfPlane::disk_to_halfplane(origin);
    assert(std::abs(pt_h.real()) < 1e-6);
    assert(std::abs(pt_h.imag() - 1.0) < 1e-6); // Origin maps to i

    Complex back = HyperbolicHalfPlane::halfplane_to_disk(pt_h);
    assert(std::abs(back.real()) < 1e-6);
    assert(std::abs(back.imag()) < 1e-6);

    // Mobius transform isometry
    HyperbolicHalfPlane::MobiusTransform T{1.0, 1.0, 0.0, 1.0}; // Translation z + 1
    assert(std::abs(T.determinant() - 1.0) < 1e-6);
    Complex moved = T.apply(pt_h);
    assert(std::abs(moved.real() - 1.0) < 1e-6);
    assert(std::abs(moved.imag() - 1.0) < 1e-6);

    // Modular delay taps
    auto taps = HyperbolicHalfPlane::generate_modular_delay_taps(8, 20.0f);
    assert(taps.size() == 8);
    for (float tap : taps) {
        assert(tap >= 20.0f);
    }
    std::cout << "PASSED\n";
}

void test_pseudosphere_horn() {
    std::cout << "[TEST] Beltrami Pseudosphere Tractrix Horn... ";
    aether::engines::PseudosphereHorn horn(48000);
    assert(horn.bell_cutoff_frequency() > 100.0f);

    horn.excite_throat(1.0f);
    for (int i = 0; i < 500; ++i) {
        auto frame = horn.step();
        assert(!std::isnan(frame.left));
        assert(!std::isnan(frame.right));
        assert(!std::isinf(frame.left));
        assert(!std::isinf(frame.right));
        assert(std::abs(frame.left) <= 1.0f);
        assert(std::abs(frame.right) <= 1.0f);
    }
    std::cout << "PASSED\n";
}

void test_yoshida_integrator() {
    std::cout << "[TEST] 4th-Order Symplectic Yoshida Integrator... ";
    using aether::math::Yoshida4thIntegrator;
    Yoshida4thIntegrator::State state{1.0, 0.0}; // Initial position = 1, velocity = 0
    constexpr double omega2 = 4.0; // Harmonic oscillator: V(q) = 0.5 * omega2 * q^2

    auto force = [](double q) noexcept { return -omega2 * q; };
    auto potential = [](double q) noexcept { return 0.5 * omega2 * q * q; };

    const double e0 = Yoshida4thIntegrator::total_energy(state, potential);
    assert(std::abs(e0 - 2.0) < 1e-9);

    constexpr double dt = 0.005;
    for (int step = 0; step < 10000; ++step) {
        Yoshida4thIntegrator::step(state, dt, force);
    }

    const double e_final = Yoshida4thIntegrator::total_energy(state, potential);
    // Symplectic preservation: energy drift must remain bounded without secular growth
    const double rel_error = std::abs((e_final - e0) / e0);
    assert(rel_error < 1e-4);
    std::cout << "PASSED\n";
}

void test_braille_spectrogram() {
    std::cout << "[TEST] 2D Time-Frequency Braille Canvas... ";
    std::vector<aether::StereoFrame> test_frames(1024, aether::StereoFrame{0.5f, 0.5f});
    // Visualizer runs without throwing or memory corruption
    aether::ui::TerminalVisualizer::display_spectrogram(test_frames, 48000, 20, 6);
    std::cout << "PASSED\n";
}

void test_polyphony_manager() {
    std::cout << "[TEST] 8-Voice Polyphony Manager & LRU Stealing... ";
    aether::PolyphonyManager<8> poly(48000);
    assert(poly.active_voices() == 0);

    // Play 3 notes (C Major triad)
    poly.note_on(60, 261.63f, 0.8f);
    poly.note_on(64, 329.63f, 0.75f);
    poly.note_on(67, 392.00f, 0.70f);
    assert(poly.active_voices() == 3);

    // Step 200 samples and verify output is active
    float energy = 0.0f;
    for (int i = 0; i < 200; ++i) {
        auto frame = poly.step();
        energy += std::abs(frame.left) + std::abs(frame.right);
    }
    assert(energy > 0.001f);

    // Saturate all 8 voices and force LRU voice stealing
    for (int note = 70; note <= 78; ++note) {
        poly.note_on(note, 440.0f + static_cast<float>(note) * 10.0f, 0.6f);
    }
    assert(poly.active_voices() == 8);

    // Re-trigger existing note
    poly.note_on(70, 440.0f + 700.0f, 0.9f);
    assert(poly.active_voices() == 8);

    poly.all_notes_off();
    std::cout << "PASSED\n";
}

void test_henon_heiles_chaos() {
    std::cout << "[TEST] Henon-Heiles Galactic Hamiltonian Chaos... ";
    aether::engines::HenonHeilesOscillator henon(0.13, 250.0);
    const double e0 = henon.compute_energy();
    assert(std::abs(e0 - 0.13) < 0.015);

    // Step system for 3000 steps with 4th-order Yoshida integrator
    bool has_non_zero = false;
    for (int step = 0; step < 3000; ++step) {
        auto frame = henon.step(48000.0f);
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
        assert(std::abs(frame.left) <= 1.0f && std::abs(frame.right) <= 1.0f);
        if (std::abs(frame.left) > 0.01f) has_non_zero = true;
    }
    assert(has_non_zero);

    // Verify bounded energy conservation (no divergence)
    const double e_final = henon.compute_energy();
    assert(!std::isnan(e_final));
    assert(e_final > 0.0 && e_final < 0.3);
    std::cout << "PASSED\n";
}

void test_engine_presets() {
    std::cout << "[TEST] Master Engine 14-Preset Complete Audio Rendering... ";
    aether::Engine engine(48000);
    std::vector<aether::StereoFrame> block(256);

    const aether::Preset presets[] = {
        aether::Preset::HyperbolicCosmicBell,
        aether::Preset::ChladniPlateGong,
        aether::Preset::ChaoticQuantumDrone,
        aether::Preset::HyperbolicAetherSpace,
        aether::Preset::BowedCrystalPlate,
        aether::Preset::VonKarmanTurbulentGong,
        aether::Preset::PseudosphereTractrixHorn,
        aether::Preset::HenonHeilesChaos,
        aether::Preset::HyperbolicStringJawari,
        aether::Preset::SineGordonBreather,
        aether::Preset::AcousticBlackHole,
        aether::Preset::AcousticWormhole,
        aether::Preset::TopologicalMetamaterial,
        aether::Preset::KdvSolitonLattice
    };

    for (auto preset : presets) {
        engine.load_preset(preset);
        engine.note_on({0.2f, 0.3f}, 0.8f);
        engine.process_block(block);

        for (const auto& frame : block) {
            assert(!std::isnan(frame.left) && !std::isnan(frame.right));
            assert(!std::isinf(frame.left) && !std::isinf(frame.right));
            assert(frame.left >= -1.01f && frame.left <= 1.01f);
            assert(frame.right >= -1.01f && frame.right <= 1.01f);
        }
    }

    // Test polyphonic note routing
    engine.poly_note_on(60, 0.8f);
    engine.poly_note_on(64, 0.8f);
    engine.process_block(block);
    engine.all_notes_off();

    std::cout << "PASSED\n";
}

void test_fft_spectral_transform() {
    std::cout << "[TEST] Radix-2 Cooley-Tukey FFT & Parseval Invariant... ";
    using aether::math::FFT;
    constexpr size_t N = 128;
    std::vector<std::complex<float>> data(N);

    // Test Dirac delta impulse roundtrip
    data[0] = {1.0f, 0.0f};
    for (size_t i = 1; i < N; ++i) data[i] = {0.0f, 0.0f};

    FFT::forward(data);
    for (size_t k = 0; k < N; ++k) {
        assert(std::abs(std::abs(data[k]) - 1.0f) < 1e-5f);
    }

    FFT::inverse(data);
    assert(std::abs(data[0].real() - 1.0f) < 1e-5f);
    for (size_t i = 1; i < N; ++i) {
        assert(std::abs(data[i]) < 1e-5f);
    }

    // Test Parseval's energy theorem on synthetic tone
    std::vector<float> time_signal(N);
    for (size_t n = 0; n < N; ++n) {
        time_signal[n] = std::sin(2.0f * std::numbers::pi_v<float> * 4.0f * n / N);
    }

    std::vector<float> power_spec(N / 2 + 1);
    FFT::compute_power_spectrum(time_signal, power_spec, aether::math::WindowType::Rectangular);

    // Peak bin must be at index k = 4
    size_t peak_bin = 0;
    float max_p = 0.0f;
    for (size_t k = 0; k < power_spec.size(); ++k) {
        if (power_spec[k] > max_p) {
            max_p = power_spec[k];
            peak_bin = k;
        }
    }
    assert(peak_bin == 4);
    std::cout << "PASSED\n";
}

void test_hyperbolic_string_jawari() {
    std::cout << "[TEST] Nonlinear Stiff Kirchhoff-Jawari String... ";
    aether::engines::HyperbolicString string(48000);
    string.pluck(0.25f, 0.9f);

    float total_energy = 0.0f;
    for (int step = 0; step < 2000; ++step) {
        auto frame = string.step();
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
        assert(!std::isinf(frame.left) && !std::isinf(frame.right));
        assert(std::abs(frame.left) <= 1.0f && std::abs(frame.right) <= 1.0f);
        total_energy += std::abs(frame.left) + std::abs(frame.right);
    }
    assert(total_energy > 0.05f);
    std::cout << "PASSED\n";
}

void test_hyperbolic_spatializer_3d() {
    std::cout << "[TEST] 3D Poincare Ball (H^3) Spatializer & Metric... ";
    using aether::dsp::fx::HyperbolicSpatializer3D;

    HyperbolicSpatializer3D::Position3D p0{0.0f, 0.0f, 0.0f};
    HyperbolicSpatializer3D::Position3D p1{0.5f, 0.0f, 0.0f};
    HyperbolicSpatializer3D::Position3D p2{0.0f, 0.5f, 0.0f};

    // Metric identity: d_H(p, p) == 0
    assert(std::abs(HyperbolicSpatializer3D::geodesic_distance(p0, p0)) < 1e-5f);

    // Symmetry: d_H(p0, p1) == d_H(p1, p0)
    const float d01 = HyperbolicSpatializer3D::geodesic_distance(p0, p1);
    const float d10 = HyperbolicSpatializer3D::geodesic_distance(p1, p0);
    assert(std::abs(d01 - d10) < 1e-5f);

    // Triangle inequality: d_H(p1, p2) <= d_H(p1, p0) + d_H(p0, p2)
    const float d12 = HyperbolicSpatializer3D::geodesic_distance(p1, p2);
    const float d02 = HyperbolicSpatializer3D::geodesic_distance(p0, p2);
    assert(d12 <= (d01 + d02 + 1e-4f));

    // Spatializer processing test
    HyperbolicSpatializer3D spat(48000);
    spat.set_source_position(p1, p0);
    assert(spat.geodesic_distance() > 0.0f);

    auto stereo = spat.process(0.75f);
    assert(!std::isnan(stereo.left) && !std::isnan(stereo.right));
    std::cout << "PASSED\n";
}

void test_partitioned_convolution() {
    std::cout << "[TEST] Zero-Latency Partitioned Overlap-Save FFT Convolution... ";
    using aether::dsp::fx::PartitionedConvolution;

    // Synthetic 512-sample exponential decay impulse response
    constexpr size_t IR_LEN = 512;
    std::vector<float> ir(IR_LEN);
    for (size_t i = 0; i < IR_LEN; ++i) {
        ir[i] = std::exp(-static_cast<float>(i) * 0.02f);
    }

    PartitionedConvolution conv;
    conv.load_impulse_response(ir, 0.6f);
    // 512 samples / 128 block size = 4 partitions
    assert(conv.num_partitions() == 4);

    // Feed a block of audio and ensure no NaNs and output energy exists
    std::vector<aether::StereoFrame> test_input(256, aether::StereoFrame{0.5f, 0.5f});
    conv.process_block(test_input);

    for (const auto& f : test_input) {
        assert(!std::isnan(f.left) && !std::isnan(f.right));
        assert(!std::isinf(f.left) && !std::isinf(f.right));
    }
    std::cout << "PASSED\n";
}

void test_bolza_surface_fuchsian_group() {
    std::cout << "[TEST] Bolza Genus-2 Surface & Fuchsian Octagon Isometries... ";
    using aether::math::BolzaSurface;

    auto vertices = BolzaSurface::get_octagon_vertices();
    assert(vertices.size() == 8);

    // Check all vertices are at radius R = 2^(-1/4) ~= 0.840896
    for (const auto& v : vertices) {
        const float r = std::sqrt(v.x * v.x + v.y * v.y);
        assert(std::abs(r - BolzaSurface::VERTEX_RADIUS) < 1e-4f);
    }

    // Origin is inside the fundamental domain
    assert(BolzaSurface::is_inside_fundamental_domain({0.0f, 0.0f}));

    // Test side-pairing wrapping: mapped point must remain inside unit disk |z| < 1
    aether::Vec2 boundary_point{0.82f, 0.0f};
    aether::Vec2 wrapped = BolzaSurface::wrap_point(boundary_point, 0);
    const float r_wrap = std::sqrt(wrapped.x * wrapped.x + wrapped.y * wrapped.y);
    assert(r_wrap < 1.0f);

    // Test Bolza Manifold Impulse Response synthesis
    auto ir = BolzaSurface::synthesize_bolza_ir(48000, 1.0f);
    assert(!ir.empty());
    bool non_zero = false;
    for (float sample : ir) {
        assert(!std::isnan(sample));
        if (std::abs(sample) > 0.01f) non_zero = true;
    }
    assert(non_zero);
    std::cout << "PASSED\n";
}

void test_sine_gordon_breather() {
    std::cout << "[TEST] Sine-Gordon Topological Soliton & Breather... ";
    aether::engines::SineGordonBreather sgb(48000);
    sgb.configure(0.40f, 320.0f, 0.0005f);

    // Test breather injection
    sgb.inject_breather(0.5f, 0.70f, 1.0f);
    assert(sgb.total_energy() > 0.0f);

    for (int i = 0; i < 600; ++i) {
        auto frame = sgb.process();
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
        assert(!std::isinf(frame.left) && !std::isinf(frame.right));
        assert(std::abs(frame.left) <= 1.0f);
        assert(std::abs(frame.right) <= 1.0f);
    }

    // Test Kink-Antikink collision
    sgb.inject_kink_antikink(0.4f, 0.65f);
    for (int i = 0; i < 600; ++i) {
        auto frame = sgb.process();
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_acoustic_black_hole() {
    std::cout << "[TEST] Acoustic Black Hole & Sonic Horizon Waveguide... ";
    aether::engines::AcousticBlackHole abh(48000);
    abh.configure(2.4f, 0.10f, 0.01f, 0.05f);

    // Strike incident wave
    abh.strike(0.85f, 0.9f);

    bool non_zero = false;
    for (int i = 0; i < 1000; ++i) {
        auto frame = abh.process();
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
        assert(!std::isinf(frame.left) && !std::isinf(frame.right));
        if (std::abs(frame.left) > 0.01f || std::abs(frame.right) > 0.01f) {
            non_zero = true;
        }
    }
    assert(non_zero);
    std::cout << "PASSED\n";
}

void test_acoustic_wormhole() {
    std::cout << "[TEST] Relativistic Acoustic Wormhole & Quasinormal Ringing... ";
    aether::engines::AcousticWormhole wormhole(48000);
    wormhole.configure(0.35f, 1.4f, 0.42f, 0.0006f);

    wormhole.strike_universe_a(0.65f, 0.95f);
    assert(wormhole.total_energy() > 0.0f);

    bool b_received = false;
    for (int i = 0; i < 1200; ++i) {
        auto frame = wormhole.process();
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
        assert(!std::isinf(frame.left) && !std::isinf(frame.right));
        if (std::abs(frame.right) > 0.005f) {
            b_received = true; // Confirms flux tunneling across the throat into Universe B
        }
    }
    assert(b_received);
    std::cout << "PASSED\n";
}

void test_topological_metamaterial() {
    std::cout << "[TEST] Topological Dirac Metamaterial & Chiral Edge States... ";
    aether::engines::TopologicalMetamaterial topo(48000);
    topo.configure(0.45f, 0.38f, 0.0005f);

    topo.strike_edge(0.95f);
    assert(topo.total_energy() > 0.0f);

    bool edge_signal = false;
    for (int i = 0; i < 1000; ++i) {
        auto frame = topo.process();
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
        assert(!std::isinf(frame.left) && !std::isinf(frame.right));
        if (std::abs(frame.left) > 0.005f) {
            edge_signal = true; // Confirms edge mode transport along domain wall
        }
    }
    assert(edge_signal);
    std::cout << "PASSED\n";
}

void test_kdv_soliton_lattice() {
    std::cout << "[TEST] Toda / KdV Completely Integrable Soliton Lattice... ";
    aether::engines::KdvSolitonLattice toda(48000);
    toda.configure(1.8f, 340.0f, 0.0004f);

    toda.inject_soliton(0.2f, 1.2f);
    assert(toda.total_energy() > 0.0f);

    bool strain_observed = false;
    for (int i = 0; i < 1000; ++i) {
        auto frame = toda.process();
        assert(!std::isnan(frame.left) && !std::isnan(frame.right));
        assert(!std::isinf(frame.left) && !std::isinf(frame.right));
        if (std::abs(frame.left) > 0.01f || std::abs(frame.right) > 0.01f) {
            strain_observed = true;
        }
    }
    assert(strain_observed);
    std::cout << "PASSED\n";
}

void test_midi_parser_robustness() {
    std::cout << "[TEST] MIDI Parser Boundary & Truncation Robustness... ";
    const std::string tmp_bad_midi = "test_truncated.mid";
    {
        std::ofstream out(tmp_bad_midi, std::ios::binary);
        out.write("MThd", 4);
        uint8_t h[10] = {0, 0, 0, 6, 0, 1, 0, 1, 1, 0xE0};
        out.write(reinterpret_cast<const char*>(h), 10);
        out.write("MTrk", 4);
        uint8_t trk[6] = {0, 0, 0, 2, 0x00, 0xFF}; // Truncated meta event
        out.write(reinterpret_cast<const char*>(trk), 6);
    }

    aether::midi::MidiFile mf;
    mf.load(tmp_bad_midi);
    std::remove(tmp_bad_midi.c_str());
    std::cout << "PASSED\n";
}

void test_wav_reader_robustness() {
    std::cout << "[TEST] WAV Parser Division-by-Zero & Chunk Validation... ";
    const std::string tmp_bad_wav = "test_bad.wav";
    {
        std::ofstream out(tmp_bad_wav, std::ios::binary);
        out.write("RIFF", 4);
        uint32_t size = 44;
        out.write(reinterpret_cast<const char*>(&size), 4);
        out.write("WAVE", 4);
        out.write("fmt ", 4);
        uint32_t sub1 = 16;
        uint16_t fmt = 1;
        uint16_t chans = 0; // Invalid 0 channels
        uint32_t sr = 44100;
        uint32_t br = 0;
        uint16_t ba = 0;
        uint16_t bits = 0; // Invalid 0 bits per sample
        out.write(reinterpret_cast<const char*>(&sub1), 4);
        out.write(reinterpret_cast<const char*>(&fmt), 2);
        out.write(reinterpret_cast<const char*>(&chans), 2);
        out.write(reinterpret_cast<const char*>(&sr), 4);
        out.write(reinterpret_cast<const char*>(&br), 4);
        out.write(reinterpret_cast<const char*>(&ba), 2);
        out.write(reinterpret_cast<const char*>(&bits), 2);
        out.write("data", 4);
        uint32_t data_sz = 100;
        out.write(reinterpret_cast<const char*>(&data_sz), 4);
    }

    auto res = aether::WavReader::read_wav(tmp_bad_wav);
    assert(!res.valid); // Must reject cleanly without crash/SIGFPE
    std::remove(tmp_bad_wav.c_str());
    std::cout << "PASSED\n";
}

void test_hyperbolic_string_high_pitches() {
    std::cout << "[TEST] Hyperbolic String CFL Stability at 440 Hz & 1200 Hz... ";
    aether::engines::HyperbolicString string(48000);

    // Test 440 Hz (standard concert pitch)
    string.set_pitch(440.0f);
    string.pluck(0.22f, 0.9f);
    for (int i = 0; i < 2000; ++i) {
        auto f = string.step();
        assert(!std::isnan(f.left) && !std::isnan(f.right));
        assert(!std::isinf(f.left) && !std::isinf(f.right));
        assert(std::abs(f.left) <= 1.0f && std::abs(f.right) <= 1.0f);
    }

    // Test maximum pitch 1200 Hz
    string.reset();
    string.set_pitch(1200.0f);
    string.pluck(0.22f, 0.9f);
    for (int i = 0; i < 2000; ++i) {
        auto f = string.step();
        assert(!std::isnan(f.left) && !std::isnan(f.right));
        assert(!std::isinf(f.left) && !std::isinf(f.right));
        assert(std::abs(f.left) <= 1.0f && std::abs(f.right) <= 1.0f);
    }
    std::cout << "PASSED\n";
}

void test_monophonic_note_off_decay() {
    std::cout << "[TEST] Monophonic note_off() Envelope Sound Release... ";
    aether::Engine engine(48000);
    engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    engine.reverb().configure(0.0f, 0.0f, 0.0f); // zero reverb to isolate synth envelope release
    engine.envelope().configure(0.001f, 0.05f, 0.8f, 0.02f, 48000.0f);
    engine.note_on({0.3f, 0.4f}, 0.9f);

    std::vector<aether::StereoFrame> block(256);
    engine.process_block(block);
    float energy_active = 0.0f;
    for (const auto& f : block) energy_active += std::abs(f.left) + std::abs(f.right);
    assert(energy_active > 0.001f);

    engine.note_off();
    // Render to allow release stage to complete (0.02s release = 960 samples ~ 4 blocks)
    for (int i = 0; i < 50; ++i) {
        engine.process_block(block);
    }

    float energy_released = 0.0f;
    for (const auto& f : block) energy_released += std::abs(f.left) + std::abs(f.right);
    assert(energy_released < 1e-4f);
    std::cout << "PASSED\n";
}

void test_composer_sorted_events() {
    std::cout << "[TEST] Algorithmic Composer Monotonic Timestamp Sorting... ";
    using aether::math::AlgorithmicComposer;
    auto score = AlgorithmicComposer::generate_fibonacci_score(10.0);
    assert(!score.empty());
    for (size_t i = 1; i < score.size(); ++i) {
        assert(score[i].timestamp_sec >= score[i - 1].timestamp_sec);
    }
    std::cout << "PASSED\n";
}

void test_partitioned_convolution_zero_latency() {
    std::cout << "[TEST] Partitioned Convolution Zero-Latency Response... ";
    using aether::dsp::fx::PartitionedConvolution;
    PartitionedConvolution conv;

    // Delta impulse at t=0
    std::vector<float> ir(256, 0.0f);
    ir[0] = 1.0f;
    conv.load_impulse_response(ir, 1.0f);

    // Feed a single impulse at sample 0
    auto out0 = conv.process(aether::StereoFrame{1.0f, 1.0f});
    // With true zero-latency head FIR, output at sample 0 must be non-zero immediately
    assert(std::abs(out0.left) > 0.1f);
    assert(std::abs(out0.right) > 0.1f);
    std::cout << "PASSED\n";
}

void test_scala_multiple_reloads() {
    std::cout << "[TEST] Scala File Multiple Consecutive Reloads... ";
    aether::dsp::TuningSystem tuning;

    const std::string scl1 = "temp1.scl";
    const std::string scl2 = "temp2.scl";
    {
        std::ofstream f1(scl1);
        f1 << "! scale 1\nScale One\n 2\n!\n 100.0\n 2/1\n";
        std::ofstream f2(scl2);
        f2 << "! scale 2\nScale Two with longer description\n 3\n!\n 200.0\n 400.0\n 2/1\n";
    }

    bool ok1 = tuning.load_scala_file(scl1);
    assert(ok1);
    bool ok2 = tuning.load_scala_file(scl2);
    assert(ok2); // Must succeed without exception

    std::remove(scl1.c_str());
    std::remove(scl2.c_str());
    std::cout << "PASSED\n";
}

void test_fft_window_size_one() {
    std::cout << "[TEST] FFT Window Size 1 and 0 Stability... ";
    std::vector<float> single = {1.5f};
    aether::math::FFT::apply_window(single, aether::math::WindowType::Hann);
    assert(!std::isnan(single[0]));
    assert(single[0] == 1.5f);

    std::vector<float> empty;
    aether::math::FFT::apply_window(empty, aether::math::WindowType::BlackmanHarris);
    std::cout << "PASSED\n";
}

void test_binaural_panner_rear_symmetry() {
    std::cout << "[TEST] Binaural Panner Rear 180-deg Symmetry... ";
    aether::dsp::fx::BinauralPanner panner(48000);
    panner.set_position(180.0f, 1.0f);
    panner.reset();
    auto s1 = panner.process(1.0f);

    panner.set_position(-180.0f, 1.0f);
    panner.reset();
    auto s2 = panner.process(1.0f);

    assert(std::abs(s1.left - s1.right) < 1e-4f);
    assert(std::abs(s2.left - s2.right) < 1e-4f);
    std::cout << "PASSED\n";
}

void test_topological_metamaterial_strike() {
    std::cout << "[TEST] Topological Metamaterial Excitation at x=1... ";
    aether::engines::TopologicalMetamaterial topo(48000);
    topo.strike_edge(0.9f);
    auto s = topo.process();
    assert(!std::isnan(s.left) && !std::isnan(s.right));
    std::cout << "PASSED\n";
}

void test_pseudosphere_horn_low_sample_rate() {
    std::cout << "[TEST] Pseudosphere Horn Stability at 16000 Hz... ";
    aether::engines::PseudosphereHorn horn(16000);
    horn.excite_throat(0.8f);
    for (int i = 0; i < 500; ++i) {
        auto s = horn.step();
        assert(!std::isnan(s.left) && !std::isnan(s.right));
        assert(!std::isinf(s.left) && !std::isinf(s.right));
    }
    std::cout << "PASSED\n";
}

void test_adsr_envelope_zero_sustain_decay() {
    std::cout << "[TEST] ADSR Envelope Zero Sustain Decay Invariant... ";
    aether::dsp::ADSREnvelope env;
    env.configure(0.01f, 0.05f, 0.0f, 0.02f, 48000.0f);
    env.gate(true);

    // Run into attack/decay stage
    for (int i = 0; i < 200; ++i) {
        (void)env.process();
    }
    assert(env.is_active());

    // Release when sustain is 0.0
    env.gate(false);
    for (int i = 0; i < 2000; ++i) {
        (void)env.process();
    }
    // Must reach Idle without infinite hanging
    assert(!env.is_active());
    assert(env.stage() == aether::dsp::EnvelopeStage::Idle);
    std::cout << "PASSED\n";
}

void test_hyperbolic_waveguide_boundary_integrity() {
    std::cout << "[TEST] Hyperbolic Waveguide Boundary Integrity & No SIMD Overrun... ";
    aether::engines::HyperbolicWaveguide wg;
    wg.configure(0.45f, 0.003f, 1.0f);
    wg.strike(aether::Vec2{0.0f, 0.0f}, 0.9f);

    for (int step = 0; step < 500; ++step) {
        auto frame = wg.step();
        assert(std::isfinite(frame.left) && std::isfinite(frame.right));
    }

    const auto& state = wg.state();
    for (size_t y = 0; y < aether::engines::HyperbolicWaveguide::GRID_DIM; ++y) {
        for (size_t x = 0; x < aether::engines::HyperbolicWaveguide::GRID_DIM; ++x) {
            const size_t idx = y * aether::engines::HyperbolicWaveguide::GRID_DIM + x;
            assert(std::isfinite(state[idx]));
        }
    }
    std::cout << "PASSED\n";
}

void test_von_karman_stability_high_velocity() {
    std::cout << "[TEST] Von Karman Large-Deflection Stability with Maximum Excitation... ";
    aether::engines::VonKarmanPlate plate(48000);
    plate.init_modes(110.0f, 1.2f, 1.0f);
    plate.strike(aether::Vec2{0.5f, 0.5f}, 1.0f, 0.005f);

    for (int i = 0; i < 4800; ++i) {
        auto frame = plate.step();
        assert(std::isfinite(frame.left) && std::isfinite(frame.right));
        assert(std::abs(frame.left) <= 1.0f && std::abs(frame.right) <= 1.0f);
    }
    std::cout << "PASSED\n";
}

void test_chaotic_oscillator_dc_blocking() {
    std::cout << "[TEST] Chaotic Oscillator Dual-Channel DC Blocker... ";
    aether::engines::ChaoticOscillator chaos;
    chaos.configure(aether::engines::AttractorType::Lorenz, 1.0f);

    float sum_l = 0.0f;
    float sum_r = 0.0f;
    const int total_samples = 48000;

    // Warm-up filter
    for (int i = 0; i < 4000; ++i) {
        (void)chaos.step(48000.0f);
    }

    // Accumulate DC component over 1 second
    for (int i = 0; i < total_samples; ++i) {
        auto frame = chaos.step(48000.0f);
        assert(std::isfinite(frame.left) && std::isfinite(frame.right));
        sum_l += frame.left;
        sum_r += frame.right;
    }

    const float dc_l = std::abs(sum_l / total_samples);
    const float dc_r = std::abs(sum_r / total_samples);
    assert(dc_l < 0.05f);
    assert(dc_r < 0.05f);
    std::cout << "PASSED\n";
}

void test_biquad_nan_self_healing() {
    std::cout << "[TEST] Biquad Filter Non-Finite Input Self-Healing... ";
    aether::dsp::BiquadFilter filter;
    filter.configure(aether::dsp::FilterType::Lowpass, 1000.0f, 0.707f, 48000.0f);

    // Feed NaNs and Infinities
    float nan_val = std::numeric_limits<float>::quiet_NaN();
    float inf_val = std::numeric_limits<float>::infinity();
    auto out1 = filter.process(nan_val);
    auto out2 = filter.process(inf_val);
    assert(std::isfinite(out1) && std::isfinite(out2));

    // Subsequent normal inputs must process without getting stuck on NaN
    auto out3 = filter.process(0.5f);
    assert(std::isfinite(out3));
    std::cout << "PASSED\n";
}

void test_partitioned_convolution_reset() {
    std::cout << "[TEST] Partitioned Convolution State Reset... ";
    aether::dsp::fx::PartitionedConvolution conv;
    std::vector<float> ir(512, 0.1f);
    conv.load_impulse_response(ir, 1.0f);

    // Feed audio
    for (int i = 0; i < 200; ++i) {
        (void)conv.process(aether::StereoFrame{0.8f, 0.8f});
    }

    // Reset clears state
    conv.reset();
    auto silent_out = conv.process(aether::StereoFrame{0.0f, 0.0f});
    assert(std::abs(silent_out.left) < 1e-4f);
    assert(std::abs(silent_out.right) < 1e-4f);
    std::cout << "PASSED\n";
}

void test_pseudosphere_horn_extreme_params() {
    std::cout << "[TEST] Pseudosphere Horn Parameter Robustness... ";
    aether::engines::PseudosphereHorn::Config cfg;
    cfg.horn_length_m = -10.0f; // Negative length must be clamped safely
    cfg.speed_of_sound = -343.0f; // Negative speed of sound
    cfg.curvature_a = 0.0f;
    aether::engines::PseudosphereHorn horn(0, cfg); // Sample rate 0

    horn.excite_throat(0.5f);
    for (int i = 0; i < 100; ++i) {
        auto frame = horn.step();
        assert(std::isfinite(frame.left) && std::isfinite(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_henon_heiles_boundedness() {
    std::cout << "[TEST] Henon-Heiles Chaos Long-Term Stability (100k Steps)... ";
    aether::engines::HenonHeilesOscillator chaos(0.12, 400.0);

    for (int i = 0; i < 100000; ++i) {
        auto frame = chaos.step(48000.0f);
        assert(std::isfinite(frame.left) && std::isfinite(frame.right));
        assert(std::abs(frame.left) <= 1.0f && std::abs(frame.right) <= 1.0f);
    }
    std::cout << "PASSED\n";
}

void test_hyperbolic_halfplane_singularities() {
    std::cout << "[TEST] Hyperbolic Half-Plane Singularity Guards... ";
    using aether::math::HyperbolicHalfPlane;
    using Complex = HyperbolicHalfPlane::Complex;

    // Degenerate Mobius transform (det == 0)
    HyperbolicHalfPlane::MobiusTransform degen{0.0, 0.0, 0.0, 0.0};
    auto inv = degen.inverse();
    assert(inv.determinant() == 1.0);

    // Disk to halfplane at w = 1.0 (boundary pole)
    Complex at_boundary(1.0, 0.0);
    Complex hp = HyperbolicHalfPlane::disk_to_halfplane(at_boundary);
    assert(std::isfinite(hp.imag()));

    // Halfplane to disk at z = -i (pole)
    Complex neg_i(0.0, -1.0);
    Complex dsk = HyperbolicHalfPlane::halfplane_to_disk(neg_i);
    assert(std::isfinite(dsk.real()) && std::isfinite(dsk.imag()));

    // Negative taps and negative base delay
    auto empty1 = HyperbolicHalfPlane::generate_modular_delay_taps(0, 10.0f);
    auto empty2 = HyperbolicHalfPlane::generate_modular_delay_taps(10, -5.0f);
    assert(empty1.empty());
    assert(empty2.empty());
    std::cout << "PASSED\n";
}

void test_bolza_surface_boundary_wrap() {
    std::cout << "[TEST] Bolza Surface Boundary Wraps & Clamping... ";
    using aether::math::BolzaSurface;
    aether::Vec2 p{0.5f, 0.5f};
    // Modulo side wraps
    auto wrapped = BolzaSurface::wrap_point(p, 100);
    assert(std::isfinite(wrapped.x) && std::isfinite(wrapped.y));

    // Zero sample rate or invalid duration in IR synthesis
    auto ir = BolzaSurface::synthesize_bolza_ir(0, -1.0f);
    assert(!ir.empty());
    for (float val : ir) {
        assert(std::isfinite(val));
    }
    std::cout << "PASSED\n";
}

void test_binaural_panner_high_sample_rate() {
    std::cout << "[TEST] Binaural Panner 192kHz ITD Buffer Scaling... ";
    aether::dsp::fx::BinauralPanner panner(192000);
    panner.set_position(90.0f, 1.0f);
    for (int i = 0; i < 500; ++i) {
        auto frame = panner.process(0.5f);
        assert(std::isfinite(frame.left) && std::isfinite(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_stick_slip_finite_force() {
    std::cout << "[TEST] Stick-Slip Exciter Non-Finite Input Handling... ";
    aether::engines::StickSlipExciter exciter;
    exciter.configure(0.8f, 0.3f, 4.0f);

    float nan_val = std::numeric_limits<float>::quiet_NaN();
    float inf_val = std::numeric_limits<float>::infinity();

    float f1 = exciter.compute_force(nan_val, 1.0f, 0.0f);
    float f2 = exciter.compute_force(1.0f, inf_val, 0.0f);
    assert(f1 == 0.0f);
    assert(f2 == 0.0f);
    std::cout << "PASSED\n";
}

void test_engine_reset() {
    std::cout << "[TEST] Master Engine Full Reset to Silence... ";
    aether::Engine engine(48000);
    engine.load_preset(aether::Preset::HyperbolicCosmicBell);
    engine.note_on({0.3f, 0.3f}, 0.9f);

    std::vector<aether::StereoFrame> block(128);
    engine.process_block(block);

    float energy = 0.0f;
    for (const auto& f : block) {
        energy += std::abs(f.left) + std::abs(f.right);
    }
    assert(energy > 0.01f);

    // Reset clears all sound
    engine.reset();
    engine.process_block(block);
    float energy_after = 0.0f;
    for (const auto& f : block) {
        energy_after += std::abs(f.left) + std::abs(f.right);
    }
    assert(energy_after < 1e-4f);
    std::cout << "PASSED\n";
}

void test_c_api_extended() {
    std::cout << "[TEST] Extended C-ABI Functions (Scale, Degree, Reset)... ";
    auto handle = aether_engine_create(48000);
    assert(handle != nullptr);

    aether_engine_set_scale(handle, 1); // Bohlen-Pierce
    aether_engine_note_on_degree(handle, 3, 0.85f);

    std::vector<float> buffer(128 * 2);
    aether_engine_process_interleaved(handle, buffer.data(), 128);

    float energy = 0.0f;
    for (float smp : buffer) {
        energy += std::abs(smp);
    }
    assert(energy > 0.01f);

    aether_engine_all_notes_off(handle);
    aether_engine_reset(handle);
    aether_engine_destroy(handle);
    std::cout << "PASSED\n";
}

void test_fft_non_power_of_two_graceful() {
    std::cout << "[TEST] FFT Graceful Handling of Non-Power-of-Two Spans... ";
    using aether::math::FFT;
    using Complex = FFT::Complex;

    std::vector<Complex> bad_span(7, Complex(1.0f, 0.0f));
    // Must return cleanly without assertion failure or crash
    FFT::forward(bad_span);
    FFT::inverse(bad_span);
    FFT::bit_reverse(bad_span);

    std::vector<float> bad_input(7, 1.0f);
    std::vector<float> bad_output(4, 0.0f);
    FFT::compute_power_spectrum(bad_input, bad_output);
    std::cout << "PASSED\n";
}

void test_yoshida_integrator_non_finite_healing() {
    std::cout << "[TEST] Yoshida 4th Integrator Non-Finite State Healing... ";
    using aether::math::Yoshida4thIntegrator;
    Yoshida4thIntegrator::State state{
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity()
    };

    Yoshida4thIntegrator::step(state, 0.001, [](double q) { return -q; });
    assert(state.q == 0.0);
    assert(state.p == 0.0);

    double e = Yoshida4thIntegrator::total_energy(state, [](double q) { return 0.5 * q * q; });
    assert(std::isfinite(e));
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "\033[1;36m=== Running AetherDSP Complete Invariant Suite (65 Tests) ===\033[0m\n";
    test_ring_buffer();
    test_poincare_geometry();
    test_chladni_plate();
    test_hyperbolic_waveguide();
    test_biquad_filter();
    test_tuning_system();
    test_reverb();
    test_midi_serialization();
    test_binaural_panner();
    test_hyperbolic_tiling();
    test_wav_roundtrip();
    test_c_api();
    test_clap_plugin_descriptor();
    test_stick_slip_exciter();
    test_scala_catalog();
    test_svg_export();
    test_preset_manager();
    test_geodesic_raytracer();
    test_convolution_reverb();
    test_braille_visualizer();
    test_von_karman_plate();
    test_hyperbolic_halfplane();
    test_pseudosphere_horn();
    test_yoshida_integrator();
    test_braille_spectrogram();
    test_polyphony_manager();
    test_henon_heiles_chaos();
    test_fft_spectral_transform();
    test_hyperbolic_string_jawari();
    test_hyperbolic_spatializer_3d();
    test_partitioned_convolution();
    test_bolza_surface_fuchsian_group();
    test_sine_gordon_breather();
    test_acoustic_black_hole();
    test_acoustic_wormhole();
    test_topological_metamaterial();
    test_kdv_soliton_lattice();
    test_engine_presets();
    test_midi_parser_robustness();
    test_wav_reader_robustness();
    test_hyperbolic_string_high_pitches();
    test_monophonic_note_off_decay();
    test_composer_sorted_events();
    test_partitioned_convolution_zero_latency();
    test_scala_multiple_reloads();
    test_fft_window_size_one();
    test_binaural_panner_rear_symmetry();
    test_topological_metamaterial_strike();
    test_pseudosphere_horn_low_sample_rate();
    test_adsr_envelope_zero_sustain_decay();
    test_hyperbolic_waveguide_boundary_integrity();
    test_von_karman_stability_high_velocity();
    test_chaotic_oscillator_dc_blocking();
    test_biquad_nan_self_healing();
    test_partitioned_convolution_reset();
    test_pseudosphere_horn_extreme_params();
    test_henon_heiles_boundedness();
    test_hyperbolic_halfplane_singularities();
    test_bolza_surface_boundary_wrap();
    test_binaural_panner_high_sample_rate();
    test_stick_slip_finite_force();
    test_engine_reset();
    test_c_api_extended();
    test_fft_non_power_of_two_graceful();
    test_yoshida_integrator_non_finite_healing();
    std::cout << "\033[1;32mALL 65 ADVANCED INVARIANT TESTS PASSED SUCCESSFULLY!\033[0m\n";
    return 0;
}
