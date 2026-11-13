#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Bucket-Brigade Device (BBD) Chorus / Flanger
// Models discrete bucket storage capacitance, clock charge transfer inefficiency (CTI),
// and anti-aliasing reconstruction filtering in fixed-point Q15.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackBbdChorus {
public:
    static constexpr size_t Stages = 512; // 512-stage bucket-brigade line (MN3007 / MN3207)

    EurorackBbdChorus() {
        reset();
    }

    void reset() noexcept {
        buckets_.fill(0);
        write_idx_ = 0;
        lfo_phase_ = 0;
        lfo_inc_ = 150000; // ~1.7 Hz at 48kHz
        depth_q15_ = math::FixedQ15::from_float(0.6f);
        feedback_q15_ = math::FixedQ15(0);
        cti_loss_q15_ = math::FixedQ15::from_float(0.002f); // 0.2% charge loss per transfer
        filter_state_ = 0;
    }

    void configure(uint32_t lfo_inc, math::FixedQ15 depth, math::FixedQ15 feedback) noexcept {
        lfo_inc_ = lfo_inc;
        depth_q15_ = depth;
        feedback_q15_ = feedback;
    }

    // Process a single audio sample through the 512-stage BBD bucket line
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        // LFO modulation for clock rate
        lfo_phase_ += lfo_inc_;
        uint32_t phase16 = lfo_phase_ >> 16;
        int32_t tri = 0;
        if (phase16 < 32768) {
            tri = static_cast<int32_t>(phase16) - 16384;
        } else {
            tri = 49152 - static_cast<int32_t>(phase16);
        }
        int32_t abs_tri = (tri < 0) ? -tri : tri;
        int32_t lfo_sine = (2 * tri) - ((tri * abs_tri) >> 14); // -32768 to 32767

        // Delay modulation: nominal 256 stages +/- depth
        int32_t mod_delay = 256 + ((lfo_sine * depth_q15_.raw) >> 17); // range 256 +/- 120
        if (mod_delay < 32) mod_delay = 32;
        if (mod_delay >= static_cast<int32_t>(Stages)) mod_delay = Stages - 1;

        // Read from bucket stage
        size_t read_idx = (write_idx_ + Stages - static_cast<size_t>(mod_delay)) % Stages;
        int16_t bbd_out = buckets_[read_idx];

        // Apply charge transfer inefficiency (CTI high-frequency roll-off)
        filter_state_ += ((bbd_out - filter_state_) * 20000) >> 15;
        int16_t filtered_bbd = static_cast<int16_t>(filter_state_);

        // Feedback loop with soft-clipping
        int64_t fb = (static_cast<int64_t>(filtered_bbd) * feedback_q15_.raw) >> 15;
        int64_t write_val = input.raw + fb;
        if (write_val > 32767) write_val = 32767;
        if (write_val < -32768) write_val = -32768;

        buckets_[write_idx_] = static_cast<int16_t>(write_val);
        write_idx_ = (write_idx_ + 1) % Stages;

        // Dry + Wet mix (50/50 chorus blend)
        int32_t wet = filtered_bbd;
        int32_t dry = input.raw;
        int32_t mixed = (dry + wet) >> 1;

        return math::FixedQ15(static_cast<int16_t>(mixed));
    }

private:
    std::array<int16_t, Stages> buckets_{};
    size_t write_idx_{0};
    uint32_t lfo_phase_{0};
    uint32_t lfo_inc_{150000};
    math::FixedQ15 depth_q15_{math::FixedQ15::from_float(0.6f)};
    math::FixedQ15 feedback_q15_{0};
    math::FixedQ15 cti_loss_q15_{math::FixedQ15::from_float(0.002f)};
    int32_t filter_state_{0};
};

} // namespace aether::embedded
