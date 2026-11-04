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

int main() {
    std::cout << "\033[1;36m=== Running AetherDSP Complete Invariant Suite (15 Tests) ===\033[0m\n";
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
    std::cout << "\033[1;32mALL 15 ADVANCED INVARIANT TESTS PASSED SUCCESSFULLY!\033[0m\n";
    return 0;
}
