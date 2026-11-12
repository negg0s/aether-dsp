#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedChorus
 * Stereo modulation chorus / flanger processor in Q15 for embedded targets.
 * Features:
 * - Dual-voice variable-delay lines (up to 512 samples each)
 * - Anti-phase triangle LFO modulation (90-degree phase shift between Left and Right)
 * - Linear fractional delay interpolation in Q15
 * - Zero heap, zero FPU, strictly bounded.
 */
class FixedChorus {
public:
    static constexpr size_t MAX_DELAY = 512;
    static constexpr size_t DELAY_MASK = MAX_DELAY - 1;

    constexpr FixedChorus() noexcept = default;

    /**
     * configure
     * base_delay: base offset in samples [5, 250]
     * depth: modulation depth in samples [1, 100]
     * rate_inc: LFO 32-bit phase increment per sample
     * feedback: Q15 feedback amount [-32768, 32767]
     * mix: wet level in Q15 [0, 32767]
     */
    void configure(size_t base_delay, size_t depth, uint32_t rate_inc, FixedQ15 feedback, FixedQ15 mix) noexcept {
        base_delay_ = std::clamp<size_t>(base_delay, 5, 250);
        depth_ = std::clamp<size_t>(depth, 1, 100);
        rate_inc_ = rate_inc;
        feedback_ = feedback;
        mix_ = mix;
    }

    void reset() noexcept {
        buf_l_.fill(FixedQ15::from_raw(0));
        buf_r_.fill(FixedQ15::from_raw(0));
        write_pos_ = 0;
        lfo_phase_ = 0;
    }

    struct StereoQ15 {
        FixedQ15 left;
        FixedQ15 right;
    };

    [[nodiscard]] StereoQ15 process(FixedQ15 in_l, FixedQ15 in_r) noexcept {
        // Step anti-phase LFO (left = phase, right = phase + 90 deg)
        lfo_phase_ += rate_inc_;
        uint32_t phase_l = lfo_phase_;
        uint32_t phase_r = lfo_phase_ + 0x40000000u; // 90 degree offset

        // Compute modulated delay for Left
        auto [int_d_l, frac_l] = compute_delay_offset(phase_l);
        auto [int_d_r, frac_r] = compute_delay_offset(phase_r);

        // Read interpolated wet sample Left
        FixedQ15 wet_l = read_interpolated(buf_l_, int_d_l, frac_l);
        FixedQ15 wet_r = read_interpolated(buf_r_, int_d_r, frac_r);

        // Write input + feedback to buffers
        int32_t fb_l = (static_cast<int32_t>(wet_l.raw) * feedback_.raw) >> 15;
        int32_t fb_r = (static_cast<int32_t>(wet_r.raw) * feedback_.raw) >> 15;

        int32_t to_write_l = std::clamp<int32_t>(static_cast<int32_t>(in_l.raw) + fb_l, -32768, 32767);
        int32_t to_write_r = std::clamp<int32_t>(static_cast<int32_t>(in_r.raw) + fb_r, -32768, 32767);

        buf_l_[write_pos_] = FixedQ15::from_raw(static_cast<int16_t>(to_write_l));
        buf_r_[write_pos_] = FixedQ15::from_raw(static_cast<int16_t>(to_write_r));
        write_pos_ = (write_pos_ + 1) & DELAY_MASK;

        // Mix dry / wet
        int32_t dry_mul = 32767 - mix_.raw;
        int32_t out_l = ((static_cast<int32_t>(in_l.raw) * dry_mul) >> 15) + ((static_cast<int32_t>(wet_l.raw) * mix_.raw) >> 15);
        int32_t out_r = ((static_cast<int32_t>(in_r.raw) * dry_mul) >> 15) + ((static_cast<int32_t>(wet_r.raw) * mix_.raw) >> 15);

        out_l = std::clamp<int32_t>(out_l, -32768, 32767);
        out_r = std::clamp<int32_t>(out_r, -32768, 32767);

        return {FixedQ15::from_raw(static_cast<int16_t>(out_l)), FixedQ15::from_raw(static_cast<int16_t>(out_r))};
    }

private:
    struct DelayComp {
        size_t int_delay;
        int32_t frac_q15;
    };

    [[nodiscard]] DelayComp compute_delay_offset(uint32_t phase) const noexcept {
        // Triangle LFO from phase: range [0, 32767]
        uint16_t p = static_cast<uint16_t>(phase >> 16);
        int32_t tri = 0;
        if (p < 32768) {
            tri = static_cast<int32_t>(p); // 0 to 32767
        } else {
            tri = 65535 - static_cast<int32_t>(p); // 32767 to 0
        }

        // Total delay in Q15 = (base_delay << 15) + depth * tri
        int32_t total_q15 = (static_cast<int32_t>(base_delay_) << 15) + ((static_cast<int32_t>(depth_) * tri));
        size_t int_d = static_cast<size_t>(total_q15 >> 15);
        int32_t frac = total_q15 & 0x7FFF;

        return {int_d, frac};
    }

    [[nodiscard]] FixedQ15 read_interpolated(const std::array<FixedQ15, MAX_DELAY>& buf, size_t int_d, int32_t frac) const noexcept {
        size_t idx0 = (write_pos_ + MAX_DELAY - int_d) & DELAY_MASK;
        size_t idx1 = (idx0 + MAX_DELAY - 1) & DELAY_MASK;

        int32_t s0 = buf[idx0].raw;
        int32_t s1 = buf[idx1].raw;
        int32_t interpolated = s0 + (((s1 - s0) * frac) >> 15);
        interpolated = std::clamp<int32_t>(interpolated, -32768, 32767);

        return FixedQ15::from_raw(static_cast<int16_t>(interpolated));
    }

    std::array<FixedQ15, MAX_DELAY> buf_l_{};
    std::array<FixedQ15, MAX_DELAY> buf_r_{};
    size_t write_pos_{0};
    uint32_t lfo_phase_{0};
    size_t base_delay_{64};
    size_t depth_{32};
    uint32_t rate_inc_{1000000};
    FixedQ15 feedback_{FixedQ15::from_raw(0)};
    FixedQ15 mix_{FixedQ15::from_raw(16384)}; // 50%
};

} // namespace aether::math
