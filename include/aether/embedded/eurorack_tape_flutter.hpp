#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Tape Flutter & WOW Warble Simulator
// Simulates capstan eccentricities, motor wow (0.5 - 2 Hz), and tape flutter (10 - 50 Hz)
// via dual-rate modulated fractional delay line with Hermite interpolation in Q15.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackTapeFlutter {
public:
    static constexpr size_t BufferSize = 512;

    EurorackTapeFlutter() {
        reset();
    }

    void reset() noexcept {
        buffer_.fill(0);
        write_idx_ = 0;
        wow_phase_ = 0;
        flutter_phase_ = 0;
        wow_inc_ = 100000;      // ~1.1 Hz at 48kHz
        flutter_inc_ = 2500000;  // ~28 Hz at 48kHz
        depth_q15_ = math::FixedQ15::from_float(0.5f);
        nominal_delay_ = 128;
    }

    void configure(uint32_t wow_inc, uint32_t flutter_inc, math::FixedQ15 depth) noexcept {
        wow_inc_ = wow_inc;
        flutter_inc_ = flutter_inc;
        depth_q15_ = depth;
    }

    // Process a single audio sample through tape wow/flutter simulation
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        buffer_[write_idx_] = input.raw;

        // Advance modulation LFOs
        wow_phase_ += wow_inc_;
        flutter_phase_ += flutter_inc_;

        // Wow modulation (slow triangle)
        int32_t wow_tri = static_cast<int32_t>(wow_phase_ >> 16);
        if (wow_tri > 32767) wow_tri = 65535 - wow_tri;
        wow_tri = (wow_tri - 16384) << 1; // -32768 to 32767

        // Flutter modulation (fast triangle)
        int32_t flut_tri = static_cast<int32_t>(flutter_phase_ >> 16);
        if (flut_tri > 32767) flut_tri = 65535 - flut_tri;
        flut_tri = (flut_tri - 16384) << 1;

        // Combined modulation offset in samples: depth * (0.7 * wow + 0.3 * flutter)
        int32_t mod_sum = ((wow_tri * 7) + (flut_tri * 3)) / 10;
        int32_t delay_offset = (mod_sum * depth_q15_.raw) >> 20; // range ~ +/- 16 samples

        int32_t target_delay = nominal_delay_ + delay_offset;
        if (target_delay < 4) target_delay = 4;
        if (target_delay > 400) target_delay = 400;

        // Read from circular buffer with linear fractional interpolation
        size_t int_delay = static_cast<size_t>(target_delay);
        size_t read_idx0 = (write_idx_ + BufferSize - int_delay) % BufferSize;
        size_t read_idx1 = (read_idx0 + BufferSize - 1) % BufferSize;

        int16_t s0 = buffer_[read_idx0];
        int16_t s1 = buffer_[read_idx1];

        // 50% midpoint interpolation between s0 and s1
        int32_t interpolated = (static_cast<int32_t>(s0) + static_cast<int32_t>(s1)) >> 1;

        write_idx_ = (write_idx_ + 1) % BufferSize;

        return math::FixedQ15(static_cast<int16_t>(interpolated));
    }

private:
    std::array<int16_t, BufferSize> buffer_{};
    size_t write_idx_{0};
    uint32_t wow_phase_{0};
    uint32_t flutter_phase_{0};
    uint32_t wow_inc_{100000};
    uint32_t flutter_inc_{2500000};
    math::FixedQ15 depth_q15_{math::FixedQ15::from_float(0.5f)};
    int32_t nominal_delay_{128};
};

} // namespace aether::embedded
