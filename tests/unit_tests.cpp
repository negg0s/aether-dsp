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
#include <iostream>
#include <cassert>
#include <cmath>

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
    cfg.max_time = 0.2f;
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

int main() {
    std::cout << "\033[1;36m=== Running AetherDSP Complete Invariant Suite (25 Tests) ===\033[0m\n";
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
    std::cout << "\033[1;32mALL 25 ADVANCED INVARIANT TESTS PASSED SUCCESSFULLY!\033[0m\n";
    return 0;
}
