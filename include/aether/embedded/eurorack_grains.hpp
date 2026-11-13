#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>
#include <cstddef>

namespace aether::embedded {

/**
 * @brief Fixed-point real-time granular audio cloud disperser / pitch-freezer.
 * Designed for microcontrollers (Daisy Seed STM32H7, 64KB RAM budget).
 * Features:
 * - 4 overlapping synchronous/asynchronous grains.
 * - Circular audio record buffer (4096 samples = ~85ms at 48kHz, Q15).
 * - Grain parameters: Position, Pitch playback speed, Size, and Density/Jitter.
 * - Hann envelope windowing per grain computed via triangle approximation.
 * - Freeze mode: locks buffer writing and scans existing grains continuously.
 */
class EurorackGranularCloud {
public:
    static constexpr size_t kBufferSize = 4096;
    static constexpr size_t kBufferMask = kBufferSize - 1;
    static constexpr size_t kMaxGrains = 4;

    EurorackGranularCloud() {
        reset();
    }

    void reset() {
        write_ptr_ = 0;
        freeze_ = false;
        position_q15_ = 0;
        grain_size_ = 1024;
        pitch_ratio_q12_ = 4096; // 1.0x pitch
        density_mask_ = 0x00FF; // Trigger frequency
        lfsr_state_ = 0xACE1u;

        for (size_t i = 0; i < kBufferSize; ++i) {
            buffer_[i] = 0;
        }

        for (size_t i = 0; i < kMaxGrains; ++i) {
            grains_[i].active = false;
            grains_[i].start_pos = 0;
            grains_[i].current_offset_q12 = 0;
            grains_[i].length = 1024;
            grains_[i].pitch_inc_q12 = 4096;
        }
    }

    void set_freeze(bool freeze) {
        freeze_ = freeze;
    }

    void set_position(math::FixedQ15 pos) {
        if (pos.raw < 0) pos.raw = 0;
        position_q15_ = pos.raw;
    }

    void set_size(size_t size_samples) {
        if (size_samples < 64) size_samples = 64;
        if (size_samples > (kBufferSize / 2)) size_samples = kBufferSize / 2;
        grain_size_ = size_samples;
    }

    /**
     * @brief Pitch ratio in Q12 format (4096 = 1.0x, 2048 = 0.5x octave down, 8192 = 2.0x octave up).
     */
    void set_pitch_ratio_q12(uint32_t ratio_q12) {
        if (ratio_q12 < 512) ratio_q12 = 512;
        if (ratio_q12 > 16384) ratio_q12 = 16384;
        pitch_ratio_q12_ = ratio_q12;
    }

    math::FixedQ15 process(math::FixedQ15 input) {
        // Record incoming audio into circular buffer if not frozen
        if (!freeze_) {
            buffer_[write_ptr_] = input.raw;
            write_ptr_ = (write_ptr_ + 1) & kBufferMask;
        }

        // Periodic grain spawn scheduler
        step_scheduler();

        // Accumulate active grains
        int32_t out_accum = 0;

        for (size_t i = 0; i < kMaxGrains; ++i) {
            if (!grains_[i].active) continue;

            auto& g = grains_[i];
            size_t sample_offset = g.current_offset_q12 >> 12;

            if (sample_offset >= g.length) {
                g.active = false;
                continue;
            }

            // Read sample from circular buffer with wrap
            size_t read_idx = (g.start_pos + sample_offset) & kBufferMask;
            int16_t raw_sample = buffer_[read_idx];

            // Linear ramp-up/ramp-down window envelope approximation
            // Envelope is 0 at offset 0, reaches 32767 at length/2, 0 at length
            int32_t env = 0;
            size_t half_len = g.length >> 1;
            if (half_len == 0) half_len = 1;

            if (sample_offset < half_len) {
                env = (static_cast<int32_t>(sample_offset) * 32767) / half_len;
            } else {
                size_t remaining = g.length - sample_offset;
                env = (static_cast<int32_t>(remaining) * 32767) / half_len;
            }
            if (env > 32767) env = 32767;

            int32_t windowed_sample = (static_cast<int32_t>(raw_sample) * env) >> 15;
            out_accum += windowed_sample;

            // Advance grain playback position
            g.current_offset_q12 += g.pitch_inc_q12;
        }

        // Soft limit output
        if (out_accum > 32767) out_accum = 32767;
        if (out_accum < -32768) out_accum = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out_accum));
    }

private:
    struct Grain {
        bool active{false};
        size_t start_pos{0};
        uint32_t current_offset_q12{0};
        size_t length{1024};
        uint32_t pitch_inc_q12{4096};
    };

    int16_t buffer_[kBufferSize];
    Grain grains_[kMaxGrains];
    size_t write_ptr_{0};
    bool freeze_{false};
    int16_t position_q15_{0};
    size_t grain_size_{1024};
    uint32_t pitch_ratio_q12_{4096};
    uint16_t density_mask_{0x00FF};
    uint16_t lfsr_state_{0xACE1u};
    uint32_t clock_div_{0};

    uint16_t step_lfsr() {
        uint16_t bit = ((lfsr_state_ >> 0) ^ (lfsr_state_ >> 2) ^ (lfsr_state_ >> 3) ^ (lfsr_state_ >> 5)) & 1u;
        lfsr_state_ = (lfsr_state_ >> 1) | (bit << 15);
        return lfsr_state_;
    }

    void step_scheduler() {
        clock_div_++;
        // Trigger check every 256 samples
        if ((clock_div_ & 0xFF) == 0) {
            // Find an inactive grain
            for (size_t i = 0; i < kMaxGrains; ++i) {
                if (!grains_[i].active) {
                    spawn_grain(grains_[i]);
                    break;
                }
            }
        }
    }

    void spawn_grain(Grain& g) {
        g.active = true;
        g.length = grain_size_;
        g.current_offset_q12 = 0;
        g.pitch_inc_q12 = pitch_ratio_q12_;

        // Calculate start position relative to write head or scan position
        // position_q15_ in [0, 32767] maps to [0, kBufferSize - 1]
        size_t base_offset = (static_cast<size_t>(position_q15_) * (kBufferSize - 1)) >> 15;
        // Jitter using LFSR
        uint16_t jitter = step_lfsr() & 0x003F; // up to 63 samples jitter
        g.start_pos = (write_ptr_ + kBufferSize - base_offset + jitter) & kBufferMask;
    }
};

} // namespace aether::embedded
