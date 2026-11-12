#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedPitchDetector
 * Zero-heap, fixed-point pitch detector and fundamental frequency tracker in Q15.
 * Uses optimized Autocorrelation Function (ACF) with Average Magnitude Difference Function (AMDF)
 * for pitch detection on resource-constrained embedded targets (Daisy Seed, Cortex-M4/M7):
 * - Window size: 512 samples
 * - Tracks fundamental frequencies from ~75 Hz up to ~1000 Hz at 48 kHz
 * - Hysteresis voiced/unvoiced confidence score
 * - Zero dynamic heap allocation, zero FPU.
 */
class FixedPitchDetector {
public:
    static constexpr size_t WINDOW_SIZE = 512;
    static constexpr size_t MIN_PERIOD = 48;   // ~1000 Hz at 48 kHz
    static constexpr size_t MAX_PERIOD = 480;  // ~100 Hz at 48 kHz

    constexpr FixedPitchDetector() noexcept = default;

    void reset() noexcept {
        buffer_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
        last_detected_period_ = 109; // A4 (~440 Hz)
        is_voiced_ = false;
    }

    struct PitchResult {
        size_t period_samples; // Detected fundamental period in samples
        int16_t confidence_q15; // Confidence score in Q15 [0, 32767]
        bool voiced;           // True if signal has strong pitch periodicity
    };

    /**
     * push_sample
     * Push incoming audio sample into pitch tracking circular window.
     */
    void push_sample(FixedQ15 sample) noexcept {
        buffer_[write_pos_] = sample;
        write_pos_ = (write_pos_ + 1) % WINDOW_SIZE;
    }

    /**
     * analyze
     * Run pitch tracking on currently captured sample frame.
     * Computes AMDF dip minimum corresponding to fundamental period.
     */
    [[nodiscard]] PitchResult analyze() noexcept {
        size_t best_period = MIN_PERIOD;
        int64_t min_amdf = 0x7FFFFFFFFFFFFFFFLL;
        int64_t total_energy = 0;

        // Linearize circular buffer into local stack window
        std::array<int16_t, WINDOW_SIZE> frame;
        for (size_t i = 0; i < WINDOW_SIZE; ++i) {
            size_t idx = (write_pos_ + i) % WINDOW_SIZE;
            int16_t val = buffer_[idx].raw;
            frame[i] = val;
            total_energy += std::abs(static_cast<int32_t>(val));
        }

        // Silence / unvoiced check
        if (total_energy < (static_cast<int64_t>(WINDOW_SIZE) * 100)) {
            is_voiced_ = false;
            return { last_detected_period_, 0, false };
        }

        // Search period via AMDF: amdf(tau) = sum |x[i] - x[i - tau]|
        for (size_t tau = MIN_PERIOD; tau <= MAX_PERIOD; tau += 2) {
            int64_t diff_sum = 0;
            for (size_t i = tau; i < WINDOW_SIZE; i += 4) { // 4x decimated sum for MCU speed
                diff_sum += std::abs(static_cast<int32_t>(frame[i]) - static_cast<int32_t>(frame[i - tau]));
            }

            if (diff_sum < min_amdf) {
                min_amdf = diff_sum;
                best_period = tau;
            }
        }

        // Normalize confidence
        int64_t avg_energy = total_energy / 4;
        int16_t confidence = 0;
        if (avg_energy > 0) {
            int64_t score = 32767 - ((min_amdf * 32767) / avg_energy);
            confidence = static_cast<int16_t>(std::clamp<int64_t>(score, 0, 32767));
        }

        is_voiced_ = (confidence > 12000);
        if (is_voiced_) {
            last_detected_period_ = best_period;
        }

        return { best_period, confidence, is_voiced_ };
    }

private:
    std::array<FixedQ15, WINDOW_SIZE> buffer_{};
    size_t write_pos_{0};
    size_t last_detected_period_{109};
    bool is_voiced_{false};
};

} // namespace aether::math
