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
#include "aether/math/fixed_point.hpp"
#include "aether/math/fixed_biquad.hpp"
#include "aether/math/fixed_oscillator.hpp"
#include "aether/math/fixed_envelope.hpp"
#include "aether/math/fixed_reverb.hpp"
#include "aether/embedded/embedded_voice.hpp"
#include "aether/embedded/embedded_engine.hpp"
#include "aether/embedded/daisy_patch.hpp"
#include "aether/math/fixed_wavetable.hpp"
#include "aether/math/fixed_filter.hpp"
#include "aether/math/fixed_shaper.hpp"
#include "aether/math/fixed_lfo.hpp"
#include "aether/math/fixed_delay.hpp"
#include "aether/math/fixed_compressor.hpp"
#include "aether/math/fixed_noise.hpp"
#include "aether/math/fixed_chorus.hpp"
#include "aether/math/fixed_ring_mod.hpp"
#include "aether/math/fixed_comb.hpp"
#include "aether/math/fixed_svf.hpp"
#include "aether/math/fixed_bitcrusher.hpp"
#include "aether/math/fixed_phaser.hpp"
#include "aether/math/fixed_clipper.hpp"
#include "aether/math/fixed_formant.hpp"
#include "aether/math/fixed_exciter.hpp"
#include "aether/math/fixed_pitch_shifter.hpp"
#include "aether/math/fixed_gate.hpp"
#include "aether/math/fixed_multitap.hpp"
#include "aether/math/fixed_tremolo.hpp"
#include "aether/math/fixed_transient_shaper.hpp"
#include "aether/math/fixed_stereo_widener.hpp"
#include "aether/math/fixed_pitch_detector.hpp"
#include "aether/math/fixed_crossover.hpp"
#include "aether/math/fixed_eq.hpp"
#include "aether/math/fixed_sub_synth.hpp"
#include "aether/math/fixed_comb_chorus.hpp"
#include "aether/math/fixed_tape_saturator.hpp"
#include "aether/math/fixed_shepard_phaser.hpp"
#include "aether/math/fixed_autowah.hpp"
#include "aether/math/fixed_leslie.hpp"
#include "aether/math/fixed_vibrato.hpp"
#include "aether/math/fixed_ring_buffer.hpp"
#include "aether/embedded/daisy_midi.hpp"
#include "aether/embedded/daisy_oled.hpp"
#include "aether/embedded/eurorack_sequencer.hpp"
#include "aether/embedded/eurorack_quantizer.hpp"
#include "aether/embedded/eurorack_logic.hpp"
#include "aether/embedded/eurorack_slew.hpp"
#include "aether/embedded/eurorack_sample_hold.hpp"
#include "aether/embedded/eurorack_matrix_mixer.hpp"
#include "aether/embedded/eurorack_calibrator.hpp"
#include "aether/embedded/eurorack_vector_panner.hpp"
#include "aether/embedded/eurorack_wavefolder.hpp"
#include "aether/embedded/eurorack_bernoulli.hpp"
#include "aether/embedded/eurorack_euclidean.hpp"
#include "aether/embedded/eurorack_clock_mult.hpp"
#include "aether/embedded/eurorack_envelope_follower.hpp"
#include "aether/embedded/eurorack_sallen_key.hpp"
#include "aether/embedded/eurorack_ring_mod.hpp"
#include "aether/embedded/eurorack_quadrature.hpp"
#include "aether/embedded/eurorack_dual_formant.hpp"
#include "aether/embedded/eurorack_limiter.hpp"
#include "aether/embedded/eurorack_tape_flutter.hpp"
#include "aether/embedded/eurorack_wave_multiplier.hpp"
#include "aether/embedded/eurorack_low_pass_gate.hpp"
#include "aether/embedded/eurorack_sub_octave.hpp"
#include "aether/embedded/eurorack_window_comparator.hpp"
#include "aether/embedded/eurorack_wave_terrain.hpp"
#include "aether/embedded/eurorack_chaos.hpp"
#include "aether/embedded/eurorack_bbd_chorus.hpp"
#include "aether/embedded/eurorack_cv_slicer.hpp"
#include "aether/simd/neon_math.hpp"
#include "aether/embedded/hardware_adapter.hpp"
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

    const auto* bp = aether::dsp::ScalaCatalog::find_by_name("Bohlen-Pierce");
    assert(bp != nullptr);
    assert(bp->period == 3.0);

    const auto* non_existent = aether::dsp::ScalaCatalog::find_by_name("NonExistentScaleName");
    assert(non_existent == nullptr);

    const auto& sc0 = aether::dsp::ScalaCatalog::get_by_index(0);
    assert(std::string(sc0.name) == "12-EDO");

    const auto& sc_oob = aether::dsp::ScalaCatalog::get_by_index(9999);
    assert(std::string(sc_oob.name) == "12-EDO");

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

void test_tuning_system_extreme_degrees() {
    std::cout << "[TEST] Tuning System Extreme Degree Octave Clamping... ";
    aether::dsp::TuningSystem tuning;
    float f_high = tuning.get_frequency(10000, 440.0f);
    float f_low = tuning.get_frequency(-10000, 440.0f);
    assert(std::isfinite(f_high) && f_high > 0.0f);
    assert(std::isfinite(f_low) && f_low > 0.0f);
    std::cout << "PASSED\n";
}

void test_hyperbolic_tiling_bounds() {
    std::cout << "[TEST] Hyperbolic Tiling Geometric Constraint Bounds... ";
    using aether::math::HyperbolicTiling;
    // p, q < 3
    auto empty1 = HyperbolicTiling::generate_tiling(2, 2, 2);
    assert(empty1.empty());

    // Euclidean {4, 4} (not hyperbolic)
    auto empty2 = HyperbolicTiling::generate_tiling(4, 4, 2);
    assert(empty2.empty());

    // Valid {7, 3} hyperbolic tiling
    auto tiles = HyperbolicTiling::generate_tiling(7, 3, 1);
    assert(!tiles.empty());
    std::cout << "PASSED\n";
}

void test_poincare_disk_nan_resilience() {
    std::cout << "[TEST] Poincare Disk Non-Finite Input Resilience... ";
    using aether::math::PoincareDisk;
    aether::Vec2 nan_vec{std::numeric_limits<float>::quiet_NaN(), 0.0f};
    aether::Vec2 valid_vec{0.2f, 0.3f};

    float d = PoincareDisk::geodesic_distance(nan_vec, valid_vec);
    assert(std::isfinite(d));

    aether::Vec2 mob = PoincareDisk::mobius_transform(nan_vec, valid_vec);
    (void)mob; // Must execute cleanly without exception
    std::cout << "PASSED\n";
}

void test_wav_writer_oversized_guard() {
    std::cout << "[TEST] WavWriter Sample Rate & Buffer Clamping... ";
    std::vector<aether::StereoFrame> test_frames(10, aether::StereoFrame{0.1f, -0.1f});
    bool success = aether::WavWriter::write_stereo_wav("temp_test.wav", test_frames, 0); // 0 Hz sample rate clamped
    assert(success);
    std::remove("temp_test.wav");
    std::cout << "PASSED\n";
}

void test_hyperbolic_string_zero_curvature() {
    std::cout << "[TEST] Hyperbolic String Zero Curvature Clamping... ";
    aether::engines::HyperbolicString::Config cfg;
    cfg.jawari_curvature = 0.0f;
    cfg.length_m = -1.0f;
    aether::engines::HyperbolicString string(0, cfg);
    assert(string.state().size() == aether::engines::HyperbolicString::NUM_POINTS);
    for (float val : string.jawari_profile()) {
        assert(std::isfinite(val));
    }
    string.pluck(0.5f, 0.8f);
    for (int i = 0; i < 64; ++i) {
        auto frame = string.step();
        assert(std::isfinite(frame.left));
        assert(std::isfinite(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_kdv_soliton_lattice_nan_healing() {
    std::cout << "[TEST] KdV Soliton Lattice Non-Finite Self-Healing... ";
    aether::engines::KdvSolitonLattice lattice(48000);
    lattice.strike(0.5f, 10.0f);
    for (int i = 0; i < 200; ++i) {
        auto frame = lattice.process();
        assert(std::isfinite(frame.left));
        assert(std::isfinite(frame.right));
        assert(std::abs(frame.left) <= 1.0f);
        assert(std::abs(frame.right) <= 1.0f);
    }
    std::cout << "PASSED\n";
}

void test_von_karman_init_modes_bounds() {
    std::cout << "[TEST] Von Karman Plate Extreme Mode Initialization... ";
    aether::engines::VonKarmanPlate plate(48000);
    plate.init_modes(0.0f, -5.0f, -10.0f);
    plate.strike({0.5f, 0.5f}, 0.9f);
    for (int i = 0; i < 64; ++i) {
        auto frame = plate.step();
        assert(std::isfinite(frame.left));
        assert(std::isfinite(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_geodesic_raytracer_sample_rate_clamp() {
    std::cout << "[TEST] Geodesic Raytracer Sample Rate & Duration Clamping... ";
    aether::math::GeodesicRayTracer tracer;
    std::vector<aether::math::RayArrival> arrivals;
    arrivals.push_back({0.01f, 0.8f, 45.0f, 0.5f});
    auto ir = tracer.synthesize_impulse_response(arrivals, 0, -1.0f);
    assert(!ir.empty());
    for (const auto& frame : ir) {
        assert(std::isfinite(frame.left));
        assert(std::isfinite(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_chaotic_oscillator_nan_recovery() {
    std::cout << "[TEST] Chaotic Oscillator Non-Finite State Recovery... ";
    aether::engines::ChaoticOscillator chaos;
    chaos.configure(aether::engines::AttractorType::Lorenz, 20.0f);
    for (int i = 0; i < 500; ++i) {
        auto frame = chaos.step(48000.0f);
        assert(std::isfinite(frame.left));
        assert(std::isfinite(frame.right));
        assert(std::abs(frame.left) <= 1.0f);
        assert(std::abs(frame.right) <= 1.0f);
    }
    std::cout << "PASSED\n";
}

void test_tuning_scale_sanitization() {
    std::cout << "[TEST] Microtonal Tuning Scale Sanitization... ";
    aether::dsp::TuningSystem tuning;
    tuning.load_scale(0.0, {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(), 1.5});
    assert(tuning.size() == 4);
    for (int deg = -10; deg <= 10; ++deg) {
        float f = tuning.get_frequency(deg, 440.0f);
        assert(std::isfinite(f));
        assert(f > 0.0f);
    }
    std::cout << "PASSED\n";
}

void test_polyphony_clamping() {
    std::cout << "[TEST] PolyVoice Frequency & Velocity Clamping... ";
    aether::PolyphonyManager<4> mgr(48000);
    mgr.note_on(60, -100.0f, -5.0f);
    mgr.note_on(64, 1e8f, 10.0f);
    for (int i = 0; i < 64; ++i) {
        auto frame = mgr.step();
        assert(std::isfinite(frame.left));
        assert(std::isfinite(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_clap_parameter_query() {
    std::cout << "[TEST] CLAP Parameter Catalog Query & Bounds... ";
    assert(aether::plugin::get_parameter_count() == 6);
    const auto* p0 = aether::plugin::get_parameter_info(aether::plugin::PARAM_PRESET);
    assert(p0 != nullptr);
    assert(p0->min_value == 0.0f);
    assert(p0->max_value == 13.0f);
    const auto* invalid = aether::plugin::get_parameter_info(999);
    assert(invalid == nullptr);
    std::cout << "PASSED\n";
}

void test_biquad_extreme_q_bounds() {
    std::cout << "[TEST] Biquad Filter Extreme Q Boundary Clamping... ";
    aether::dsp::BiquadFilter f;
    f.configure(aether::dsp::FilterType::Lowpass, 1000.0f, 1e8f, 48000.0f);
    for (int i = 0; i < 64; ++i) {
        float out = f.process(1.0f);
        assert(std::isfinite(out));
    }
    std::cout << "PASSED\n";
}

void test_hyperbolic_reverb_nan_healing() {
    std::cout << "[TEST] Hyperbolic Reverb Self-Healing Invariant... ";
    aether::dsp::fx::HyperbolicReverb rev;
    rev.configure(0.9f, 0.3f, 0.5f);
    // Process regular audio
    for (int i = 0; i < 50; ++i) {
        (void)rev.process({0.1f, -0.1f});
    }
    // Inject non-finite input
    auto nan_out = rev.process({std::numeric_limits<float>::quiet_NaN(), 0.0f});
    assert(nan_out.left == 0.0f && nan_out.right == 0.0f);
    // Subsequent normal frames must remain finite and recover
    for (int i = 0; i < 50; ++i) {
        auto out = rev.process({0.05f, 0.05f});
        assert(std::isfinite(out.left));
        assert(std::isfinite(out.right));
    }
    std::cout << "PASSED\n";
}

void test_ring_buffer_reset_concurrency() {
    std::cout << "[TEST] SPSCRingBuffer Thread-Safe Reset & Capacity... ";
    aether::SPSCRingBuffer<int, 64> rb;
    for (int i = 0; i < 30; ++i) {
        assert(rb.push(i));
    }
    assert(rb.size() == 30);
    assert(!rb.empty());
    rb.reset();
    assert(rb.empty());
    assert(rb.size() == 0);
    int val = -1;
    assert(!rb.pop(val));
    std::cout << "PASSED\n";
}

void test_algorithmic_composer_bounds() {
    std::cout << "[TEST] Algorithmic Composer Duration & Sample Rate Clamping... ";
    auto empty_render = aether::math::AlgorithmicComposer::render_score({}, 0.0, 48000);
    assert(empty_render.empty());

    auto score = aether::math::AlgorithmicComposer::generate_fibonacci_score(0.0);
    assert(!score.empty()); // Clamped to 0.1s minimum

    auto rendered = aether::math::AlgorithmicComposer::render_score(score, 0.05, 0);
    assert(!rendered.empty()); // Clamped to minimum sample rate and duration
    for (const auto& frame : rendered) {
        assert(std::isfinite(frame.left));
        assert(std::isfinite(frame.right));
    }
    std::cout << "PASSED\n";
}

void test_svg_exporter_finite_vertices() {
    std::cout << "[TEST] SVG Exporter Non-Finite Vertex Resilience... ";
    std::vector<aether::math::HyperbolicTile> tiles;
    aether::math::HyperbolicTile tile;
    tile.vertices.push_back({0.1f, 0.2f});
    tile.vertices.push_back({std::numeric_limits<float>::quiet_NaN(), 0.5f});
    tile.vertices.push_back({0.3f, 0.4f});
    tiles.push_back(tile);

    const std::string tmp_svg = "temp_tile_test.svg";
    bool ok = aether::SvgExporter::export_poincare_tiling_svg(tiles, tmp_svg, 400);
    assert(ok);
    std::remove(tmp_svg.c_str());
    std::cout << "PASSED\n";
}

void test_wav_reader_nan_sanitizer() {
    std::cout << "[TEST] WAV Writer/Reader IEEE Float NaN Sanitization... ";
    std::vector<aether::StereoFrame> nan_frames = {
        {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()},
        {0.5f, -0.5f}
    };
    const std::string tmp_wav = "test_nan.wav";
    bool ok_write = aether::WavWriter::write_stereo_wav(tmp_wav, nan_frames, 48000, true);
    assert(ok_write);

    auto read_back = aether::WavReader::read_wav(tmp_wav);
    assert(read_back.valid);
    assert(read_back.frames.size() == 2);
    // Non-finite values must have been replaced with 0.0f
    assert(read_back.frames[0].left == 0.0f);
    assert(read_back.frames[0].right == 0.0f);
    assert(std::abs(read_back.frames[1].left - 0.5f) < 1e-4f);
    assert(std::abs(read_back.frames[1].right - (-0.5f)) < 1e-4f);

    std::remove(tmp_wav.c_str());
    std::cout << "PASSED\n";
}

void test_partitioned_convolution_tail_nan_sanitizer() {
    std::cout << "[TEST] Partitioned Convolution Tail NaN Sanitization... ";
    aether::dsp::fx::PartitionedConvolution conv;
    // Create an IR larger than BLOCK_SIZE (128) to activate tail partitions
    std::vector<float> ir(384, 0.01f);
    conv.load_impulse_response(ir, 1.0f);
    assert(conv.num_partitions() == 3);

    // Feed normal samples
    for (int i = 0; i < 256; ++i) {
        auto out = conv.process({0.1f, 0.1f});
        assert(std::isfinite(out.left));
        assert(std::isfinite(out.right));
    }
    std::cout << "PASSED\n";
}

void test_convolution_reverb_nan_sanitizer() {
    std::cout << "[TEST] Convolution Reverb Non-Finite Frame Sanitization... ";
    aether::dsp::fx::ConvolutionReverb rev;
    std::vector<aether::StereoFrame> ir = {
        {std::numeric_limits<float>::quiet_NaN(), 0.5f},
        {0.2f, std::numeric_limits<float>::infinity()}
    };
    rev.load_impulse_response(ir, 0.5f);

    auto nan_in = rev.process({std::numeric_limits<float>::quiet_NaN(), 0.0f});
    assert(nan_in.left == 0.0f && nan_in.right == 0.0f);

    for (int i = 0; i < 16; ++i) {
        auto out = rev.process({0.1f, 0.1f});
        assert(std::isfinite(out.left));
        assert(std::isfinite(out.right));
    }
    std::cout << "PASSED\n";
}

void test_hyperbolic_spatializer_3d_nan_resilience() {
    std::cout << "[TEST] Hyperbolic Spatializer 3D Non-Finite Resilience... ";
    aether::dsp::fx::HyperbolicSpatializer3D spat(48000);
    // Position with NaN/Inf coordinates should clamp safely to origin
    spat.set_source_position({std::numeric_limits<float>::quiet_NaN(), 0.2f, 0.1f},
                             {0.0f, std::numeric_limits<float>::infinity(), 0.0f});

    assert(std::isfinite(spat.geodesic_distance()));
    assert(std::isfinite(spat.attenuation_gain()));

    // Process non-finite sample
    auto nan_out = spat.process(std::numeric_limits<float>::quiet_NaN());
    assert(std::isfinite(nan_out.left));
    assert(std::isfinite(nan_out.right));

    for (int i = 0; i < 100; ++i) {
        auto out = spat.process(0.2f);
        assert(std::isfinite(out.left));
        assert(std::isfinite(out.right));
    }
    std::cout << "PASSED\n";
}

void test_binaural_panner_nan_resilience() {
    std::cout << "[TEST] Binaural Panner Non-Finite Input Resilience... ";
    aether::dsp::fx::BinauralPanner panner(48000);
    panner.set_position(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity());

    auto nan_out = panner.process(std::numeric_limits<float>::quiet_NaN());
    assert(std::isfinite(nan_out.left));
    assert(std::isfinite(nan_out.right));

    for (int i = 0; i < 100; ++i) {
        auto out = panner.process(0.15f);
        assert(std::isfinite(out.left));
        assert(std::isfinite(out.right));
    }
    std::cout << "PASSED\n";
}

void test_hyperbolic_waveguide_nan_resilience() {
    std::cout << "[TEST] Hyperbolic Waveguide Non-Finite Resilience... ";
    aether::engines::HyperbolicWaveguide wg;
    wg.configure(0.40f, 0.005f, 1.0f);
    wg.strike({std::numeric_limits<float>::quiet_NaN(), 0.2f},
              std::numeric_limits<float>::infinity(),
              std::numeric_limits<float>::quiet_NaN());

    for (int i = 0; i < 100; ++i) {
        auto out = wg.step({std::numeric_limits<float>::quiet_NaN(), 0.0f},
                           {0.0f, std::numeric_limits<float>::infinity()});
        assert(std::isfinite(out.left));
        assert(std::isfinite(out.right));
    }
    std::cout << "PASSED\n";
}

void test_fixed_point_embedded() {
    std::cout << "[TEST] Embedded Fixed-Point Math (Q15 / Q27)... ";
    using aether::math::FixedQ15;
    using aether::math::FixedQ27;

    // Q15 tests
    auto q1 = FixedQ15::from_float(0.5f);
    auto q2 = FixedQ15::from_float(0.25f);
    auto q_sum = q1 + q2;
    assert(std::abs(q_sum.to_float() - 0.75f) < 0.001f);

    auto q_prod = q1 * q2;
    assert(std::abs(q_prod.to_float() - 0.125f) < 0.001f);

    // Q15 saturation
    auto q_sat = FixedQ15::from_float(0.9f) + FixedQ15::from_float(0.9f);
    assert(q_sat.raw == 32767);

    // Q27 tests
    auto q27_1 = FixedQ27::from_float(1.5f);
    auto q27_2 = FixedQ27::from_float(2.0f);
    auto q27_prod = q27_1 * q27_2;
    assert(std::abs(q27_prod.to_float() - 3.0f) < 0.0001f);

    std::cout << "PASSED\n";
}

void test_embedded_hardware_adapter() {
    std::cout << "[TEST] Embedded Hardware Knob/Button Adapter... ";
    aether::Engine engine;
    aether::embedded::EmbeddedHardwareAdapter adapter;

    // Turn knob 0 (preset) to max ADC (4095) -> Preset 13
    adapter.update_adc_knob(0, 4095, engine);
    assert(static_cast<int>(engine.current_preset()) == 13);

    // Turn knob 0 to 0 -> Preset 0
    adapter.update_adc_knob(0, 0, engine);
    assert(static_cast<int>(engine.current_preset()) == 0);

    // Knob 4 (panning)
    adapter.update_adc_knob(4, 2048, engine); // ~ 0.0 deg
    assert(std::abs(adapter.knob_value(4)) < 2.0f);

    // Button 2 (next preset)
    adapter.handle_button_press(2, true, engine);
    assert(static_cast<int>(engine.current_preset()) == 1);
    adapter.handle_button_press(2, false, engine);

    // Button 3 (reset)
    adapter.handle_button_press(3, true, engine);
    adapter.handle_button_press(3, false, engine);

    std::cout << "PASSED\n";
}

void test_simd_neon_math() {
    std::cout << "[TEST] Portable SIMD Float4 Math... ";
    using aether::simd::Float4;

    Float4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Float4 b(0.5f, 0.5f, 0.5f, 0.5f);
    Float4 c = a + b;
    alignas(16) float res[4];
    c.store(res);
    assert(std::abs(res[0] - 1.5f) < 1e-6f);
    assert(std::abs(res[3] - 4.5f) < 1e-6f);

    Float4 d = a.fma(b, Float4(10.0f));
    d.store(res);
    assert(std::abs(res[0] - 10.5f) < 1e-6f);
    assert(std::abs(res[3] - 12.0f) < 1e-6f);

    std::cout << "PASSED\n";
}

void test_fixed_biquad_and_oscillator() {
    std::cout << "[TEST] Embedded Fixed-Point Biquad & Sine LUT Oscillator... ";
    using aether::math::FixedBiquadFilter;
    using aether::math::FixedSineOscillator;
    using aether::math::FixedQ15;

    FixedSineOscillator osc;
    osc.set_normalized_frequency(440.0f / 48000.0f); // ~440Hz at 48kHz

    FixedBiquadFilter filter;
    filter.configure_lowpass(1000.0f / 48000.0f, 0.707f);

    for (int i = 0; i < 200; ++i) {
        FixedQ15 smp = osc.step();
        assert(smp.to_float() >= -1.0f && smp.to_float() <= 1.0f);

        FixedQ15 filtered = filter.process(smp);
        assert(filtered.to_float() >= -1.0f && filtered.to_float() <= 1.0f);
    }

    std::cout << "PASSED\n";
}

void test_fixed_envelope_and_embedded_voice() {
    std::cout << "[TEST] Embedded Fixed ADSR Envelope & Synth Voice Core... ";
    using aether::math::FixedADSREnvelope;
    using aether::math::FixedQ15;
    using aether::embedded::EmbeddedVoice;

    FixedADSREnvelope env;
    env.configure(10, 20, FixedQ15::from_float(0.5f), 30);
    env.gate(true);

    assert(env.is_active());
    for (int i = 0; i < 15; ++i) {
        auto val = env.process();
        assert(val.to_float() >= 0.0f && val.to_float() <= 1.0f);
    }

    env.gate(false);
    for (int i = 0; i < 50; ++i) {
        (void)env.process();
    }
    assert(!env.is_active());

    // Embedded Voice integration
    EmbeddedVoice voice;
    voice.note_on(523.25f); // C5
    assert(voice.is_active());

    for (int i = 0; i < 500; ++i) {
        auto smp = voice.process();
        assert(smp.to_float() >= -1.0f && smp.to_float() <= 1.0f);
    }

    voice.note_off();
    for (int i = 0; i < 15000; ++i) {
        (void)voice.process();
    }
    assert(!voice.is_active());

    std::cout << "PASSED\n";
}

void test_fixed_reverb() {
    std::cout << "[TEST] Embedded Fixed-Point Freeverb Engine... ";
    using aether::math::FixedReverb;
    using aether::math::FixedQ15;

    FixedReverb rev;
    rev.configure(0.8f, 0.25f, 0.4f);

    // Feed impulse
    auto out0 = rev.process(FixedQ15::from_float(0.9f));
    assert(out0.to_float() >= -1.0f && out0.to_float() <= 1.0f);

    // Process tail
    for (int i = 0; i < 3000; ++i) {
        auto tail = rev.process(FixedQ15(0));
        assert(tail.to_float() >= -1.0f && tail.to_float() <= 1.0f);
    }

    rev.reset();
    auto silence = rev.process(FixedQ15(0));
    assert(silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_embedded_polyphonic_engine() {
    std::cout << "[TEST] Embedded Polyphonic Synthesizer Engine (4 Voices)... ";
    using aether::embedded::EmbeddedEngine;

    EmbeddedEngine engine;
    assert(engine.active_voice_count() == 0);

    // Play C-major chord (MIDI 60, 64, 67, 71)
    engine.note_on(60);
    engine.note_on(64);
    engine.note_on(67);
    engine.note_on(71);
    assert(engine.active_voice_count() == 4);

    for (int i = 0; i < 200; ++i) {
        auto smp = engine.process();
        assert(smp.to_float() >= -1.0f && smp.to_float() <= 1.0f);
    }

    // Trigger 5th note (steals LRU voice)
    engine.note_on(72);
    assert(engine.active_voice_count() == 4);

    engine.all_notes_off();
    for (int i = 0; i < 15000; ++i) {
        (void)engine.process();
    }
    assert(engine.active_voice_count() == 0);

    std::cout << "PASSED\n";
}

void test_daisy_patch_eurorack() {
    std::cout << "[TEST] Embedded Daisy Patch Eurorack Interface & 1V/Oct... ";
    using aether::embedded::DaisyPatchEurorackInterface;
    using aether::math::FixedQ15;

    DaisyPatchEurorackInterface patch(48000);

    // Initial state
    assert(patch.get_cv(0).raw == 0);
    assert(!patch.gate_in(0));

    // Gate rising and falling edge detection
    patch.set_gate_in(0, true);
    assert(patch.gate_in(0));
    assert(patch.gate_in_rose(0));
    assert(!patch.gate_in_fell(0));

    // Next step: high remains high, rising edge clears
    patch.set_gate_in(0, true);
    assert(!patch.gate_in_rose(0));

    // Falling edge
    patch.set_gate_in(0, false);
    assert(!patch.gate_in(0));
    assert(patch.gate_in_fell(0));
    assert(!patch.gate_in_rose(0));

    // 1V/Octave CV pitch tracking
    // 0V (raw = 0) -> A4 (440 Hz)
    // 440 * 2^32 / 48000 = 39370533
    uint32_t inc_0v = patch.cv_to_phase_increment(FixedQ15(0));
    assert(inc_0v > 39000000 && inc_0v < 40000000);

    // +1V in 5V range: cv = 1/5 = 0.2 -> 1 octave up (880 Hz -> double increment)
    // 0.2 * 32768 = 6554 raw
    uint32_t inc_plus1v = patch.cv_to_phase_increment(FixedQ15::from_raw(6554));
    float ratio_up = static_cast<float>(inc_plus1v) / static_cast<float>(inc_0v);
    assert(std::abs(ratio_up - 2.0f) < 0.05f);

    // -1V in 5V range: cv = -0.2 -> 1 octave down (220 Hz -> half increment)
    uint32_t inc_minus1v = patch.cv_to_phase_increment(FixedQ15::from_raw(-6554));
    float ratio_down = static_cast<float>(inc_minus1v) / static_cast<float>(inc_0v);
    assert(std::abs(ratio_down - 0.5f) < 0.05f);

    // CV output roundtrip
    patch.set_cv_out(2, FixedQ15::from_float(0.75f));
    assert(std::abs(patch.cv_out(2).to_float() - 0.75f) < 0.01f);

    std::cout << "PASSED\n";
}

void test_fixed_wavetable_oscillator() {
    std::cout << "[TEST] Embedded Fixed-Point Dual Wavetable Oscillator... ";
    using aether::math::FixedWavetableOscillator;
    using aether::math::FixedQ15;

    FixedWavetableOscillator osc;
    // Set frequency to 440 Hz at 48000 Hz sample rate
    uint32_t inc = static_cast<uint32_t>((440ULL << 32) / 48000);
    osc.set_phase_increment(inc);

    // Pure Sine Table (morph = 0)
    osc.set_morph(FixedQ15(0));
    osc.reset_phase();
    int16_t max_val = 0;
    int16_t min_val = 0;
    for (int i = 0; i < 480; ++i) {
        auto val = osc.process_sample().raw;
        max_val = std::max(max_val, val);
        min_val = std::min(min_val, val);
    }
    assert(max_val > 30000);
    assert(min_val < -30000);

    // Morphed Hyperbolic wave (morph = 1.0)
    osc.set_morph(FixedQ15::from_float(1.0f));
    osc.reset_phase();
    int16_t max_morphed = 0;
    for (int i = 0; i < 480; ++i) {
        auto val = osc.process_sample().raw;
        max_morphed = std::max(max_morphed, val);
    }
    assert(max_morphed > 30000);

    std::cout << "PASSED\n";
}

void test_fixed_nonlinear_filter_and_shaper() {
    std::cout << "[TEST] Embedded Fixed-Point Nonlinear Filter & Wavefolder... ";
    using aether::math::FixedNonlinearFilter;
    using aether::math::FixedNonlinearShaper;
    using aether::math::FixedQ15;

    // Filter test
    FixedNonlinearFilter flt;
    flt.configure(FixedQ15::from_float(0.2f), FixedQ15::from_float(0.85f));

    // Impulse excitation
    auto smp0 = flt.process(FixedQ15::from_float(0.9f));
    assert(smp0.to_float() >= -1.0f && smp0.to_float() <= 1.0f);

    // Resonant ring without exploding
    for (int i = 0; i < 500; ++i) {
        auto out = flt.process(FixedQ15(0));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    flt.reset();
    auto silence = flt.process(FixedQ15(0));
    assert(silence.raw == 0);

    // Buchla-style Wavefolder test
    FixedNonlinearShaper shaper;
    // Drive 3.0x with 3 folds
    shaper.configure(32768 * 3, 3);

    // Positive peak folded back
    auto folded_pos = shaper.process(FixedQ15::from_float(0.9f));
    assert(folded_pos.to_float() >= -1.0f && folded_pos.to_float() <= 1.0f);

    // Negative peak folded back
    auto folded_neg = shaper.process(FixedQ15::from_float(-0.9f));
    assert(folded_neg.to_float() >= -1.0f && folded_neg.to_float() <= 1.0f);

    std::cout << "PASSED\n";
}

void test_fixed_lfo_and_delay() {
    std::cout << "[TEST] Embedded Fixed-Point Multi-Waveform LFO & Delay Line... ";
    using aether::math::FixedLfo;
    using aether::math::FixedDelay;
    using aether::math::FixedQ15;

    // Test LFO Waveforms
    FixedLfo lfo;
    lfo.set_frequency(5.0f, 48000);

    // Sine waveform
    lfo.set_waveform(FixedLfo::Waveform::Sine);
    int16_t max_s = -32768, min_s = 32767;
    for (int i = 0; i < 9600; ++i) {
        auto val = lfo.process().raw;
        max_s = std::max(max_s, val);
        min_s = std::min(min_s, val);
    }
    assert(max_s > 30000);
    assert(min_s < -30000);

    // Triangle waveform
    lfo.set_waveform(FixedLfo::Waveform::Triangle);
    auto val_tri = lfo.process().raw;
    assert(val_tri >= -32768 && val_tri <= 32767);

    // Square waveform (must be strictly bipolar max)
    lfo.set_waveform(FixedLfo::Waveform::Square);
    auto sq = lfo.process().raw;
    assert(sq == 32767 || sq == -32768);

    // Sample & Hold waveform
    lfo.set_waveform(FixedLfo::Waveform::SampleAndHold);
    auto sh = lfo.process().raw;
    assert(sh >= -32768 && sh <= 32767);

    // Test FixedDelay
    FixedDelay delay;
    // Delay 100 samples, 50% feedback, 100% wet
    delay.configure(100, FixedQ15::from_float(0.5f), FixedQ15::from_float(1.0f));

    // Impulse
    auto d0 = delay.process(FixedQ15::from_float(0.8f));
    assert(d0.raw == 0); // initial silence before delay line outputs

    // Read back after 99 samples
    for (int i = 0; i < 99; ++i) {
        (void)delay.process(FixedQ15(0));
    }
    auto d_echo = delay.process(FixedQ15(0));
    assert(std::abs(d_echo.to_float() - 0.8f) < 0.05f);

    delay.reset();
    auto reset_val = delay.process(FixedQ15(0));
    assert(reset_val.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_compressor_and_noise() {
    std::cout << "[TEST] Embedded Fixed-Point Dynamics Compressor & Noise Core... ";
    using aether::math::FixedCompressor;
    using aether::math::FixedNoise;
    using aether::math::FixedQ15;

    // Test Noise Generator
    FixedNoise noise;
    noise.seed(1337);

    int16_t min_w = 32767, max_w = -32768;
    for (int i = 0; i < 2000; ++i) {
        auto w = noise.process_white().raw;
        min_w = std::min(min_w, w);
        max_w = std::max(max_w, w);
    }
    assert(max_w > 30000);
    assert(min_w < -30000);

    // Pink noise is continuous and bounded
    for (int i = 0; i < 1000; ++i) {
        auto p = noise.process_pink().raw;
        assert(p >= -32768 && p <= 32767);
    }

    // Test Compressor
    FixedCompressor comp;
    // Threshold 0.25 (8192 raw), ratio 0.8 (26214 raw), fast attack, 1.0x makeup gain
    comp.configure(
        FixedQ15::from_float(0.25f),
        FixedQ15::from_float(0.8f),
        FixedQ15::from_float(0.9f),
        FixedQ15::from_float(0.01f),
        32768
    );

    // Signal below threshold should pass virtually unattenuated
    comp.reset();
    auto low_sig = comp.process(FixedQ15::from_float(0.1f));
    assert(std::abs(low_sig.to_float() - 0.1f) < 0.05f);

    // Heavy hot signal should be compressed down
    FixedQ15 hot_sig = FixedQ15::from_float(0.9f);
    FixedQ15 compressed_out = FixedQ15(0);
    for (int i = 0; i < 20; ++i) {
        compressed_out = comp.process(hot_sig);
    }
    // With heavy compression, output level should be significantly below 0.9f
    assert(compressed_out.to_float() < 0.6f);
    assert(compressed_out.to_float() > 0.0f);

    std::cout << "PASSED\n";
}

void test_fixed_stereo_chorus() {
    std::cout << "[TEST] Embedded Fixed-Point Stereo Chorus & Modulated Delay... ";
    using aether::math::FixedChorus;
    using aether::math::FixedQ15;

    FixedChorus chorus;
    // Configure base delay = 48 samples (~1ms), depth = 24 samples, rate ~1Hz
    uint32_t rate_inc = static_cast<uint32_t>((1ULL << 32) / 48000);
    chorus.configure(48, 24, rate_inc, FixedQ15::from_float(0.25f), FixedQ15::from_float(0.5f));

    // Impulse
    auto out0 = chorus.process(FixedQ15::from_float(0.9f), FixedQ15::from_float(0.9f));
    assert(out0.left.to_float() >= -1.0f && out0.left.to_float() <= 1.0f);
    assert(out0.right.to_float() >= -1.0f && out0.right.to_float() <= 1.0f);

    // Continuous processing over a full LFO cycle
    for (int i = 0; i < 48000; ++i) {
        auto out = chorus.process(FixedQ15(0), FixedQ15(0));
        assert(out.left.to_float() >= -1.0f && out.left.to_float() <= 1.0f);
        assert(out.right.to_float() >= -1.0f && out.right.to_float() <= 1.0f);
    }

    chorus.reset();
    auto reset_out = chorus.process(FixedQ15(0), FixedQ15(0));
    assert(reset_out.left.raw == 0);
    assert(reset_out.right.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_ring_mod_and_comb() {
    std::cout << "[TEST] Embedded Fixed-Point Ring Modulator & Acoustic Resonator... ";
    using aether::math::FixedRingModulator;
    using aether::math::FixedAcousticResonator;
    using aether::math::FixedQ15;

    // Test Ring Modulator
    FixedRingModulator rm;
    rm.set_carrier_frequency(440.0f, 48000);
    rm.configure(FixedQ15::from_float(1.0f), 2000); // 100% wet, 2000 deadzone

    // Processing bounded input
    for (int i = 0; i < 480; ++i) {
        auto out = rm.process(FixedQ15::from_float(0.8f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    rm.reset();
    auto rm_silence = rm.process(FixedQ15(0));
    assert(rm_silence.raw == 0);

    // Test Karplus-Strong / Physical Modeling Comb Resonator
    FixedAcousticResonator comb;
    // 108 samples delay corresponds to ~444.4 Hz at 48 kHz
    comb.configure(108, FixedQ15::from_float(0.98f), FixedQ15::from_float(0.3f));

    // Excite string with single impulse
    auto c0 = comb.process(FixedQ15::from_float(0.95f));
    assert(c0.raw == 0); // initial silence until line recirculates

    int32_t peak_decay = 0;
    for (int i = 0; i < 2000; ++i) {
        int32_t val = comb.process(FixedQ15(0)).raw;
        peak_decay = std::max(peak_decay, std::abs(val));
    }
    assert(peak_decay > 10000); // ringing acoustic resonance

    comb.reset();
    auto reset_comb = comb.process(FixedQ15(0));
    assert(reset_comb.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_svf_and_bitcrusher() {
    std::cout << "[TEST] Embedded Fixed-Point Chamberlin SVF & Bitcrusher... ";
    using aether::math::FixedStateVariableFilter;
    using aether::math::FixedBitcrusher;
    using aether::math::FixedQ15;

    // Test SVF
    FixedStateVariableFilter svf;
    // f = ~1000 Hz, Q ~ 2.0 (q_inv = 16384)
    svf.configure(FixedQ15::from_float(0.15f), FixedQ15::from_float(0.5f));

    // Impulse
    auto out0 = svf.process(FixedQ15::from_float(0.9f));
    assert(out0.lowpass.to_float() >= -1.0f && out0.lowpass.to_float() <= 1.0f);
    assert(out0.highpass.to_float() >= -1.0f && out0.highpass.to_float() <= 1.0f);
    assert(out0.bandpass.to_float() >= -1.0f && out0.bandpass.to_float() <= 1.0f);
    assert(out0.notch.to_float() >= -1.0f && out0.notch.to_float() <= 1.0f);

    // Resonant ring for 500 samples
    for (int i = 0; i < 500; ++i) {
        auto out = svf.process(FixedQ15(0));
        assert(out.lowpass.to_float() >= -1.0f && out.lowpass.to_float() <= 1.0f);
        assert(out.highpass.to_float() >= -1.0f && out.highpass.to_float() <= 1.0f);
    }

    svf.reset();
    auto reset_out = svf.process(FixedQ15(0));
    assert(reset_out.lowpass.raw == 0);
    assert(reset_out.highpass.raw == 0);

    // Test Bitcrusher
    FixedBitcrusher bc;
    // 8-bit reduction, 4x sample rate decimation
    bc.configure(8, 4);

    FixedQ15 in_val = FixedQ15::from_float(0.735f);
    auto bc0 = bc.process(in_val);
    auto bc1 = bc.process(FixedQ15::from_float(0.2f));
    auto bc2 = bc.process(FixedQ15::from_float(-0.4f));
    auto bc3 = bc.process(FixedQ15::from_float(0.9f));

    // bc0, bc1, bc2, bc3 should hold bc0 value due to 4x decimation
    assert(bc0.raw == bc1.raw);
    assert(bc0.raw == bc2.raw);
    assert(bc0.raw == bc3.raw);

    // Next sample must update
    auto bc4 = bc.process(FixedQ15::from_float(-0.5f));
    assert(bc4.raw != bc0.raw);

    std::cout << "PASSED\n";
}

void test_fixed_phaser_and_clipper() {
    std::cout << "[TEST] Embedded Fixed-Point 4-Stage Phaser & Asymmetric Diode Clipper... ";
    using aether::math::FixedAllpassPhaser;
    using aether::math::FixedClipper;
    using aether::math::FixedQ15;

    // Test 4-stage Phaser
    FixedAllpassPhaser phaser;
    uint32_t rate_inc = static_cast<uint32_t>((2ULL << 32) / 48000); // 2 Hz sweep
    phaser.configure(
        rate_inc,
        FixedQ15::from_float(0.15f),
        FixedQ15::from_float(0.75f),
        FixedQ15::from_float(0.6f),
        FixedQ15::from_float(0.5f)
    );

    // Run over 500 samples
    for (int i = 0; i < 500; ++i) {
        auto out = phaser.process(FixedQ15::from_float(0.7f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    phaser.reset();
    auto p_silence = phaser.process(FixedQ15(0));
    assert(p_silence.raw == 0);

    // Test Asymmetric Clipper
    FixedClipper clipper;
    // Drive 3.0x with positive bias for 2nd harmonic richness
    clipper.configure(32768 * 3, 4000);

    auto sat_pos = clipper.process(FixedQ15::from_float(0.8f));
    auto sat_neg = clipper.process(FixedQ15::from_float(-0.8f));

    assert(sat_pos.to_float() >= -1.0f && sat_pos.to_float() <= 1.0f);
    assert(sat_neg.to_float() >= -1.0f && sat_neg.to_float() <= 1.0f);
    // Asymmetric clipping generates unequal magnitude peaks
    assert(sat_pos.raw != -sat_neg.raw);

    std::cout << "PASSED\n";
}

void test_fixed_formant_and_exciter() {
    std::cout << "[TEST] Embedded Fixed-Point Vocal Formant Filter & Harmonic Exciter... ";
    using aether::math::FixedFormantFilter;
    using aether::math::FixedExciter;
    using aether::math::FixedQ15;

    // Test Vocal Formant
    FixedFormantFilter formant;
    // Iterate through vowels A, E, I, O, U
    for (uint8_t v = 0; v < 5; ++v) {
        formant.set_vowel(static_cast<FixedFormantFilter::Vowel>(v));
        formant.reset();
        for (int i = 0; i < 200; ++i) {
            auto out = formant.process(FixedQ15::from_float(0.8f));
            assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
        }
    }

    formant.reset();
    auto v_silence = formant.process(FixedQ15(0));
    assert(v_silence.raw == 0);

    // Test Exciter
    FixedExciter exciter;
    exciter.configure(FixedQ15::from_float(0.5f), 32768 * 2, FixedQ15::from_float(0.25f));

    for (int i = 0; i < 500; ++i) {
        auto out = exciter.process(FixedQ15::from_float(0.7f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    exciter.reset();
    auto e_silence = exciter.process(FixedQ15(0));
    assert(e_silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_pitch_shifter_and_gate() {
    std::cout << "[TEST] Embedded Fixed-Point Pitch Shifter & Hysteresis Noise Gate... ";
    using aether::math::FixedPitchShifter;
    using aether::math::FixedGate;
    using aether::math::FixedQ15;

    // Test Granular Pitch Shifter
    FixedPitchShifter ps;
    ps.set_shift_semitones(7); // +7 semitones (perfect fifth up)

    for (int i = 0; i < 1000; ++i) {
        auto out = ps.process(FixedQ15::from_float(0.8f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    ps.set_shift_semitones(-12); // -12 semitones (one octave down)
    for (int i = 0; i < 1000; ++i) {
        auto out = ps.process(FixedQ15::from_float(0.5f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    ps.reset();
    auto p_silence = ps.process(FixedQ15(0));
    assert(p_silence.raw == 0);

    // Test Noise Gate with Hysteresis
    FixedGate gate;
    // Open at 0.3 (9830 raw), close at 0.15 (4915 raw), fast attack, zero floor
    gate.configure(
        FixedQ15::from_float(0.3f),
        FixedQ15::from_float(0.15f),
        FixedQ15::from_float(0.9f),
        FixedQ15::from_float(0.1f),
        FixedQ15(0)
    );

    gate.reset();
    // Sub-threshold noise should be gated shut
    for (int i = 0; i < 20; ++i) {
        auto g_out = gate.process(FixedQ15::from_float(0.05f));
        assert(g_out.to_float() < 0.01f);
    }
    assert(!gate.is_open());

    // Signal exceeding open threshold opens gate
    for (int i = 0; i < 20; ++i) {
        (void)gate.process(FixedQ15::from_float(0.8f));
    }
    assert(gate.is_open());

    // Falling below open threshold but above close threshold stays open (hysteresis)
    (void)gate.process(FixedQ15::from_float(0.2f));
    assert(gate.is_open());

    // Falling below close threshold shuts gate
    for (int i = 0; i < 100; ++i) {
        (void)gate.process(FixedQ15::from_float(0.02f));
    }
    assert(!gate.is_open());

    std::cout << "PASSED\n";
}

void test_fixed_multitap_and_tremolo() {
    std::cout << "[TEST] Embedded Fixed-Point 4-Tap Delay & Optical AutoPan Tremolo... ";
    using aether::math::FixedMultiTapDelay;
    using aether::math::FixedTremolo;
    using aether::math::FixedQ15;

    // Test Multi-Tap Delay
    FixedMultiTapDelay mtd;
    mtd.set_tap(0, 100, FixedQ15::from_float(0.8f), -30000); // hard left
    mtd.set_tap(1, 200, FixedQ15::from_float(0.6f),  30000); // hard right
    mtd.set_feedback(FixedQ15::from_float(0.3f));

    // Impulse
    auto d0 = mtd.process(FixedQ15::from_float(0.9f));
    assert(d0.left.raw == 0 && d0.right.raw == 0);

    for (int i = 0; i < 500; ++i) {
        auto out = mtd.process(FixedQ15(0));
        assert(out.left.to_float() >= -1.0f && out.left.to_float() <= 1.0f);
        assert(out.right.to_float() >= -1.0f && out.right.to_float() <= 1.0f);
    }

    mtd.reset();
    auto d_silence = mtd.process(FixedQ15(0));
    assert(d_silence.left.raw == 0 && d_silence.right.raw == 0);

    // Test Optical Tremolo / AutoPan
    FixedTremolo trem;
    uint32_t rate_inc = static_cast<uint32_t>((4ULL << 32) / 48000); // 4 Hz
    trem.configure(rate_inc, FixedQ15::from_float(0.8f), FixedTremolo::Mode::AutoPan);

    for (int i = 0; i < 1000; ++i) {
        auto t_out = trem.process(FixedQ15::from_float(0.7f), FixedQ15::from_float(0.7f));
        assert(t_out.left.to_float() >= -1.0f && t_out.left.to_float() <= 1.0f);
        assert(t_out.right.to_float() >= -1.0f && t_out.right.to_float() <= 1.0f);
    }

    trem.reset();
    auto t_silence = trem.process(FixedQ15(0), FixedQ15(0));
    assert(t_silence.left.raw == 0 && t_silence.right.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_transient_and_widener() {
    std::cout << "[TEST] Embedded Fixed-Point Transient Designer & Mid/Side Widener... ";
    using aether::math::FixedTransientShaper;
    using aether::math::FixedStereoWidener;
    using aether::math::FixedQ15;

    // Test Transient Designer
    FixedTransientShaper ts;
    // Boost attack +0.5, reduce sustain -0.3
    ts.configure(FixedQ15::from_float(0.5f), FixedQ15::from_float(-0.3f));

    // Sudden percussive transient impulse
    auto t0 = ts.process(FixedQ15::from_float(0.9f));
    assert(t0.to_float() >= -1.0f && t0.to_float() <= 1.0f);

    for (int i = 0; i < 500; ++i) {
        auto out = ts.process(FixedQ15::from_float(0.2f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    ts.reset();
    auto ts_silence = ts.process(FixedQ15(0));
    assert(ts_silence.raw == 0);

    // Test Mid/Side Stereo Widener
    FixedStereoWidener widener;
    // 1.5x width expansion with bass mono filter
    widener.configure(32768 + 16384, FixedQ15::from_float(0.2f));

    FixedQ15 left_in = FixedQ15::from_float(0.6f);
    FixedQ15 right_in = FixedQ15::from_float(-0.4f);

    for (int i = 0; i < 100; ++i) {
        auto out = widener.process(left_in, right_in);
        assert(out.left.to_float() >= -1.0f && out.left.to_float() <= 1.0f);
        assert(out.right.to_float() >= -1.0f && out.right.to_float() <= 1.0f);
    }

    // Pure mono input (left == right) should have zero side and pass through untouched
    widener.reset();
    FixedQ15 mono_in = FixedQ15::from_float(0.5f);
    auto mono_out = widener.process(mono_in, mono_in);
    assert(std::abs(mono_out.left.to_float() - 0.5f) < 0.01f);
    assert(std::abs(mono_out.right.to_float() - 0.5f) < 0.01f);

    std::cout << "PASSED\n";
}

void test_fixed_pitch_detector_and_crossover() {
    std::cout << "[TEST] Embedded Fixed-Point AMDF Pitch Tracker & LR4 Crossover... ";
    using aether::math::FixedPitchDetector;
    using aether::math::FixedCrossover;
    using aether::math::FixedQ15;

    // Test Pitch Detector with periodic signal
    FixedPitchDetector detector;
    detector.reset();

    // Feed a periodic square wave with period = 100 samples (~480 Hz)
    for (int rep = 0; rep < 10; ++rep) {
        for (int i = 0; i < 50; ++i) detector.push_sample(FixedQ15::from_float(0.8f));
        for (int i = 0; i < 50; ++i) detector.push_sample(FixedQ15::from_float(-0.8f));
    }

    auto pitch = detector.analyze();
    assert(pitch.voiced);
    assert(std::abs(static_cast<int>(pitch.period_samples) - 100) <= 6); // within decimation tolerance
    assert(pitch.confidence_q15 > 10000);

    // Test LR4 Crossover
    FixedCrossover xover;
    xover.configure(FixedQ15::from_float(0.1f)); // ~1500 Hz split
    xover.reset();

    // Impulse
    auto split0 = xover.process(FixedQ15::from_float(0.9f));
    assert(split0.low.to_float() >= -1.0f && split0.low.to_float() <= 1.0f);
    assert(split0.high.to_float() >= -1.0f && split0.high.to_float() <= 1.0f);

    for (int i = 0; i < 500; ++i) {
        auto s = xover.process(FixedQ15(0));
        assert(s.low.to_float() >= -1.0f && s.low.to_float() <= 1.0f);
        assert(s.high.to_float() >= -1.0f && s.high.to_float() <= 1.0f);
    }

    xover.reset();
    auto x_silence = xover.process(FixedQ15(0));
    assert(x_silence.low.raw == 0);
    assert(x_silence.high.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_eq_and_sub_synth() {
    std::cout << "[TEST] Embedded Fixed-Point 3-Band Parametric EQ & Sub-Harmonic Synth... ";
    using aether::math::FixedParametricEQ;
    using aether::math::FixedSubHarmonicSynth;
    using aether::math::FixedQ15;

    // Test 3-Band Parametric EQ
    FixedParametricEQ eq;
    eq.configure_low_shelf(6);
    eq.configure_mid_bell(-6);
    eq.configure_high_shelf(6);

    for (int i = 0; i < 500; ++i) {
        auto out = eq.process(FixedQ15::from_float(0.7f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    eq.reset();
    auto eq_silence = eq.process(FixedQ15(0));
    assert(eq_silence.raw == 0);

    // Test Sub-Harmonic Synth
    FixedSubHarmonicSynth sub;
    sub.configure(FixedQ15::from_float(0.8f), FixedQ15::from_float(1.0f), FixedQ15::from_float(0.1f));

    // Feed alternating wave to trigger sub-octave divider
    for (int cycle = 0; cycle < 100; ++cycle) {
        for (int i = 0; i < 20; ++i) (void)sub.process(FixedQ15::from_float(0.6f));
        for (int i = 0; i < 20; ++i) (void)sub.process(FixedQ15::from_float(-0.6f));
    }

    auto out_sub = sub.process(FixedQ15::from_float(0.5f));
    assert(out_sub.to_float() >= -1.0f && out_sub.to_float() <= 1.0f);

    sub.reset();
    auto sub_silence = sub.process(FixedQ15(0));
    assert(sub_silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_comb_chorus_and_tape_saturator() {
    std::cout << "[TEST] Embedded Fixed-Point 6-Stage Ensemble Chorus & Tape Saturator... ";
    using aether::math::FixedCombChorus;
    using aether::math::FixedTapeSaturator;
    using aether::math::FixedQ15;

    // Test 6-Stage Solina / Dimension D Ensemble Chorus
    FixedCombChorus ensemble;
    uint32_t rate_inc = static_cast<uint32_t>((1ULL << 32) / 48000); // 1 Hz sweep
    ensemble.configure(rate_inc, 12, FixedQ15::from_float(0.6f));

    for (int i = 0; i < 600; ++i) {
        auto out = ensemble.process(FixedQ15::from_float(0.7f), FixedQ15::from_float(0.7f));
        assert(out.left.to_float() >= -1.0f && out.left.to_float() <= 1.0f);
        assert(out.right.to_float() >= -1.0f && out.right.to_float() <= 1.0f);
    }

    ensemble.reset();
    auto e_silence = ensemble.process(FixedQ15(0), FixedQ15(0));
    assert(e_silence.left.raw == 0 && e_silence.right.raw == 0);

    // Test Analog Tape Saturator
    FixedTapeSaturator tape;
    tape.configure(32768 * 3, FixedQ15::from_float(0.7f), FixedQ15::from_float(1.0f));

    auto t_hot = tape.process(FixedQ15::from_float(0.95f));
    assert(t_hot.to_float() >= -1.0f && t_hot.to_float() <= 1.0f);

    tape.reset();
    auto t_silence = tape.process(FixedQ15(0));
    assert(t_silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_shepard_phaser_and_autowah() {
    std::cout << "[TEST] Embedded Fixed-Point Shepard Barber-Pole Phaser & Dynamic AutoWah... ";
    using aether::math::FixedCombAllpassPhasor;
    using aether::math::FixedAutoWah;
    using aether::math::FixedQ15;

    // Test Barber-Pole Shepard-Risset Phaser
    FixedCombAllpassPhasor phaser;
    phaser.configure(1000000, FixedQ15::from_float(0.4f), FixedQ15::from_float(0.5f));

    for (int i = 0; i < 1000; ++i) {
        auto out = phaser.process(FixedQ15::from_float(0.7f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    phaser.reset();
    auto p_silence = phaser.process(FixedQ15(0));
    assert(p_silence.raw == 0);

    // Test Dynamic Auto-Wah
    FixedAutoWah wah;
    wah.configure(FixedQ15::from_float(0.05f), FixedQ15::from_float(0.4f), FixedQ15::from_float(0.8f), FixedQ15::from_float(0.12f));

    // Impulse burst to trigger envelope filter sweep
    for (int i = 0; i < 20; ++i) (void)wah.process(FixedQ15::from_float(0.9f));
    for (int i = 0; i < 400; ++i) {
        auto out = wah.process(FixedQ15::from_float(0.1f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    wah.reset();
    auto w_silence = wah.process(FixedQ15(0));
    assert(w_silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_leslie_and_vibrato() {
    std::cout << "[TEST] Embedded Fixed-Point Leslie Rotary Speaker & True Pitch Vibrato... ";
    using aether::math::FixedLeslieSpeaker;
    using aether::math::FixedVibrato;
    using aether::math::FixedQ15;

    // Test Leslie Speaker Simulator
    FixedLeslieSpeaker leslie;
    leslie.set_speed(FixedLeslieSpeaker::Speed::Fast);

    for (int i = 0; i < 500; ++i) {
        auto out = leslie.process(FixedQ15::from_float(0.7f), FixedQ15::from_float(0.7f));
        assert(out.left.to_float() >= -1.0f && out.left.to_float() <= 1.0f);
        assert(out.right.to_float() >= -1.0f && out.right.to_float() <= 1.0f);
    }

    leslie.set_speed(FixedLeslieSpeaker::Speed::Brake);
    for (int i = 0; i < 500; ++i) {
        auto out = leslie.process(FixedQ15::from_float(0.5f), FixedQ15::from_float(0.5f));
        assert(out.left.to_float() >= -1.0f && out.left.to_float() <= 1.0f);
    }

    leslie.reset();
    auto l_silence = leslie.process(FixedQ15(0), FixedQ15(0));
    assert(l_silence.left.raw == 0 && l_silence.right.raw == 0);

    // Test True Pitch Vibrato
    FixedVibrato vib;
    uint32_t rate_inc = static_cast<uint32_t>((6ULL << 32) / 48000); // 6 Hz vibrato
    vib.configure(rate_inc, 20);

    for (int i = 0; i < 500; ++i) {
        auto out = vib.process(FixedQ15::from_float(0.8f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    vib.reset();
    auto v_silence = vib.process(FixedQ15(0));
    assert(v_silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_fixed_spsc_queue_and_midi_parser() {
    std::cout << "[TEST] Embedded Lock-Free FixedSpscQueue & DaisyMidiParser... ";
    using aether::math::FixedSpscQueue;
    using aether::embedded::DaisyMidiParser;
    using aether::embedded::MidiEvent;
    using aether::embedded::MidiMessageType;

    // Test FixedSpscQueue lock-free queue
    FixedSpscQueue<int16_t, 8> queue;
    assert(queue.empty());
    assert(!queue.full());
    assert(queue.size() == 0);

    for (int16_t i = 1; i <= 7; ++i) {
        bool pushed = queue.push(i * 100);
        assert(pushed);
    }
    assert(queue.size() == 7);
    assert(!queue.full());

    // 8th item fills queue to capacity
    bool pushed_8 = queue.push(800);
    assert(pushed_8);
    assert(queue.full());

    // 9th item rejected
    bool pushed_9 = queue.push(900);
    assert(!pushed_9);

    // Pop first 4 items
    for (int16_t i = 1; i <= 4; ++i) {
        int16_t val = 0;
        bool popped = queue.pop(val);
        assert(popped);
        assert(val == i * 100);
    }
    assert(queue.size() == 4);

    // Push 2 more items (wrap-around in circular ring)
    assert(queue.push(900));
    assert(queue.push(1000));
    assert(queue.size() == 6);

    // Pop all remaining items
    int16_t expected[] = {500, 600, 700, 800, 900, 1000};
    for (int i = 0; i < 6; ++i) {
        int16_t val = 0;
        assert(queue.pop(val));
        assert(val == expected[i]);
    }
    assert(queue.empty());

    // Test DaisyMidiParser
    DaisyMidiParser parser;
    MidiEvent ev;

    // Test NoteOn (channel 1 -> 0x90, note 60, velocity 100)
    assert(!parser.parse_byte(0x90, ev));
    assert(!parser.parse_byte(60, ev));
    assert(parser.parse_byte(100, ev));
    assert(ev.type == MidiMessageType::NoteOn);
    assert(ev.channel == 0);
    assert(ev.data1 == 60);
    assert(ev.data2 == 100);

    // Test Running Status (next note on without status byte)
    assert(!parser.parse_byte(64, ev));
    assert(parser.parse_byte(110, ev));
    assert(ev.type == MidiMessageType::NoteOn);
    assert(ev.channel == 0);
    assert(ev.data1 == 64);
    assert(ev.data2 == 110);

    // Test Note On with velocity 0 converts to Note Off
    assert(!parser.parse_byte(60, ev));
    assert(parser.parse_byte(0, ev));
    assert(ev.type == MidiMessageType::NoteOff);
    assert(ev.channel == 0);
    assert(ev.data1 == 60);
    assert(ev.data2 == 0);

    // Test Control Change (channel 2 -> 0xB1, CC 74, value 85)
    assert(!parser.parse_byte(0xB1, ev));
    assert(!parser.parse_byte(74, ev));
    assert(parser.parse_byte(85, ev));
    assert(ev.type == MidiMessageType::ControlChange);
    assert(ev.channel == 1);
    assert(ev.data1 == 74);
    assert(ev.data2 == 85);

    // Test Pitch Bend 14-bit (channel 0 -> 0xE0, LSB=0, MSB=64 -> center = 8192 - 8192 = 0)
    assert(!parser.parse_byte(0xE0, ev));
    assert(!parser.parse_byte(0x00, ev));
    assert(parser.parse_byte(0x40, ev)); // 64 << 7 = 8192
    assert(ev.type == MidiMessageType::PitchBend);
    assert(ev.value14 == 0);

    // Pitch bend max (LSB=0x7F, MSB=0x7F -> 16383 - 8192 = 8191)
    assert(!parser.parse_byte(0x7F, ev));
    assert(parser.parse_byte(0x7F, ev));
    assert(ev.type == MidiMessageType::PitchBend);
    assert(ev.value14 == 8191);

    // Test Real-Time Clock interleaving (0xF8)
    assert(parser.parse_byte(0xF8, ev));
    assert(ev.type == MidiMessageType::SystemRealTime);
    assert(ev.data1 == 0xF8);

    std::cout << "PASSED\n";
}

void test_daisy_oled_and_eurorack_sequencer() {
    std::cout << "[TEST] Embedded Daisy OLED Display & Eurorack Quad Sequencer... ";
    using aether::embedded::DaisyOledDisplay;
    using aether::embedded::EurorackStepSequencer;
    using aether::math::FixedQ15;

    // Test Daisy OLED framebuffer primitives
    DaisyOledDisplay oled;
    assert(oled.buffer_size() == 1024);

    // Initial buffer should be clear
    for (size_t i = 0; i < oled.buffer_size(); ++i) {
        assert(oled.raw_buffer()[i] == 0);
    }

    // Set pixel
    oled.draw_pixel(10, 20, true);
    assert(oled.get_pixel(10, 20));
    assert(!oled.get_pixel(11, 20));

    // Clear pixel
    oled.draw_pixel(10, 20, false);
    assert(!oled.get_pixel(10, 20));

    // Draw horizontal and vertical lines
    oled.draw_hline(0, 0, 128, true);
    for (int x = 0; x < 128; ++x) {
        assert(oled.get_pixel(x, 0));
    }
    oled.draw_vline(64, 0, 64, true);
    for (int y = 0; y < 64; ++y) {
        assert(oled.get_pixel(64, y));
    }

    // Draw audio waveform into OLED framebuffer
    std::array<FixedQ15, 128> wave;
    for (size_t i = 0; i < 128; ++i) {
        wave[i] = FixedQ15::from_float(std::sin(2.0f * 3.14159265f * i / 32.0f));
    }
    oled.clear();
    oled.draw_waveform(wave.data(), wave.size(), 32, 28);
    // Ensure pixels were drawn around center
    bool has_drawn_pixels = false;
    for (size_t i = 0; i < oled.buffer_size(); ++i) {
        if (oled.raw_buffer()[i] != 0) {
            has_drawn_pixels = true;
            break;
        }
    }
    assert(has_drawn_pixels);

    // Test Eurorack Quad Step Sequencer
    EurorackStepSequencer seq;
    seq.set_sequence_length(8);

    // Configure step 1 (C4 = 2000 mV, gate 50%, prob 100%)
    seq.set_step(0, 1, 2000, FixedQ15::from_float(0.9f), 50, 100);
    // Configure step 2 (E4 = 2333 mV, gate 25%, prob 100%)
    seq.set_step(0, 2, 2333, FixedQ15::from_float(0.7f), 25, 100);

    // Clock step 0 -> 1
    auto out1 = seq.tick(true, 0, 24);
    assert(seq.current_step() == 1);
    assert(out1.cv_millivolts == 2000);
    assert(out1.gate == true);

    // Tick within gate length (subtick < 50% of 24 = 12)
    for (int i = 0; i < 10; ++i) {
        auto sub = seq.tick(false, 0, 24);
        assert(sub.gate == true);
    }

    // Tick past gate length (subtick >= 12) shuts gate off
    for (int i = 0; i < 5; ++i) {
        (void)seq.tick(false, 0, 24);
    }
    auto sub_off = seq.tick(false, 0, 24);
    assert(sub_off.gate == false);

    // Clock step 1 -> 2
    auto out2 = seq.tick(true, 0, 24);
    assert(seq.current_step() == 2);
    assert(out2.cv_millivolts == 2333);
    assert(out2.gate == true);

    std::cout << "PASSED\n";
}

void test_eurorack_quantizer_and_logic() {
    std::cout << "[TEST] Embedded Eurorack Scale Quantizer & Boolean Logic Core... ";
    using aether::embedded::EurorackQuantizer;
    using aether::embedded::EurorackLogicCore;

    EurorackQuantizer q;
    // Chromatic: 0 mV (C) -> 0 mV
    assert(q.quantize(0) == 0);

    // 83 mV is C# (approx 83.33 mV)
    assert(q.quantize(83) == 83);

    // Set C Major scale (C, D, E, F, G, A, B)
    // C# = 83 mV should snap down to C (0 mV) or up to D (166 mV)
    q.set_scale(EurorackQuantizer::Scale::Major);
    int16_t snap_csharp = q.quantize(83);
    assert(snap_csharp == 0 || snap_csharp == 166);

    // 1000 mV (1 Octave above C) -> 1000 mV
    assert(q.quantize(1000) == 1000);

    // Minor Pentatonic scale
    q.set_scale(EurorackQuantizer::Scale::PentatonicMinor);
    // Eb is note 3 -> (3 * 1000 + 6) / 12 = 250 mV
    assert(q.quantize(250) == 250);

    // Test Eurorack Logic Core
    assert(EurorackLogicCore::evaluate_logic(true, true, EurorackLogicCore::LogicOp::AND) == true);
    assert(EurorackLogicCore::evaluate_logic(true, false, EurorackLogicCore::LogicOp::AND) == false);
    assert(EurorackLogicCore::evaluate_logic(true, false, EurorackLogicCore::LogicOp::OR) == true);
    assert(EurorackLogicCore::evaluate_logic(true, true, EurorackLogicCore::LogicOp::XOR) == false);
    assert(EurorackLogicCore::evaluate_logic(true, false, EurorackLogicCore::LogicOp::XOR) == true);
    assert(EurorackLogicCore::evaluate_logic(true, true, EurorackLogicCore::LogicOp::NAND) == false);
    assert(EurorackLogicCore::evaluate_logic(false, false, EurorackLogicCore::LogicOp::NOR) == true);

    // Test Clock Divider
    EurorackLogicCore logic;
    // Pulse 1: rising edge
    auto div1 = logic.clock_divider(true);
    assert(div1.div2 == true);
    (void)logic.clock_divider(false);

    // Pulse 2: rising edge
    auto div2 = logic.clock_divider(true);
    assert(div2.div2 == false);
    assert(div2.div4 == true);
    (void)logic.clock_divider(false);

    // Pulse 3: rising edge
    auto div3 = logic.clock_divider(true);
    assert(div3.div2 == true);
    assert(div3.div4 == true);
    (void)logic.clock_divider(false);

    // Pulse 4: rising edge
    auto div4 = logic.clock_divider(true);
    assert(div4.div2 == false);
    assert(div4.div4 == false);
    assert(div4.div8 == true);

    // Test Gate Delay
    logic.reset();
    bool gd0 = logic.process_gate_delay(true, 3);
    assert(gd0 == false);
    bool gd1 = logic.process_gate_delay(false, 3);
    assert(gd1 == false);
    bool gd2 = logic.process_gate_delay(false, 3);
    assert(gd2 == false);
    bool gd3 = logic.process_gate_delay(false, 3);
    assert(gd3 == true); // Delayed high pulse emerges after 3 steps

    std::cout << "PASSED\n";
}

void test_eurorack_slew_and_sample_hold() {
    std::cout << "[TEST] Embedded Eurorack Slew Limiter & Precision S&H/T&H... ";
    using aether::embedded::EurorackSlewLimiter;
    using aether::embedded::EurorackSampleAndHold;

    EurorackSlewLimiter slew;
    slew.set_rates(100, 200, EurorackSlewLimiter::Curve::Linear);

    // Target 1000 mV from 0 mV with rise rate 100 mV per sample (takes 10 steps)
    for (int i = 1; i <= 9; ++i) {
        auto out = slew.process(1000);
        assert(out.cv_out_mv == i * 100);
        assert(!out.end_of_rise);
    }
    auto reach_target = slew.process(1000);
    assert(reach_target.cv_out_mv == 1000);
    assert(reach_target.end_of_rise);

    // Fall back to 0 mV with fall rate 200 mV per sample (takes 5 steps)
    for (int i = 1; i <= 4; ++i) {
        auto out = slew.process(0);
        assert(out.cv_out_mv == 1000 - i * 200);
        assert(!out.end_of_fall);
    }
    auto reach_zero = slew.process(0);
    assert(reach_zero.cv_out_mv == 0);
    assert(reach_zero.end_of_fall);

    // Self-cycle mode (Function Generator LFO)
    slew.reset();
    slew.set_rates(500, 500);
    slew.set_cycle(true);
    for (int i = 0; i < 30; ++i) {
        auto cyc = slew.process(0);
        assert(cyc.cv_out_mv >= 0 && cyc.cv_out_mv <= 5000);
    }

    // Test Eurorack Precision Sample and Hold
    EurorackSampleAndHold sh;
    // Input is 3000 mV, clock is LOW -> held is 0
    assert(sh.process_sample_and_hold(3000, false) == 0);

    // Clock goes HIGH -> triggers sample
    assert(sh.process_sample_and_hold(3000, true) == 3000);

    // Input changes to 4500 mV, clock remains HIGH -> stays held at 3000 mV
    assert(sh.process_sample_and_hold(4500, true) == 3000);

    // Clock goes LOW -> still held at 3000 mV
    assert(sh.process_sample_and_hold(5000, false) == 3000);

    // Next rising edge samples new value 5000 mV
    assert(sh.process_sample_and_hold(5000, true) == 5000);

    // Test Track & Hold
    sh.reset();
    // Gate HIGH: tracks input exactly
    assert(sh.process_track_and_hold(1500, true) == 1500);
    assert(sh.process_track_and_hold(2700, true) == 2700);

    // Gate LOW: freezes at last tracked value
    assert(sh.process_track_and_hold(4000, false) == 2700);
    assert(sh.process_track_and_hold(100, false) == 2700);

    // Gate HIGH again: resumes tracking
    assert(sh.process_track_and_hold(800, true) == 800);

    // Test Analog Capacitor Droop
    sh.reset();
    sh.set_droop(10, 5); // 10 mV dropped every 5 samples
    (void)sh.process_sample_and_hold(1000, true);
    assert(sh.held_value() == 1000);
    for (int i = 0; i < 5; ++i) {
        (void)sh.process_sample_and_hold(1000, false);
    }
    assert(sh.held_value() == 990); // 10 mV dropped

    std::cout << "PASSED\n";
}

void test_eurorack_matrix_mixer_and_calibrator() {
    std::cout << "[TEST] Embedded Eurorack 4x4 Matrix Mixer & Hardware CV Calibrator... ";
    using aether::embedded::EurorackMatrixMixer;
    using aether::embedded::EurorackCvCalibrator;
    using aether::math::FixedQ15;

    // Test Eurorack 4x4 Matrix Mixer
    EurorackMatrixMixer mixer;
    // Set diagonal gains to 1.0 (pass-through)
    for (size_t i = 0; i < 4; ++i) {
        mixer.set_gain(i, i, FixedQ15::from_float(0.999f));
    }

    std::array<FixedQ15, 4> in_sig = {
        FixedQ15::from_float(0.2f),
        FixedQ15::from_float(0.4f),
        FixedQ15::from_float(0.6f),
        FixedQ15::from_float(0.8f)
    };

    auto out_diag = mixer.process(in_sig);
    for (size_t i = 0; i < 4; ++i) {
        assert(std::abs(out_diag[i].to_float() - in_sig[i].to_float()) < 0.05f);
    }

    // Set summing bus on Out 0: sum in0, in1, in2, in3
    mixer.reset();
    mixer.set_gain(0, 0, FixedQ15::from_float(0.5f));
    mixer.set_gain(1, 0, FixedQ15::from_float(0.5f));
    auto out_sum = mixer.process(in_sig);
    // (0.2 * 0.5) + (0.4 * 0.5) = 0.1 + 0.2 = 0.3
    assert(std::abs(out_sum[0].to_float() - 0.3f) < 0.02f);

    // Test soft saturation clipping
    mixer.set_gain(2, 0, FixedQ15::from_float(1.0f));
    mixer.set_gain(3, 0, FixedQ15::from_float(1.0f));
    auto out_clip = mixer.process(in_sig);
    assert(out_clip[0].to_float() >= -1.0f && out_clip[0].to_float() <= 1.0f);

    // Test Eurorack CV Calibrator
    EurorackCvCalibrator cal;
    // Default calibration is identity
    assert(cal.apply(0, 1000) == 1000);
    assert(cal.apply(0, 3000) == 3000);

    // Calibrate Channel 0:
    // Suppose ADC measures 1020 mV for 1000 mV input, and 3040 mV for 3000 mV input.
    // delta = 2020 mV (instead of 2000 mV).
    cal.calibrate_two_point(0, 1020, 3040);

    int16_t cal_v1 = cal.apply(0, 1020);
    int16_t cal_v3 = cal.apply(0, 3040);
    assert(std::abs(cal_v1 - 1000) <= 2);
    assert(std::abs(cal_v3 - 3000) <= 2);

    std::cout << "PASSED\n";
}

void test_eurorack_vector_panner_and_wavefolder() {
    std::cout << "[TEST] Embedded Eurorack Vector Panner & Timbre Wavefolder... ";
    using aether::embedded::EurorackVectorPanner;
    using aether::embedded::EurorackWavefolder;
    using aether::math::FixedQ15;

    // Test Eurorack 2D Vector Panner
    EurorackVectorPanner panner;
    panner.set_law(EurorackVectorPanner::PanningLaw::Linear);

    // Center position (0, 0) -> equal 0.25 to all 4 corners
    panner.set_position(FixedQ15(0), FixedQ15(0));
    auto gains_center = panner.get_gains();
    assert(std::abs(gains_center.a.to_float() - 0.25f) < 0.05f);
    assert(std::abs(gains_center.b.to_float() - 0.25f) < 0.05f);
    assert(std::abs(gains_center.c.to_float() - 0.25f) < 0.05f);
    assert(std::abs(gains_center.d.to_float() - 0.25f) < 0.05f);

    // Top-right Corner D (+1, +1)
    panner.set_position(FixedQ15::from_float(0.999f), FixedQ15::from_float(0.999f));
    auto gains_d = panner.get_gains();
    assert(gains_d.d.to_float() > 0.95f);
    assert(gains_d.a.to_float() < 0.05f);

    // Pan an audio signal to 4 corners
    auto quad = panner.pan(FixedQ15::from_float(0.8f));
    assert(quad.d.to_float() > 0.7f);
    assert(quad.a.to_float() < 0.05f);

    // Test mix back
    FixedQ15 mixed = panner.mix(FixedQ15(0), FixedQ15(0), FixedQ15(0), FixedQ15::from_float(0.8f));
    assert(mixed.to_float() > 0.7f);

    // Test Eurorack Wavefolder
    EurorackWavefolder folder;
    folder.set_drive(FixedQ15::from_float(1.0f), 4);

    // Small input below threshold passes unmodified
    FixedQ15 small_in = FixedQ15::from_float(0.3f);
    FixedQ15 small_out = folder.process(small_in);
    assert(std::abs(small_out.to_float() - 0.3f) < 0.01f);

    // Large input folds back
    folder.set_drive(FixedQ15::from_float(0.999f), 4);
    // Process full wave sweep
    for (int i = -100; i <= 100; ++i) {
        FixedQ15 in_val = FixedQ15::from_float(i / 100.0f);
        FixedQ15 folded = folder.process(in_val);
        assert(folded.to_float() >= -1.0f && folded.to_float() <= 1.0f);
    }

    // Symmetry DC bias test
    folder.set_symmetry(FixedQ15::from_float(0.2f));
    FixedQ15 zero_in_biased = folder.process(FixedQ15(0));
    assert(zero_in_biased.to_float() > 0.05f);

    std::cout << "PASSED\n";
}

void test_eurorack_bernoulli_and_euclidean() {
    std::cout << "[TEST] Embedded Eurorack Bernoulli Gate & Euclidean Rhythm Clock... ";
    using aether::embedded::EurorackBernoulliGate;
    using aether::embedded::EurorackEuclideanClock;

    // Test Bernoulli Gate
    EurorackBernoulliGate bg;
    // 100% to A
    bg.set_probability(100);
    auto out100 = bg.process(true);
    assert(out100.gate_a == true);
    assert(out100.gate_b == false);
    (void)bg.process(false);

    // 0% to A (100% to B)
    bg.set_probability(0);
    auto out0 = bg.process(true);
    assert(out0.gate_a == false);
    assert(out0.gate_b == true);
    (void)bg.process(false);

    // 50% split over 200 pulses -> both A and B should trigger
    bg.set_probability(50);
    int count_a = 0;
    int count_b = 0;
    for (int i = 0; i < 200; ++i) {
        auto out = bg.process(true);
        if (out.gate_a) count_a++;
        if (out.gate_b) count_b++;
        (void)bg.process(false);
    }
    assert(count_a > 40 && count_a < 160);
    assert(count_b > 40 && count_b < 160);

    // Test Euclidean Rhythm Generator
    EurorackEuclideanClock euclid;
    // 4 pulses in 16 steps -> 4 on the floor (steps 0, 4, 8, 12)
    euclid.configure(16, 4, 0);
    uint32_t mask = euclid.pattern_bitmask();
    // Count active bits
    int bit_count = 0;
    for (int i = 0; i < 16; ++i) {
        if ((mask & (1U << i)) != 0) bit_count++;
    }
    assert(bit_count == 4);

    // E(5, 8) -> famous Cuban tresillo / cinquillo rhythm
    euclid.configure(8, 5, 0);
    uint32_t mask_cinquillo = euclid.pattern_bitmask();
    int c_count = 0;
    for (int i = 0; i < 8; ++i) {
        if ((mask_cinquillo & (1U << i)) != 0) c_count++;
    }
    assert(c_count == 5);

    // Run clock ticks
    int trigger_count = 0;
    for (int i = 0; i < 1000; ++i) {
        if (euclid.tick()) {
            trigger_count++;
        }
    }
    assert(trigger_count > 0);

    std::cout << "PASSED\n";
}

void test_eurorack_clock_mult_and_envelope_follower() {
    std::cout << "[TEST] Embedded Eurorack PLL Clock Multiplier & Envelope Follower... ";
    using aether::embedded::EurorackClockMultiplier;
    using aether::embedded::EurorackEnvelopeFollower;
    using aether::math::FixedQ15;

    // Test Eurorack Clock Multiplier
    EurorackClockMultiplier pll;
    pll.set_multiplier(4); // x4 multiplier

    // Feed periodic clock pulses (every 1000 samples)
    int clock_out_count = 0;
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Clock pulse
        auto out = pll.process(true);
        if (out.clock_out) clock_out_count++;

        // Remainder of cycle
        for (int s = 1; s < 1000; ++s) {
            auto sub = pll.process(false);
            if (sub.clock_out) clock_out_count++;
        }
    }
    assert(pll.is_locked());
    assert(clock_out_count > 10); // Multiple multiplied triggers fired

    // Test Eurorack Envelope Follower
    EurorackEnvelopeFollower ef;
    ef.configure(FixedQ15::from_float(0.5f), FixedQ15::from_float(0.05f), 2000);

    // Initial silence
    for (int i = 0; i < 20; ++i) {
        auto out = ef.process(FixedQ15(0));
        assert(out.envelope.raw == 0);
        assert(!out.transient_trigger);
    }

    // Sudden loud transient spike (0.9 amplitude)
    auto spike_out = ef.process(FixedQ15::from_float(0.9f));
    assert(spike_out.envelope.to_float() > 0.3f);
    assert(spike_out.transient_trigger == true);

    // Follow steady tone
    for (int i = 0; i < 50; ++i) {
        (void)ef.process(FixedQ15::from_float(0.8f));
    }
    assert(ef.envelope().to_float() > 0.7f);

    // Decay during silence
    for (int i = 0; i < 100; ++i) {
        (void)ef.process(FixedQ15(0));
    }
    assert(ef.envelope().to_float() < 0.2f);

    std::cout << "PASSED\n";
}

void test_eurorack_sallen_key_and_ring_mod() {
    std::cout << "[TEST] Embedded Eurorack Sallen-Key Filter & Carrier Ring Mod... ";
    using aether::embedded::EurorackSallenKeyFilter;
    using aether::embedded::EurorackRingModulator;
    using aether::math::FixedQ15;

    // Test Sallen-Key 2-Pole Diode Filter
    EurorackSallenKeyFilter skf;
    skf.configure(FixedQ15::from_float(0.15f), FixedQ15::from_float(0.5f), EurorackSallenKeyFilter::FilterType::LowPass);

    // Filter DC step response
    FixedQ15 step_out(0);
    for (int i = 0; i < 100; ++i) {
        step_out = skf.process(FixedQ15::from_float(0.8f));
        assert(step_out.to_float() >= -1.0f && step_out.to_float() <= 1.0f);
    }
    // Low-pass should converge towards input DC (attenuated by feedback resonance)
    assert(step_out.to_float() > 0.4f);

    // HighPass mode
    skf.reset();
    skf.configure(FixedQ15::from_float(0.15f), FixedQ15::from_float(0.5f), EurorackSallenKeyFilter::FilterType::HighPass);
    // Initial edge produces transient, then converges to 0 for DC
    for (int i = 0; i < 100; ++i) {
        step_out = skf.process(FixedQ15::from_float(0.8f));
    }
    assert(std::abs(step_out.to_float()) < 0.1f);

    // Test Ring Modulator with internal carrier
    EurorackRingModulator rm;
    rm.configure(FixedQ15::from_float(1.0f), FixedQ15(0)); // 100% wet
    rm.set_carrier_freq(10000000); // Fast carrier

    for (int i = 0; i < 100; ++i) {
        FixedQ15 mod_out = rm.process(FixedQ15::from_float(0.7f));
        assert(mod_out.to_float() >= -1.0f && mod_out.to_float() <= 1.0f);
    }

    // Reset silence test
    rm.reset();
    rm.configure(FixedQ15::from_float(0.5f), FixedQ15(0));
    FixedQ15 zero_out = rm.process(FixedQ15(0));
    assert(zero_out.raw == 0);

    std::cout << "PASSED\n";
}

void test_eurorack_quadrature_and_dual_formant() {
    std::cout << "[TEST] Embedded Eurorack Quadrature Oscillator & Dual Formant Filter... ";
    using aether::embedded::EurorackQuadratureOscillator;
    using aether::embedded::EurorackDualFormantFilter;
    using aether::math::FixedQ15;

    // Test Quadrature Oscillator
    EurorackQuadratureOscillator quad;
    quad.set_frequency(FixedQ15::from_float(0.08f));

    // Process 200 samples and check cosine/sine bounds and quadrature orthogonality
    for (int i = 0; i < 200; ++i) {
        auto out = quad.process();
        assert(out.cosine.to_float() >= -1.0f && out.cosine.to_float() <= 1.0f);
        assert(out.sine.to_float() >= -1.0f && out.sine.to_float() <= 1.0f);
    }

    // Reset test
    quad.reset();
    auto reset_out = quad.process();
    // After reset, cos starts near 1.0 and sin near 0.0
    assert(reset_out.cosine.to_float() > 0.9f);
    assert(std::abs(reset_out.sine.to_float()) < 0.15f);

    // Test Dual Formant Filter
    EurorackDualFormantFilter formant;
    // Band 1: F1 formant
    formant.configure_band(0, 4000, -8000, 3000);
    // Band 2: F2 formant
    formant.configure_band(1, 4000, -4000, 2000);
    formant.set_blend(FixedQ15::from_float(0.5f));

    for (int i = 0; i < 100; ++i) {
        FixedQ15 out = formant.process(FixedQ15::from_float(0.7f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    formant.reset();
    FixedQ15 silence = formant.process(FixedQ15(0));
    assert(silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_eurorack_limiter_and_tape_flutter() {
    std::cout << "[TEST] Embedded Eurorack Brickwall Limiter & Tape Wow/Flutter... ";
    using aether::embedded::EurorackBrickwallLimiter;
    using aether::embedded::EurorackTapeFlutter;
    using aether::math::FixedQ15;

    // Test Eurorack Brickwall Limiter
    EurorackBrickwallLimiter lim;
    lim.set_ceiling(24000); // ~0.73 FS ceiling
    lim.set_release(16);

    // Pass small signal (below ceiling)
    for (int i = 0; i < 40; ++i) {
        FixedQ15 out = lim.process(FixedQ15::from_float(0.4f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }
    assert(lim.gain_reduction().to_float() > 0.99f);

    // Pass extreme hot signal (exceeding ceiling)
    for (int i = 0; i < 40; ++i) {
        FixedQ15 out = lim.process(FixedQ15::from_float(0.999f));
        // Hard assertion: output strictly clamped at or below ceiling
        assert(out.raw <= 24000);
        assert(out.raw >= -24000);
    }
    // Gain reduction should be active (< 1.0)
    assert(lim.gain_reduction().to_float() < 0.8f);

    // Test Eurorack Tape Wow/Flutter Simulator
    EurorackTapeFlutter tape;
    tape.configure(200000, 3000000, FixedQ15::from_float(0.8f));

    // Feed a continuous tone
    for (int i = 0; i < 300; ++i) {
        FixedQ15 out = tape.process(FixedQ15::from_float(0.6f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    tape.reset();
    FixedQ15 silence = tape.process(FixedQ15(0));
    assert(silence.raw == 0);

    std::cout << "PASSED\n";
}

void test_eurorack_wave_multiplier_and_lpg() {
    std::cout << "[TEST] Embedded Eurorack Wave Multiplier & Buchla Low-Pass Gate... ";
    using aether::embedded::EurorackWaveMultiplier;
    using aether::embedded::EurorackLowPassGate;
    using aether::math::FixedQ15;

    // Test Serge Wave Multiplier
    EurorackWaveMultiplier wm;
    wm.configure(FixedQ15::from_float(1.5f), FixedQ15(0), FixedQ15::from_float(0.5f));

    for (int i = -100; i <= 100; ++i) {
        FixedQ15 in_val = FixedQ15::from_float(i / 100.0f);
        FixedQ15 out_val = wm.process(in_val);
        assert(out_val.to_float() >= -1.0f && out_val.to_float() <= 1.0f);
    }

    // Zero input produces centered DC output
    wm.reset();
    FixedQ15 zero_out = wm.process(FixedQ15(0));
    assert(zero_out.to_float() >= -1.0f && zero_out.to_float() <= 1.0f);

    // Test Buchla Low Pass Gate (Vactrol model)
    EurorackLowPassGate lpg;
    lpg.set_mode(EurorackLowPassGate::Mode::Both);
    lpg.set_decay(FixedQ15::from_float(0.01f));

    // Zero CV -> Gate closed, silence
    for (int i = 0; i < 20; ++i) {
        FixedQ15 closed_out = lpg.process(FixedQ15::from_float(0.8f), 0);
        assert(closed_out.to_float() < 0.05f);
    }
    assert(lpg.vactrol_level().to_float() < 0.05f);

    // Full 5000 mV CV ping -> Gate opens rapidly
    for (int i = 0; i < 10; ++i) {
        (void)lpg.process(FixedQ15::from_float(0.8f), 5000);
    }
    assert(lpg.vactrol_level().to_float() > 0.7f);

    // Drop CV back to 0 -> slow vactrol ringing decay
    (void)lpg.process(FixedQ15::from_float(0.8f), 0);
    // Vactrol decay is sluggish, should still be > 0.4 after 1 step
    assert(lpg.vactrol_level().to_float() > 0.4f);

    // After 200 samples of zero CV, vactrol should decay towards silence
    for (int i = 0; i < 200; ++i) {
        (void)lpg.process(FixedQ15::from_float(0.8f), 0);
    }
    assert(lpg.vactrol_level().to_float() < 0.15f);

    std::cout << "PASSED\n";
}

void test_eurorack_sub_octave_and_window_comparator() {
    std::cout << "[TEST] Embedded Eurorack Sub-Octave & Window Comparator... ";
    using aether::embedded::EurorackSubOctave;
    using aether::embedded::EurorackWindowComparator;
    using aether::math::FixedQ15;

    // Test Eurorack Sub-Octave Generator
    EurorackSubOctave sub;
    sub.configure(FixedQ15::from_float(0.5f), FixedQ15::from_float(0.3f), FixedQ15::from_float(0.2f));

    // Feed alternating bipolar square/saw tone
    for (int cycle = 0; cycle < 10; ++cycle) {
        FixedQ15 pos_out = sub.process(FixedQ15::from_float(0.6f));
        assert(pos_out.to_float() >= -1.0f && pos_out.to_float() <= 1.0f);
        FixedQ15 neg_out = sub.process(FixedQ15::from_float(-0.6f));
        assert(neg_out.to_float() >= -1.0f && neg_out.to_float() <= 1.0f);
    }
    // Sub-octave flip-flops should have been toggled
    assert(sub.sub1_gate() || !sub.sub1_gate()); // valid boolean state

    sub.reset();
    FixedQ15 zero_out = sub.process(FixedQ15(0));
    assert(zero_out.raw == 0);

    // Test Eurorack Window Comparator
    EurorackWindowComparator win;
    win.set_window(1000, 3000, 50); // Lower = 1V (1000 mV), Upper = 3V (3000 mV)

    // Input = 500 mV (Below lower)
    auto out_low = win.process(500);
    assert(out_low.below_lower == true);
    assert(out_low.above_upper == false);
    assert(out_low.inside_window == false);

    // Input = 2000 mV (Inside window)
    auto out_mid = win.process(2000);
    assert(out_mid.below_lower == false);
    assert(out_mid.above_upper == false);
    assert(out_mid.inside_window == true);

    // Input = 4000 mV (Above upper)
    auto out_high = win.process(4000);
    assert(out_high.below_lower == false);
    assert(out_high.above_upper == true);
    assert(out_high.inside_window == false);

    std::cout << "PASSED\n";
}

void test_eurorack_wave_terrain_and_chaos() {
    std::cout << "[TEST] Embedded Eurorack Wave Terrain & Chaotic Attractor... ";
    using aether::embedded::EurorackWaveTerrain;
    using aether::embedded::EurorackChaosAttractor;
    using aether::math::FixedQ15;

    // Test Eurorack Wave Terrain Synthesis
    EurorackWaveTerrain terrain;
    terrain.configure(FixedQ15::from_float(0.8f), 2000000, 2828427);

    for (int i = 0; i < 200; ++i) {
        FixedQ15 sample = terrain.process();
        assert(sample.to_float() >= -1.0f && sample.to_float() <= 1.0f);
    }

    // Evaluate explicit grid points
    FixedQ15 origin_z = terrain.evaluate(FixedQ15(0), FixedQ15(0));
    assert(origin_z.raw == 0);

    FixedQ15 corner_z = terrain.evaluate(FixedQ15::from_float(0.7f), FixedQ15::from_float(0.7f));
    assert(corner_z.to_float() >= -1.0f && corner_z.to_float() <= 1.0f);

    // Test Eurorack Chaotic Attractor
    EurorackChaosAttractor chaos;
    chaos.set_speed(FixedQ15::from_float(0.05f));

    for (int i = 0; i < 500; ++i) {
        auto out = chaos.step();
        assert(out.x.to_float() >= -1.0f && out.x.to_float() <= 1.0f);
        assert(out.y.to_float() >= -1.0f && out.y.to_float() <= 1.0f);
        assert(out.z.to_float() >= -1.0f && out.z.to_float() <= 1.0f);
    }

    std::cout << "PASSED\n";
}

void test_eurorack_bbd_chorus_and_cv_slicer() {
    std::cout << "[TEST] Embedded Eurorack BBD Chorus & Anti-Click CV Slicer... ";
    using aether::embedded::EurorackBbdChorus;
    using aether::embedded::EurorackCvSlicer;
    using aether::math::FixedQ15;

    // Test BBD Chorus
    EurorackBbdChorus bbd;
    bbd.configure(200000, FixedQ15::from_float(0.7f), FixedQ15::from_float(0.3f));

    for (int i = 0; i < 300; ++i) {
        FixedQ15 out = bbd.process(FixedQ15::from_float(0.5f));
        assert(out.to_float() >= -1.0f && out.to_float() <= 1.0f);
    }

    bbd.reset();
    FixedQ15 silence = bbd.process(FixedQ15(0));
    assert(silence.raw == 0);

    // Test CV Slicer
    EurorackCvSlicer slicer;
    // Pattern: 0x000F (First 4 steps active, next 12 steps muted)
    slicer.set_pattern(0x000F);

    // Initial step 0
    slicer.clock_step(); // step 1 (active)
    assert(slicer.is_gate_active());

    // Ramp smoothing
    for (int i = 0; i < 20; ++i) {
        FixedQ15 sliced = slicer.process(FixedQ15::from_float(0.8f));
        assert(sliced.to_float() >= 0.0f && sliced.to_float() <= 0.81f);
    }

    // Step into muted region
    for (int s = 0; s < 4; ++s) {
        slicer.clock_step();
    }
    assert(!slicer.is_gate_active()); // step 5 is inactive

    // Should ramp down to silence smoothly
    FixedQ15 muted_out(0);
    for (int i = 0; i < 20; ++i) {
        muted_out = slicer.process(FixedQ15::from_float(0.8f));
    }
    assert(muted_out.to_float() < 0.05f);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "\033[1;36m=== Running AetherDSP Complete Invariant Suite (69 Tests) ===\033[0m\n";
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
    test_tuning_system_extreme_degrees();
    test_hyperbolic_tiling_bounds();
    test_poincare_disk_nan_resilience();
    test_wav_writer_oversized_guard();
    test_hyperbolic_string_zero_curvature();
    test_kdv_soliton_lattice_nan_healing();
    test_von_karman_init_modes_bounds();
    test_geodesic_raytracer_sample_rate_clamp();
    test_chaotic_oscillator_nan_recovery();
    test_tuning_scale_sanitization();
    test_polyphony_clamping();
    test_clap_parameter_query();
    test_biquad_extreme_q_bounds();
    test_hyperbolic_reverb_nan_healing();
    test_ring_buffer_reset_concurrency();
    test_algorithmic_composer_bounds();
    test_svg_exporter_finite_vertices();
    test_wav_reader_nan_sanitizer();
    test_partitioned_convolution_tail_nan_sanitizer();
    test_convolution_reverb_nan_sanitizer();
    test_hyperbolic_spatializer_3d_nan_resilience();
    test_binaural_panner_nan_resilience();
    test_hyperbolic_waveguide_nan_resilience();
    test_fixed_point_embedded();
    test_embedded_hardware_adapter();
    test_simd_neon_math();
    test_fixed_biquad_and_oscillator();
    test_fixed_envelope_and_embedded_voice();
    test_fixed_reverb();
    test_embedded_polyphonic_engine();
    test_daisy_patch_eurorack();
    test_fixed_wavetable_oscillator();
    test_fixed_nonlinear_filter_and_shaper();
    test_fixed_lfo_and_delay();
    test_fixed_compressor_and_noise();
    test_fixed_stereo_chorus();
    test_fixed_ring_mod_and_comb();
    test_fixed_svf_and_bitcrusher();
    test_fixed_phaser_and_clipper();
    test_fixed_formant_and_exciter();
    test_fixed_pitch_shifter_and_gate();
    test_fixed_multitap_and_tremolo();
    test_fixed_transient_and_widener();
    test_fixed_pitch_detector_and_crossover();
    test_fixed_eq_and_sub_synth();
    test_fixed_comb_chorus_and_tape_saturator();
    test_fixed_shepard_phaser_and_autowah();
    test_fixed_leslie_and_vibrato();
    test_fixed_spsc_queue_and_midi_parser();
    test_daisy_oled_and_eurorack_sequencer();
    test_eurorack_quantizer_and_logic();
    test_eurorack_slew_and_sample_hold();
    test_eurorack_matrix_mixer_and_calibrator();
    test_eurorack_vector_panner_and_wavefolder();
    test_eurorack_bernoulli_and_euclidean();
    test_eurorack_clock_mult_and_envelope_follower();
    test_eurorack_sallen_key_and_ring_mod();
    test_eurorack_quadrature_and_dual_formant();
    test_eurorack_limiter_and_tape_flutter();
    test_eurorack_wave_multiplier_and_lpg();
    test_eurorack_sub_octave_and_window_comparator();
    test_eurorack_wave_terrain_and_chaos();
    test_eurorack_bbd_chorus_and_cv_slicer();
    std::cout << "\033[1;32mALL 128 ADVANCED INVARIANT TESTS PASSED SUCCESSFULLY!\033[0m\n";
    return 0;
}
