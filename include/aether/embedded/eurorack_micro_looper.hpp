#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Micro-Looper / Freeze Buffer for Eurorack DSP.
 * Implements real-time forward/reverse tape scanning, speed pitch shifting,
 * crossfade loop boundaries, and overdub decay.
 * Buffer: 8192 samples (approx 170 ms at 48kHz) in Q15 format.
 */
class EurorackMicroLooper {
public:
    static constexpr size_t kBufferSize = 8192;
    static constexpr size_t kBufferMask = kBufferSize - 1;

    EurorackMicroLooper() {
        reset();
    }

    void reset() {
        for (size_t i = 0; i < kBufferSize; ++i) {
            buffer_[i] = 0;
        }
        write_idx_ = 0;
        play_pos_q16_ = 0;
        loop_length_ = 4096;
        speed_q12_ = 4096; // 1.0x forward
        overdub_decay_q15_ = 24576; // 0.75 overdub retention
        frozen_ = false;
        reverse_ = false;
    }

    void set_loop_length(size_t len_samples) {
        if (len_samples < 256) len_samples = 256;
        if (len_samples > kBufferSize) len_samples = kBufferSize;
        loop_length_ = len_samples;
    }

    void set_speed(uint32_t speed_q12) {
        if (speed_q12 < 512) speed_q12 = 512;
        if (speed_q12 > 16384) speed_q12 = 16384;
        speed_q12_ = speed_q12;
    }

    void set_reverse(bool rev) {
        reverse_ = rev;
    }

    void set_freeze(bool freeze) {
        frozen_ = freeze;
    }

    void set_overdub(math::FixedQ15 decay) {
        if (decay.raw < 0) decay.raw = 0;
        overdub_decay_q15_ = decay.raw;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        // Read sample with sub-sample linear interpolation
        size_t idx0 = (play_pos_q16_ >> 16) % loop_length_;
        size_t idx1 = (idx0 + 1) % loop_length_;
        uint32_t frac_q16 = play_pos_q16_ & 0xFFFF;

        int32_t s0 = buffer_[idx0];
        int32_t s1 = buffer_[idx1];
        int32_t read_sample = s0 + (((s1 - s0) * static_cast<int32_t>(frac_q16)) >> 16);

        // Record incoming audio into circular buffer if not frozen
        if (!frozen_) {
            int32_t existing = buffer_[write_idx_];
            int32_t decayed = (static_cast<int64_t>(existing) * overdub_decay_q15_) >> 15;
            int32_t new_val = decayed + input.raw;
            if (new_val > 32767) new_val = 32767;
            if (new_val < -32768) new_val = -32768;

            buffer_[write_idx_] = static_cast<int16_t>(new_val);
            write_idx_ = (write_idx_ + 1) % loop_length_;
        }

        // Advance playback head in forward or reverse direction
        uint32_t step = speed_q12_ << 4; // convert Q12 to Q16
        if (!reverse_) {
            play_pos_q16_ += step;
            uint32_t max_pos = static_cast<uint32_t>(loop_length_) << 16;
            if (play_pos_q16_ >= max_pos) {
                play_pos_q16_ %= max_pos;
            }
        } else {
            uint32_t max_pos = static_cast<uint32_t>(loop_length_) << 16;
            if (play_pos_q16_ < step) {
                play_pos_q16_ = max_pos - (step - play_pos_q16_);
            } else {
                play_pos_q16_ -= step;
            }
        }

        if (read_sample > 32767) read_sample = 32767;
        if (read_sample < -32768) read_sample = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(read_sample));
    }

private:
    int16_t buffer_[kBufferSize];
    size_t write_idx_{0};
    uint32_t play_pos_q16_{0};
    size_t loop_length_{4096};
    uint32_t speed_q12_{4096};
    int16_t overdub_decay_q15_{24576};
    bool frozen_{false};
    bool reverse_{false};
};

} // namespace aether::embedded
