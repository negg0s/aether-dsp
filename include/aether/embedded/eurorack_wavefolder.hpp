#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Microtonal Wavefolder & Asymmetric Timbre Shaper
// Multi-stage fold with DC offset bias and symmetry control (West Coast Buchla/Serge style).
// Zero-heap, zero-allocation bare-metal MCU fixed-point wavefolder.
class EurorackWavefolder {
public:
    EurorackWavefolder() {
        reset();
    }

    void reset() noexcept {
        gain_ = math::FixedQ15::from_float(1.0f);
        symmetry_ = math::FixedQ15(0);
        stages_ = 4;
    }

    // Set folding drive gain (1.0 to 8.0 in Q15/Q12 format represented as integer scale)
    void set_drive(math::FixedQ15 gain, uint8_t stages = 4) noexcept {
        gain_ = gain;
        stages_ = (stages > 8) ? 8 : ((stages < 1) ? 1 : stages);
    }

    // Set DC offset symmetry bias (-1.0 to +1.0 in Q15)
    void set_symmetry(math::FixedQ15 sym) noexcept {
        symmetry_ = sym;
    }

    // Process a single audio sample through folding stages
    [[nodiscard]] math::FixedQ15 process(math::FixedQ15 input) const noexcept {
        // Apply drive and symmetry DC bias
        int64_t x = (static_cast<int64_t>(input.raw) * gain_.raw) >> 15;
        x += (symmetry_.raw >> 1); // DC bias

        // Multi-stage triangle folding:
        // Fold back whenever signal exceeds [-1.0, +1.0] (-32768 to 32767)
        for (uint8_t s = 0; s < stages_; ++s) {
            if (x > 32767) {
                x = 65534 - x; // fold down
            } else if (x < -32768) {
                x = -65536 - x; // fold up
            }
        }

        // Final soft-clip clamp
        if (x > 32767) x = 32767;
        if (x < -32768) x = -32768;

        return math::FixedQ15(static_cast<int16_t>(x));
    }

private:
    math::FixedQ15 gain_{math::FixedQ15::from_float(1.0f)};
    math::FixedQ15 symmetry_{0};
    uint8_t stages_{4};
};

} // namespace aether::embedded
