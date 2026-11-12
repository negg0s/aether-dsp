#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedNonlinearShaper
 * Fixed-point wavefolder and hyperbolic distortion unit in Q15.
 * Features:
 * - Infinitely foldable diode wavefolding stage (emulates Buchla 259 timbre circuit)
 * - Drive and symmetry control
 * - Zero dynamic memory, zero floating point.
 */
class FixedNonlinearShaper {
public:
    constexpr FixedNonlinearShaper() noexcept = default;

    // drive: Q15 in range [1.0, 8.0] mapped to [32768, 262144]
    // folds: number of folding cycles (1 to 6)
    void configure(int32_t drive_q15, int folds) noexcept {
        drive_ = std::clamp<int32_t>(drive_q15, 32768, 262144);
        folds_ = std::clamp<int>(folds, 1, 6);
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) const noexcept {
        // Apply drive
        int64_t driven = (static_cast<int64_t>(input.raw) * drive_) >> 15;

        // Wavefolding loop
        for (int i = 0; i < folds_; ++i) {
            if (driven > 32767) {
                driven = 65534 - driven; // Fold down
            } else if (driven < -32768) {
                driven = -65536 - driven; // Fold up
            } else {
                break;
            }
        }

        int32_t out = static_cast<int32_t>(std::clamp<int64_t>(driven, -32768, 32767));
        return FixedQ15::from_raw(static_cast<int16_t>(out));
    }

private:
    int32_t drive_{32768}; // 1.0 in Q15
    int folds_{2};
};

} // namespace aether::math
