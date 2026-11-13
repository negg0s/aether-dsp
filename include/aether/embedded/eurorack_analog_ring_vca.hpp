#pragma once

#include <cstdint>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack Analog Diode Ring Modulator and Linear VCA.
 * Models four-quadrant multiplier with diode crossover non-linearity,
 * carrier bleed parameter, and integrated audio VCA.
 *
 * All operations use Q15 fixed-point arithmetic with zero heap allocation.
 */
class EurorackAnalogRingVca {
public:
    EurorackAnalogRingVca()
        : carrier_bleed_q15_(0),
          diode_forward_q15_(800), // ~0.024 forward threshold in Q15
          vca_level_q15_(32767) {}

    // Set carrier feedthrough/bleed (0 to 32767)
    void set_carrier_bleed(int16_t bleed_q15) {
        carrier_bleed_q15_ = (bleed_q15 < 0) ? 0 : bleed_q15;
    }

    // Set diode forward drop simulation (crossover distortion)
    void set_diode_knee(int16_t knee_q15) {
        diode_forward_q15_ = (knee_q15 < 0) ? 0 : ((knee_q15 > 8192) ? 8192 : knee_q15);
    }

    // Set VCA level (0 to 32767)
    void set_vca_level(int16_t level_q15) {
        vca_level_q15_ = (level_q15 < 0) ? 0 : level_q15;
    }

    // Process audio and carrier inputs (-32768 to 32767 in Q15)
    // Returns ring modulated signal passed through VCA
    int16_t process(int16_t signal_in, int16_t carrier_in) {
        // Apply diode forward voltage drop threshold to carrier
        int16_t conditioned_carrier = carrier_in;
        if (diode_forward_q15_ > 0) {
            if (conditioned_carrier > diode_forward_q15_) {
                conditioned_carrier -= diode_forward_q15_;
            } else if (conditioned_carrier < -diode_forward_q15_) {
                conditioned_carrier += diode_forward_q15_;
            } else {
                conditioned_carrier = 0;
            }
        }

        // Four-quadrant balanced modulation: signal * carrier
        int32_t ring_prod = (static_cast<int32_t>(signal_in) * conditioned_carrier) >> 15;

        // Carrier bleed injection
        if (carrier_bleed_q15_ > 0) {
            int32_t bleed = (static_cast<int32_t>(carrier_in) * carrier_bleed_q15_) >> 15;
            ring_prod += bleed;
        }

        // Clamp before VCA
        if (ring_prod > 32767) ring_prod = 32767;
        if (ring_prod < -32768) ring_prod = -32768;

        // VCA attenuation
        int32_t vca_out = (ring_prod * vca_level_q15_) >> 15;

        if (vca_out > 32767) vca_out = 32767;
        if (vca_out < -32768) vca_out = -32768;

        return static_cast<int16_t>(vca_out);
    }

    void reset() {
        // Stateless ring core
    }

private:
    int16_t carrier_bleed_q15_;
    int16_t diode_forward_q15_;
    int16_t vca_level_q15_;
};

} // namespace embedded
} // namespace aether
