#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedAcousticResonator
 * Tunable physical modeling acoustic comb resonator in Q15:
 * - Plucked strings, Karplus-Strong synthesis, metallic acoustic resonators.
 * - Max buffer: 1024 samples (down to ~46.8 Hz fundamental pitch at 48 kHz).
 * - Internal 1-pole lowpass filter in feedback loop for realistic acoustic string loss.
 * - Zero dynamic memory allocation.
 */
class FixedAcousticResonator {
public:
    static constexpr size_t MAX_SIZE = 1024;
    static constexpr size_t MASK = MAX_SIZE - 1;

    constexpr FixedAcousticResonator() noexcept = default;

    /**
     * configure
     * delay_length: delay line length in samples [2, 1023]
     * feedback: feedback gain in Q15 [0, 32767]
     * damping: 1-pole lowpass damping factor in Q15 [0, 32767] (0 = bright, 32767 = heavy damping)
     */
    void configure(size_t delay_length, FixedQ15 feedback, FixedQ15 damping) noexcept {
        delay_len_ = std::clamp<size_t>(delay_length, 2, MAX_SIZE - 1);
        feedback_ = feedback;
        damping_ = damping;
    }

    void reset() noexcept {
        buffer_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
        lp_state_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        size_t read_pos = (write_pos_ + MAX_SIZE - delay_len_) & MASK;
        FixedQ15 delayed = buffer_[read_pos];

        // 1-pole lowpass in feedback loop: lp += (delayed - lp) * (1 - damping)
        int32_t one_minus_d = 32767 - damping_.raw;
        int32_t diff = static_cast<int32_t>(delayed.raw) - lp_state_;
        lp_state_ += (diff * one_minus_d) >> 15;
        lp_state_ = std::clamp<int32_t>(lp_state_, -32768, 32767);

        // Feedback attenuation
        int32_t fb = (lp_state_ * feedback_.raw) >> 15;

        // Sum input + feedback
        int32_t to_write = static_cast<int32_t>(input.raw) + fb;
        to_write = std::clamp<int32_t>(to_write, -32768, 32767);

        buffer_[write_pos_] = FixedQ15::from_raw(static_cast<int16_t>(to_write));
        write_pos_ = (write_pos_ + 1) & MASK;

        return delayed;
    }

private:
    std::array<FixedQ15, MAX_SIZE> buffer_{};
    size_t write_pos_{0};
    size_t delay_len_{100};
    FixedQ15 feedback_{FixedQ15::from_raw(32000)}; // ~0.976
    FixedQ15 damping_{FixedQ15::from_raw(8000)};   // mild acoustic damping
    int32_t lp_state_{0};
};

} // namespace aether::math
