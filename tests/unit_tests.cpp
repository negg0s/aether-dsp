#include "aether/types.hpp"
#include "aether/ring_buffer.hpp"
#include "aether/math/poincare.hpp"
#include "aether/math/runge_kutta.hpp"
#include "aether/dsp/biquad.hpp"
#include "aether/engines/hyperbolic_waveguide.hpp"
#include "aether/engines/chladni_plate.hpp"
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
    assert(!rb.push(999)); // Full

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

    // At origin, metric is Euclidean (conformal factor = 2, LB factor = 0.25)
    float cf0 = PoincareDisk::conformal_factor(origin);
    assert(std::abs(cf0 - 2.0f) < 1e-5f);

    float lbf0 = PoincareDisk::laplace_beltrami_factor(origin);
    assert(std::abs(lbf0 - 0.25f) < 1e-5f);

    // Geodesic distance from origin to itself is 0
    float d0 = PoincareDisk::geodesic_distance(origin, origin);
    assert(std::abs(d0) < 1e-5f);

    // Geodesic distance symmetry
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

    // Mode frequencies must strictly be positive and non-zero
    for (const auto& m : modes) {
        if (m.m > 0 && m.n > 0 && m.m != m.n) {
            assert(m.frequency > 0.0f);
            assert(!std::isnan(m.frequency));
        }
    }

    // Step simulation and verify finite bounded output
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

    for (int i = 0; i < 2000; ++i) {
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

    // Pass DC (constant 1.0) through lowpass - output should converge to 1.0
    aether::Sample out = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        out = filter.process(1.0f);
    }
    assert(std::abs(out - 1.0f) < 0.01f);
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "\033[1;36m=== Running AetherDSP Unit Test Suite ===\033[0m\n";
    test_ring_buffer();
    test_poincare_geometry();
    test_chladni_plate();
    test_hyperbolic_waveguide();
    test_biquad_filter();
    std::cout << "\033[1;32mALL UNIT TESTS PASSED SUCCESSFULLY!\033[0m\n";
    return 0;
}
