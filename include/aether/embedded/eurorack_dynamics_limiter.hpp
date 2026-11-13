#pragma once

#include <cstdint>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack Dual Analog Peak/RMS Dynamics Limiter.
 * Emulates vintage FET-style lightning-fast transient clamping with soft-knee
 * threshold compression and automatic makeup gain.
 *
 * Strict Q15 fixed-point arithmetic, zero heap allocation.
 */
class EurorackDynamicsLimiter {
public:
    EurorackDynamicsLimiter()
        : threshold_q15_(16384),   // -6 dB default
          knee_width_q15_(4096),   // Soft knee width
          makeup_gain_q15_(32767), // Unity makeup gain
          detector_q15_(0),
          attack_coeff_q15_(28000), // ~0.85 (fast attack)
          release_coeff_q15_(1000)  // ~0.03 (moderate release)
    {}

    // Set peak threshold (8192 to 32767)
    void set_threshold(int16_t thresh_q15) {
        threshold_q15_ = (thresh_q15 < 4096) ? 4096 : thresh_q15;
    }

    // Set soft knee width (0 to 16384)
    void set_knee(int16_t knee_q15) {
        knee_width_q15_ = (knee_q15 < 0) ? 0 : ((knee_q15 > 16384) ? 16384 : knee_q15);
    }

    // Set makeup gain (0 to 32767 for up to unity/scaled)
    void set_makeup_gain(int16_t makeup_q15) {
        makeup_gain_q15_ = (makeup_q15 < 0) ? 0 : makeup_q15;
    }

    // Process single audio sample
    int16_t process(int16_t in_sample) {
        // Full-wave rectified peak level
        int16_t abs_in = (in_sample < 0) ? (in_sample == -32768 ? 32767 : -in_sample) : in_sample;

        // Envelope follower with asymmetrical attack/release
        if (abs_in > detector_q15_) {
            // Attack phase
            int32_t diff = abs_in - detector_q15_;
            detector_q15_ += static_cast<int16_t>((diff * attack_coeff_q15_) >> 15);
        } else {
            // Release phase
            int32_t diff = detector_q15_ - abs_in;
            detector_q15_ -= static_cast<int16_t>((diff * release_coeff_q15_) >> 15);
        }

        // Soft-knee gain calculation
        int16_t gain_reduction_q15 = 32767; // 1.0 (no reduction)
        int16_t lower_knee = threshold_q15_ - (knee_width_q15_ >> 1);
        int16_t upper_knee = threshold_q15_ + (knee_width_q15_ >> 1);

        if (detector_q15_ > upper_knee) {
            // Hard limiting above upper knee: threshold / detector
            if (detector_q15_ > 0) {
                gain_reduction_q15 = static_cast<int16_t>((static_cast<int32_t>(threshold_q15_) * 32767) / detector_q15_);
            }
        } else if (detector_q15_ > lower_knee && knee_width_q15_ > 0) {
            // Quadratic soft knee interpolation
            int32_t delta = detector_q15_ - lower_knee;
            int32_t ratio = (delta * 32767) / knee_width_q15_;
            // Blend from 1.0 to threshold/upper_knee
            int32_t target_gr = (static_cast<int32_t>(threshold_q15_) * 32767) / upper_knee;
            int32_t blend = ((32767 - ratio) * 32767 + ratio * target_gr) >> 15;
            gain_reduction_q15 = static_cast<int16_t>(blend);
        }

        // Apply gain reduction and makeup gain
        int32_t controlled = (static_cast<int32_t>(in_sample) * gain_reduction_q15) >> 15;
        int32_t out = (controlled * makeup_gain_q15_) >> 15;

        // Final brickwall safety clamp
        if (out > 32767) out = 32767;
        if (out < -32768) out = -32768;

        return static_cast<int16_t>(out);
    }

    void reset() {
        detector_q15_ = 0;
    }

private:
    int16_t threshold_q15_;
    int16_t knee_width_q15_;
    int16_t makeup_gain_q15_;
    int16_t detector_q15_;
    int16_t attack_coeff_q15_;
    int16_t release_coeff_q15_;
};

} // namespace embedded
} // namespace aether
