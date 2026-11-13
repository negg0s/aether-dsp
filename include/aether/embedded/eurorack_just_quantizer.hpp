#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Voltage Quantizer with Just Intonation & Microtuning.
 * Features:
 * - 12 exact pure rational frequency intervals (Just Intonation ratios relative to tonic).
 * - Supported intervals: 1/1, 16/15, 9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 9/5, 15/8.
 * - Eliminates tempered beating for crystal-clear harmonic drones and overtone synthesis.
 * - Pure integer arithmetic in millivolts, zero dynamic memory allocation.
 */
class EurorackJustQuantizer {
public:
    EurorackJustQuantizer() {
        init_ratios();
        reset();
    }

    void reset() {
        root_offset_mV_ = 0;
        hysteresis_mV_ = 8;
        last_quantized_mV_ = 0;
    }

    void set_root(int16_t root_mV) {
        root_offset_mV_ = root_mV;
    }

    void set_hysteresis(int16_t hyst_mV) {
        if (hyst_mV < 0) hyst_mV = 0;
        if (hyst_mV > 50) hyst_mV = 50;
        hysteresis_mV_ = hyst_mV;
    }

    int16_t process(int16_t cv_in_mV) {
        int16_t rel_cv = cv_in_mV - root_offset_mV_;

        // Hysteresis window
        int16_t diff = rel_cv - last_quantized_mV_;
        if (diff > -hysteresis_mV_ && diff < hysteresis_mV_) {
            return last_quantized_mV_ + root_offset_mV_;
        }

        // Octave folding (1000 mV = 1 Octave)
        int32_t oct = rel_cv / 1000;
        int32_t rem = rel_cv % 1000;
        if (rem < 0) {
            rem += 1000;
            oct -= 1;
        }

        // Find closest Just Intonation pitch class
        int32_t best_dist = 10000;
        int32_t best_pitch = 0;

        for (size_t i = 0; i < 12; ++i) {
            int32_t target_mV = just_offsets_mV_[i];
            int32_t d = rem - target_mV;
            if (d < 0) d = -d;
            if (d < best_dist) {
                best_dist = d;
                best_pitch = target_mV;
            }
        }

        int32_t quantized = oct * 1000 + best_pitch;
        last_quantized_mV_ = static_cast<int16_t>(quantized);
        return last_quantized_mV_ + root_offset_mV_;
    }

private:
    int16_t root_offset_mV_{0};
    int16_t hysteresis_mV_{8};
    int16_t last_quantized_mV_{0};
    int16_t just_offsets_mV_[12];

    void init_ratios() {
        // Just Intonation cents converted to millivolts (1200 cents = 1000 mV -> mV = cents * 1000 / 1200)
        // 1/1 = 0 cents -> 0 mV
        // 16/15 = 111.73 cents -> 93 mV
        // 9/8 = 203.91 cents -> 170 mV
        // 6/5 = 315.64 cents -> 263 mV
        // 5/4 = 386.31 cents -> 322 mV
        // 4/3 = 498.04 cents -> 415 mV
        // 45/32 = 590.22 cents -> 492 mV
        // 3/2 = 701.96 cents -> 585 mV
        // 8/5 = 813.69 cents -> 678 mV
        // 5/3 = 884.36 cents -> 737 mV
        // 9/5 = 1017.60 cents -> 848 mV
        // 15/8 = 1088.27 cents -> 907 mV
        const int16_t offsets[12] = {
            0, 93, 170, 263, 322, 415, 492, 585, 678, 737, 848, 907
        };
        for (size_t i = 0; i < 12; ++i) {
            just_offsets_mV_[i] = offsets[i];
        }
    }
};

} // namespace aether::embedded
