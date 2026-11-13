#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Multi-Band Envelope Follower & Transient Dynamic Gate.
 * Features:
 * - 3-band split: Low (sub/bass), Mid (body), High (transients/air).
 * - Independent peak envelope detector on each frequency band with fast attack and adjustable release.
 * - Dynamic sidechain ducking and transient trigger generation.
 * - Suitable for punchy percussive compression, auto-wah, and ducking gates.
 */
class EurorackMultibandFollower {
public:
    EurorackMultibandFollower() {
        reset();
    }

    void reset() {
        lp_low_ = 0;
        lp_mid_ = 0;
        env_low_ = 0;
        env_mid_ = 0;
        env_high_ = 0;
        release_q15_ = 31000; // ~0.94 release
    }

    void set_release(math::FixedQ15 rel) {
        if (rel.raw < 1000) rel.raw = 1000;
        if (rel.raw > 32700) rel.raw = 32700;
        release_q15_ = rel.raw;
    }

    struct Output {
        math::FixedQ15 env_low;
        math::FixedQ15 env_mid;
        math::FixedQ15 env_high;
        math::FixedQ15 transient_gate;
    };

    Output process(math::FixedQ15 input) {
        int32_t in = input.raw;

        // Band splitter: 1-pole lowpass cascade
        // Split low at ~250 Hz (coeff ~1100 in Q15 at 48kHz)
        lp_low_ += ((static_cast<int64_t>(in - lp_low_) * 1100) >> 15);
        int32_t low_band = lp_low_;

        // Split mid at ~2500 Hz (coeff ~9500 in Q15 at 48kHz)
        lp_mid_ += ((static_cast<int64_t>(in - lp_mid_) * 9500) >> 15);
        int32_t mid_band = lp_mid_ - lp_low_;
        int32_t high_band = in - lp_mid_;

        // Rectify
        int32_t abs_l = (low_band >= 0) ? low_band : -low_band;
        int32_t abs_m = (mid_band >= 0) ? mid_band : -mid_band;
        int32_t abs_h = (high_band >= 0) ? high_band : -high_band;

        // Envelope followers (instant attack, leaky release)
        env_low_ = (abs_l > env_low_) ? abs_l : static_cast<int32_t>((static_cast<int64_t>(env_low_) * release_q15_) >> 15);
        env_mid_ = (abs_m > env_mid_) ? abs_m : static_cast<int32_t>((static_cast<int64_t>(env_mid_) * release_q15_) >> 15);
        env_high_ = (abs_h > env_high_) ? abs_h : static_cast<int32_t>((static_cast<int64_t>(env_high_) * release_q15_) >> 15);

        if (env_low_ > 32767) env_low_ = 32767;
        if (env_mid_ > 32767) env_mid_ = 32767;
        if (env_high_ > 32767) env_high_ = 32767;

        // Fast high-band transient trigger detection
        bool transient = (abs_h > 15000) && (abs_h > (env_high_ * 3) / 2);

        Output out;
        out.env_low = math::FixedQ15::from_raw(static_cast<int16_t>(env_low_));
        out.env_mid = math::FixedQ15::from_raw(static_cast<int16_t>(env_mid_));
        out.env_high = math::FixedQ15::from_raw(static_cast<int16_t>(env_high_));
        out.transient_gate = math::FixedQ15::from_raw(transient ? 32767 : 0);
        return out;
    }

private:
    int32_t lp_low_{0};
    int32_t lp_mid_{0};
    int32_t env_low_{0};
    int32_t env_mid_{0};
    int32_t env_high_{0};
    int16_t release_q15_{31000};
};

} // namespace aether::embedded
