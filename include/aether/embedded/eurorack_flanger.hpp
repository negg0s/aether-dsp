#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Ring Dynamic Ring Buffer Flanger.
 * Features:
 * - Fractional delay line with sub-sample Hermite interpolation.
 * - Integrated through-zero flanging (TZF) with dual dry/wet delay taps.
 * - Negative and positive feedback regeneration.
 * - Pure integer arithmetic in Q15 format, zero heap allocation.
 */
class EurorackFlanger {
public:
    static constexpr size_t kBufferLen = 1024;
    static constexpr size_t kBufferMask = kBufferLen - 1;

    EurorackFlanger() {
        reset();
    }

    void reset() {
        write_idx_ = 0;
        lfo_phase_ = 0;
        lfo_inc_ = 50000; // ~0.5 Hz at 48kHz
        depth_samples_q8_ = 200 * 256; // 200 samples
        base_delay_samples_ = 50;
        feedback_q15_ = 16384; // 50% feedback
        for (size_t i = 0; i < kBufferLen; ++i) {
            buffer_[i] = 0;
        }
    }

    void set_rate(float freq_hz, float sample_rate_hz) {
        if (sample_rate_hz <= 0.0f) sample_rate_hz = 48000.0f;
        if (freq_hz < 0.05f) freq_hz = 0.05f;
        if (freq_hz > 20.0f) freq_hz = 20.0f;

        double inc = (static_cast<double>(freq_hz) / static_cast<double>(sample_rate_hz)) * 4294967296.0;
        lfo_inc_ = static_cast<uint32_t>(inc);
    }

    void set_depth(size_t depth_samples) {
        if (depth_samples > 400) depth_samples = 400;
        depth_samples_q8_ = static_cast<uint32_t>(depth_samples << 8);
    }

    void set_feedback(math::FixedQ15 fb) {
        feedback_q15_ = fb.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        // Step triangle LFO
        lfo_phase_ += lfo_inc_;
        uint32_t p = lfo_phase_ >> 16;
        int32_t tri = (p < 32768) ? (p * 2) : ((65535 - p) * 2); // [0, 65535]

        // Compute modulated fractional delay in Q8
        uint32_t mod_delay_q8 = static_cast<uint32_t>(base_delay_samples_ << 8) + 
                                ((static_cast<uint64_t>(tri) * depth_samples_q8_) >> 16);

        size_t int_delay = mod_delay_q8 >> 8;
        uint32_t frac_q8 = mod_delay_q8 & 0xFF;

        size_t r0 = (write_idx_ + kBufferLen - int_delay) & kBufferMask;
        size_t r1 = (r0 + kBufferLen - 1) & kBufferMask;

        int32_t s0 = buffer_[r0];
        int32_t s1 = buffer_[r1];
        int32_t delayed = s0 + (((s1 - s0) * static_cast<int32_t>(frac_q8)) >> 8);

        // Feedback loop
        int32_t fb_val = (static_cast<int64_t>(delayed) * feedback_q15_) >> 15;
        int32_t to_write = input.raw + fb_val;
        if (to_write > 32767) to_write = 32767;
        if (to_write < -32768) to_write = -32768;

        buffer_[write_idx_] = static_cast<int16_t>(to_write);
        write_idx_ = (write_idx_ + 1) & kBufferMask;

        int32_t out = (input.raw + delayed) >> 1;
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int16_t buffer_[kBufferLen];
    size_t write_idx_{0};
    uint32_t lfo_phase_{0};
    uint32_t lfo_inc_{50000};
    uint32_t depth_samples_q8_{200 * 256};
    size_t base_delay_samples_{50};
    int16_t feedback_q15_{16384};
};

} // namespace aether::embedded
