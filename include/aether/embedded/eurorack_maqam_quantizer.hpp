#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Voltage Quantizer with Microtonal Arabic Quarter-Tone Maqam Scales.
 * Features:
 * - 24-EDO Maqam Rast, Maqam Bayati, Maqam Hijaz, and Maqam Saba tuning models.
 * - Quantizes continuous 1V/Oct CV to microtonal half-flat (sikah) and neutral third intervals.
 * - 1 semitone = 83.33 mV, 1/4 tone (sikah) = 41.67 mV.
 * - Pure integer arithmetic in millivolts, zero dynamic memory allocation.
 */
class EurorackMaqamQuantizer {
public:
    enum class Maqam : uint8_t {
        Rast = 0,    // 0, 200, 350, 500, 700, 900, 1050 cents
        Bayati = 1,  // 0, 150, 300, 500, 700, 800, 1000 cents
        Hijaz = 2,   // 0, 100, 400, 500, 700, 800, 1000 cents
        Saba = 3     // 0, 150, 300, 400, 700, 800, 1000 cents
    };

    EurorackMaqamQuantizer() {
        init_scales();
        reset();
    }

    void reset() {
        maqam_ = Maqam::Rast;
        root_mV_ = 0;
        hysteresis_mV_ = 6;
        last_quantized_mV_ = 0;
    }

    void set_maqam(Maqam m) {
        maqam_ = m;
    }

    void set_root(int16_t root_mV) {
        root_mV_ = root_mV;
    }

    void set_hysteresis(int16_t hyst_mV) {
        if (hyst_mV < 0) hyst_mV = 0;
        if (hyst_mV > 25) hyst_mV = 25;
        hysteresis_mV_ = hyst_mV;
    }

    int16_t process(int16_t cv_in_mV) {
        int16_t rel_cv = cv_in_mV - root_mV_;

        // Hysteresis window check
        int16_t diff = rel_cv - last_quantized_mV_;
        if (diff > -hysteresis_mV_ && diff < hysteresis_mV_) {
            return last_quantized_mV_ + root_mV_;
        }

        // Octave folding (1000 mV = 1 Octave = 1200 cents)
        int32_t oct = rel_cv / 1000;
        int32_t rem = rel_cv % 1000;
        if (rem < 0) {
            rem += 1000;
            oct -= 1;
        }

        // Find closest scale tone in currently selected Maqam
        const int16_t* scale = scales_[static_cast<size_t>(maqam_)];
        int32_t best_dist = 10000;
        int32_t best_pitch = 0;

        for (size_t i = 0; i < 7; ++i) {
            int32_t target_mV = scale[i];
            int32_t d = rem - target_mV;
            if (d < 0) d = -d;
            if (d < best_dist) {
                best_dist = d;
                best_pitch = target_mV;
            }
        }

        int32_t quantized = oct * 1000 + best_pitch;
        last_quantized_mV_ = static_cast<int16_t>(quantized);
        return last_quantized_mV_ + root_mV_;
    }

private:
    Maqam maqam_{Maqam::Rast};
    int16_t root_mV_{0};
    int16_t hysteresis_mV_{6};
    int16_t last_quantized_mV_{0};
    int16_t scales_[4][7];

    void init_scales() {
        // Cents converted to mV: mV = cents * 1000 / 1200
        // Rast: 0, 200, 350, 500, 700, 900, 1050 cents
        const int16_t rast[7]   = { 0, 167, 292, 417, 583, 750, 875 };
        // Bayati: 0, 150, 300, 500, 700, 800, 1000 cents
        const int16_t bayati[7] = { 0, 125, 250, 417, 583, 667, 833 };
        // Hijaz: 0, 100, 400, 500, 700, 800, 1000 cents
        const int16_t hijaz[7]  = { 0, 83, 333, 417, 583, 667, 833 };
        // Saba: 0, 150, 300, 400, 700, 800, 1000 cents
        const int16_t saba[7]   = { 0, 125, 250, 333, 583, 667, 833 };

        for (size_t i = 0; i < 7; ++i) {
            scales_[0][i] = rast[i];
            scales_[1][i] = bayati[i];
            scales_[2][i] = hijaz[i];
            scales_[3][i] = saba[i];
        }
    }
};

} // namespace aether::embedded
