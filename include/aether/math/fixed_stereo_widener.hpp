#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedStereoWidener
 * Mid/Side stereo imaging and width processor in Q15:
 * - Mid = (Left + Right) / 2
 * - Side = (Left - Right) / 2
 * - Independent Mid and Side gain scaling in Q15
 * - Elliptical bass mono-maker (highpass on Side channel to keep low end centered)
 * - Zero dynamic heap allocation.
 */
class FixedStereoWidener {
public:
    constexpr FixedStereoWidener() noexcept = default;

    /**
     * configure
     * width_q15: side gain multiplier in Q15 (0 = pure mono, 32768 = 1.0x normal, 65536 = 2.0x super wide)
     * bass_mono_cutoff: Q15 cutoff frequency for side highpass [0, 16384]
     */
    void configure(int32_t width_q15, FixedQ15 bass_mono_cutoff) noexcept {
        width_ = std::clamp<int32_t>(width_q15, 0, 131072);
        hp_cutoff_ = std::clamp<int16_t>(bass_mono_cutoff.raw, 0, 20000);
    }

    void reset() noexcept {
        side_lp_state_ = 0;
    }

    struct StereoFrameQ15 {
        FixedQ15 left;
        FixedQ15 right;
    };

    [[nodiscard]] StereoFrameQ15 process(FixedQ15 in_l, FixedQ15 in_r) noexcept {
        int32_t l = in_l.raw;
        int32_t r = in_r.raw;

        // Mid / Side encoding
        int32_t mid = (l + r) >> 1;
        int32_t side = (l - r) >> 1;

        // Elliptical bass mono filter (highpass on side): side_hp = side - side_lp
        side_lp_state_ += ((side - side_lp_state_) * hp_cutoff_) >> 15;
        side_lp_state_ = std::clamp<int32_t>(side_lp_state_, -32768, 32767);
        int32_t side_filtered = side - side_lp_state_;

        // Apply width multiplier to side signal
        int32_t side_scaled = (side_filtered * width_) >> 15;

        // Mid / Side decoding: Left = Mid + Side, Right = Mid - Side
        int32_t out_l = mid + side_scaled;
        int32_t out_r = mid - side_scaled;

        out_l = std::clamp<int32_t>(out_l, -32768, 32767);
        out_r = std::clamp<int32_t>(out_r, -32768, 32767);

        return {FixedQ15::from_raw(static_cast<int16_t>(out_l)), FixedQ15::from_raw(static_cast<int16_t>(out_r))};
    }

private:
    int32_t width_{32768}; // 1.0x
    int16_t hp_cutoff_{4000};
    int32_t side_lp_state_{0};
};

} // namespace aether::math
