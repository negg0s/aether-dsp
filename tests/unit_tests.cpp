#include "aether/types.hpp"
#include "aether/ring_buffer.hpp"
#include "aether/math/poincare.hpp"
#include "aether/math/runge_kutta.hpp"
#include "aether/math/hyperbolic_tiling.hpp"
#include "aether/dsp/biquad.hpp"
#include "aether/dsp/tuning.hpp"
#include "aether/dsp/fx/hyperbolic_reverb.hpp"
#include "aether/dsp/fx/binaural_panner.hpp"
#include "aether/engines/hyperbolic_waveguide.hpp"
#include "aether/engines/chladni_plate.hpp"
#include "aether/midi/midi_file.hpp"
#include "aether/wav_writer.hpp"
#include "aether/wav_reader.hpp"
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

    // Right side: azimuth = +90 degrees (right ear leads, left ear delayed)
    panner.set_position(90.0f, 1.0f);
    auto stereo = panner.process(1.0f);
    assert(!std::isnan(stereo.left));
    assert(!std::isnan(stereo.right));

    // Left side: azimuth = -90 degrees
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

int main() {
    std::cout << "\033[1;36m=== Running AetherDSP Complete Invariant Suite (11 Tests) ===\033[0m\n";
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
    std::cout << "\033[1;32mALL 11 ADVANCED INVARIANT TESTS PASSED SUCCESSFULLY!\033[0m\n";
    return 0;
}
