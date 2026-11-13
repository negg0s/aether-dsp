#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Voltage Quantizer with Microtonal Micro-Tuning.
 * Features:
 * - 24-EDO (Quarter-Tone Equal Temperament) quantizer.
 * - Converts continuous 1V/Oct CV in millivolts (-5000 to +5000 mV) to discrete 50-cent quarter-tone steps.
 * - 1 semitone = 83.333 mV; 1 quarter-tone = 41.666 mV.
 * - Adjustable hysteresis deadband to reject analog potentiometer / ADC thermal noise.
 * - Pure integer arithmetic in millivolts, zero heap allocation.
 */
class EurorackQuarterToneQuantizer {
public:
    EurorackQuarterToneQuantizer() {
        reset();
    }

    void reset() {
        hysteresis_mV_ = 5;
        last_quantized_mV_ = 0;
    }

    void set_hysteresis(int16_t hyst_mV) {
        if (hyst_mV < 0) hyst_mV = 0;
        if (hyst_mV > 20) hyst_mV = 20;
        hysteresis_mV_ = hyst_mV;
    }

    int16_t process(int16_t cv_in_mV) {
        // Hysteresis window check
        int16_t diff = cv_in_mV - last_quantized_mV_;
        if (diff > -hysteresis_mV_ && diff < hysteresis_mV_) {
            return last_quantized_mV_;
        }

        // 1 Octave = 1000 mV = 24 quarter-tones
        // Step size = 1000 / 24 = 41.666... mV
        // Rounding: step = (cv * 24 + sign * 500) / 1000
        int32_t step = (static_cast<int32_t>(cv_in_mV) * 24 + ((cv_in_mV >= 0) ? 500 : -500)) / 1000;
        int32_t quantized_mV = (step * 1000) / 24;

        last_quantized_mV_ = static_cast<int16_t>(quantized_mV);
        return last_quantized_mV_;
    }

private:
    int16_t hysteresis_mV_{5};
    int16_t last_quantized_mV_{0};
};

} // namespace aether::embedded
