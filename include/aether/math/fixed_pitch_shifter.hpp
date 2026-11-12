#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedPitchShifter
 * Granular time-domain crossfading pitch shifter in Q15:
 * - Dual overlapping triangular grains with 180-degree quadrature crossfade
 * - Pitch shift ratio from -12 semitones to +12 semitones (0.5x to 2.0x)
 * - Static delay ring buffer of 1024 samples
 * - 100% deterministic, zero heap, zero FPU.
 */
class FixedPitchShifter {
public:
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;
    static constexpr int32_t GRAIN_SIZE = 512;

    constexpr FixedPitchShifter() noexcept = default;

    /**
     * set_shift_semitones
     * semitones: pitch shift in semitones [-12, +12]
     * In fixed point: speed = 2^(semitones / 12)
     */
    void set_shift_semitones(int semitones) noexcept {
        semitones = std::clamp(semitones, -12, 12);
        // Table of 2^(k/12) in Q15 for k = -12 to +12
        static constexpr std::array<int32_t, 25> PITCH_RATIOS_Q15 = {
            16384, 17358, 18390, 19484, 20643, 21870, 23170, 24548, 26008, 27554, 29193, 30929, // -12 to -1
            32768, // 0 (1.0x)
            34716, 36780, 38968, 41285, 43740, 46341, 49097, 52016, 55109, 58386, 61858, 65536  // +1 to +12
        };
        int32_t ratio_q15 = PITCH_RATIOS_Q15[static_cast<size_t>(semitones + 12)];
        // Read speed delta relative to write speed (1.0)
        // delta = 1.0 - ratio
        rate_delta_q15_ = 32768 - ratio_q15;
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
        phase1_q15_ = 0;
        phase2_q15_ = (GRAIN_SIZE / 2) << 15; // 180 degree offset
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        buffer_[write_pos_] = input;

        // Grain 1 offset and window
        int32_t grain1_offset = (phase1_q15_ >> 15);
        int32_t read_idx1 = (static_cast<int32_t>(write_pos_) - grain1_offset + BUFFER_SIZE) & BUFFER_MASK;
        int32_t s1 = buffer_[static_cast<size_t>(read_idx1)].raw;

        // Triangular window for grain 1: rises 0..32767 for first half, falls 32767..0 for second half
        int32_t win1 = compute_window(phase1_q15_);

        // Grain 2 offset and window
        int32_t grain2_offset = (phase2_q15_ >> 15);
        int32_t read_idx2 = (static_cast<int32_t>(write_pos_) - grain2_offset + BUFFER_SIZE) & BUFFER_MASK;
        int32_t s2 = buffer_[static_cast<size_t>(read_idx2)].raw;

        int32_t win2 = compute_window(phase2_q15_);

        // Crossfade sum: (s1 * win1 + s2 * win2) >> 15
        int32_t mixed = ((s1 * win1) >> 15) + ((s2 * win2) >> 15);
        mixed = std::clamp<int32_t>(mixed, -32768, 32767);

        // Advance phases
        phase1_q15_ += rate_delta_q15_;
        phase2_q15_ += rate_delta_q15_;

        int32_t max_phase_q15 = GRAIN_SIZE << 15;
        if (phase1_q15_ >= max_phase_q15) phase1_q15_ -= max_phase_q15;
        if (phase1_q15_ < 0) phase1_q15_ += max_phase_q15;

        if (phase2_q15_ >= max_phase_q15) phase2_q15_ -= max_phase_q15;
        if (phase2_q15_ < 0) phase2_q15_ += max_phase_q15;

        write_pos_ = (write_pos_ + 1) & BUFFER_MASK;

        return FixedQ15::from_raw(static_cast<int16_t>(mixed));
    }

private:
    [[nodiscard]] static constexpr int32_t compute_window(int32_t phase_q15) noexcept {
        int32_t p = (phase_q15 >> 15); // 0 to GRAIN_SIZE - 1
        int32_t half = GRAIN_SIZE / 2;
        if (p < half) {
            return (p * 32767) / half;
        } else {
            return ((GRAIN_SIZE - p) * 32767) / half;
        }
    }

    std::array<FixedQ15, BUFFER_SIZE> buffer_{};
    size_t write_pos_{0};
    int32_t phase1_q15_{0};
    int32_t phase2_q15_{256 << 15};
    int32_t rate_delta_q15_{0};
};

} // namespace aether::math
