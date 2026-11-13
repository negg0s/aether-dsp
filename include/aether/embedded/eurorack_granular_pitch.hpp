#pragma once

#include <cstdint>
#include <cstddef>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack Granular Pitch Shifter and Time Disperser.
 * Uses a fixed 2048-sample circular buffer with two overlapping,
 * phase-opposed Hanning windowed playback heads.
 *
 * Transposition CV standard: 1V/Oct (1000 mV = 1 octave = 2x speed).
 * Pure Q15 arithmetic, static memory, zero heap allocation.
 */
class EurorackGranularPitch {
public:
    EurorackGranularPitch()
        : write_pos_(0),
          head1_pos_q16_(0),
          head2_pos_q16_(1024 << 16), // 180 degrees out of phase
          rate_q16_(65536),           // Unity pitch (1.0 in Q16)
          grain_size_(1024),
          grain_mask_(1023),
          dry_wet_q15_(32767) {
        for (size_t i = 0; i < kBufferSize; ++i) {
            buffer_[i] = 0;
        }
    }

    // Set pitch shift in millivolts (-1000 mV to +1000 mV for -1 to +1 octave)
    void set_pitch_shift_mV(int16_t pitch_mV) {
        // Linear approximation of 2^(mV / 1000) around unity
        // 0 mV -> 65536
        // +1000 mV -> 131072
        // -1000 mV -> 32768
        if (pitch_mV >= 0) {
            int32_t step = (pitch_mV > 1000) ? 1000 : pitch_mV;
            rate_q16_ = 65536 + ((65536 * step) / 1000);
        } else {
            int32_t step = (pitch_mV < -1000) ? -1000 : pitch_mV;
            rate_q16_ = 65536 + ((32768 * step) / 1000);
            if (rate_q16_ < 16384) rate_q16_ = 16384;
        }
    }

    void set_dry_wet(int16_t mix_q15) {
        dry_wet_q15_ = (mix_q15 < 0) ? 0 : mix_q15;
    }

    // Process single audio sample
    int16_t process(int16_t in_sample) {
        // Record into circular delay buffer
        buffer_[write_pos_] = in_sample;
        write_pos_ = (write_pos_ + 1) & (kBufferSize - 1);

        // Read grain head 1
        uint32_t idx1 = (head1_pos_q16_ >> 16) & (kBufferSize - 1);
        int16_t s1 = buffer_[idx1];

        // Triangle window for head 1 (0 at boundaries, 1 at center)
        uint32_t grain_phase1 = (head1_pos_q16_ >> 16) & grain_mask_;
        int16_t win1 = compute_window(grain_phase1, grain_size_);
        int32_t out1 = (static_cast<int32_t>(s1) * win1) >> 15;

        // Read grain head 2
        uint32_t idx2 = (head2_pos_q16_ >> 16) & (kBufferSize - 1);
        int16_t s2 = buffer_[idx2];

        uint32_t grain_phase2 = (head2_pos_q16_ >> 16) & grain_mask_;
        int16_t win2 = compute_window(grain_phase2, grain_size_);
        int32_t out2 = (static_cast<int32_t>(s2) * win2) >> 15;

        // Advance heads
        head1_pos_q16_ += rate_q16_;
        head2_pos_q16_ += rate_q16_;

        int32_t wet = out1 + out2;
        if (wet > 32767) wet = 32767;
        if (wet < -32768) wet = -32768;

        // Dry/wet crossfade
        int32_t mixed = ((in_sample * (32767 - dry_wet_q15_)) >> 15) +
                        ((wet * dry_wet_q15_) >> 15);

        if (mixed > 32767) mixed = 32767;
        if (mixed < -32768) mixed = -32768;

        return static_cast<int16_t>(mixed);
    }

    void reset() {
        write_pos_ = 0;
        head1_pos_q16_ = 0;
        head2_pos_q16_ = 1024 << 16;
        for (size_t i = 0; i < kBufferSize; ++i) {
            buffer_[i] = 0;
        }
    }

private:
    static constexpr size_t kBufferSize = 2048;

    static inline int16_t compute_window(uint32_t phase, uint32_t size) {
        uint32_t half = size >> 1;
        if (half == 0) return 32767;
        if (phase < half) {
            return static_cast<int16_t>((phase * 32767) / half);
        } else {
            return static_cast<int16_t>(((size - phase) * 32767) / half);
        }
    }

    int16_t buffer_[kBufferSize];
    size_t write_pos_;
    uint32_t head1_pos_q16_;
    uint32_t head2_pos_q16_;
    uint32_t rate_q16_;
    uint32_t grain_size_;
    uint32_t grain_mask_;
    int16_t dry_wet_q15_;
};

} // namespace embedded
} // namespace aether
