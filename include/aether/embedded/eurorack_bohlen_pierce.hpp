#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Voltage Quantizer with Bohlen-Pierce Macro-Scale.
 * Features:
 * - Bohlen-Pierce scale quantizer based on tritave (3:1 frequency ratio divided into 13 steps).
 * - Step size: 1 tritave = 1902 cents -> 1585 mV (1V/Oct standard scaled).
 * - Step interval: ~121.9 mV per step (13 steps per tritave).
 * - Ideal for non-octave alternative harmonic exploration and metallophone synthesis.
 * - Pure integer arithmetic in millivolts, zero dynamic memory allocation.
 */
class EurorackBohlenPierceQuantizer {
public:
    EurorackBohlenPierceQuantizer() {
        reset();
    }

    void reset() {
        hysteresis_mV_ = 8;
        last_quantized_mV_ = 0;
    }

    void set_hysteresis(int16_t hyst_mV) {
        if (hyst_mV < 0) hyst_mV = 0;
        if (hyst_mV > 30) hyst_mV = 30;
        hysteresis_mV_ = hyst_mV;
    }

    int16_t process(int16_t cv_in_mV) {
        // Hysteresis window check
        int16_t diff = cv_in_mV - last_quantized_mV_;
        if (diff > -hysteresis_mV_ && diff < hysteresis_mV_) {
            return last_quantized_mV_;
        }

        // 1 Tritave (3:1 frequency) = 1585 mV = 13 steps
        // Step size = 1585 / 13 = ~121.92 mV
        // Rounding: step = (cv * 13 + sign * 792) / 1585
        int32_t step = (static_cast<int32_t>(cv_in_mV) * 13 + ((cv_in_mV >= 0) ? 792 : -792)) / 1585;
        int32_t quantized_mV = (step * 1585) / 13;

        last_quantized_mV_ = static_cast<int16_t>(quantized_mV);
        return last_quantized_mV_;
    }

private:
    int16_t hysteresis_mV_{8};
    int16_t last_quantized_mV_{0};
};

} // namespace aether::embedded
