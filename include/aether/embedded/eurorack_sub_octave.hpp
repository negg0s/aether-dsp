#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Analog Phase Locked Sub-Octave Generator
// Synthesizes -1 Octave (/2 flip-flop) and -2 Octave (/4 flip-flop) sub-bass tones
// locked to incoming fundamental zero-crossings with Schmitt trigger hysteresis and RC low-pass.
// Zero-heap, zero-allocation bare-metal MCU implementation.
class EurorackSubOctave {
public:
    EurorackSubOctave() {
        reset();
    }

    void reset() noexcept {
        ff_div2_ = false;
        ff_div4_ = false;
        sub1_filter_ = 0;
        sub2_filter_ = 0;
        prev_sign_ = false;
        hysteresis_ = 1000;
        mix_sub1_ = math::FixedQ15::from_float(0.5f);
        mix_sub2_ = math::FixedQ15::from_float(0.3f);
        cutoff_coeff_ = math::FixedQ15::from_float(0.2f);
        signal_present_ = false;
    }

    void configure(math::FixedQ15 mix_sub1, math::FixedQ15 mix_sub2, math::FixedQ15 filter_cutoff) noexcept {
        mix_sub1_ = mix_sub1;
        mix_sub2_ = mix_sub2;
        cutoff_coeff_ = filter_cutoff;
    }

    // Process a single audio sample and return composite output with generated sub-octaves
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        // Zero-crossing detector with Schmitt trigger hysteresis
        bool current_sign = prev_sign_;
        if (input.raw > hysteresis_) {
            current_sign = true;
            signal_present_ = true;
        } else if (input.raw < -hysteresis_) {
            current_sign = false;
            signal_present_ = true;
        }

        // Trigger on positive rising edge of zero crossing
        if (current_sign && !prev_sign_) {
            ff_div2_ = !ff_div2_;
            if (ff_div2_) {
                ff_div4_ = !ff_div4_;
            }
        }
        prev_sign_ = current_sign;

        if (!signal_present_) {
            return input;
        }

        // Raw square wave sub-octaves (-1.0 to +1.0 in Q15)
        int32_t raw_sub1 = ff_div2_ ? 24000 : -24000;
        int32_t raw_sub2 = ff_div4_ ? 24000 : -24000;

        // 1-pole low-pass filtering to soften raw square harmonics into fat sub-bass
        sub1_filter_ += ((raw_sub1 - sub1_filter_) * cutoff_coeff_.raw) >> 15;
        sub2_filter_ += ((raw_sub2 - sub2_filter_) * cutoff_coeff_.raw) >> 15;

        // Sum dry input + sub1 + sub2
        int64_t dry = input.raw;
        int64_t sub1_part = (static_cast<int64_t>(sub1_filter_) * mix_sub1_.raw) >> 15;
        int64_t sub2_part = (static_cast<int64_t>(sub2_filter_) * mix_sub2_.raw) >> 15;

        int64_t sum = dry + sub1_part + sub2_part;
        if (sum > 32767) sum = 32767;
        if (sum < -32768) sum = -32768;

        return math::FixedQ15(static_cast<int16_t>(sum));
    }

    [[nodiscard]] bool sub1_gate() const noexcept {
        return ff_div2_;
    }

    [[nodiscard]] bool sub2_gate() const noexcept {
        return ff_div4_;
    }

private:
    bool ff_div2_{false};
    bool ff_div4_{false};
    int32_t sub1_filter_{0};
    int32_t sub2_filter_{0};
    bool prev_sign_{false};
    bool signal_present_{false};
    int16_t hysteresis_{1000};
    math::FixedQ15 mix_sub1_{math::FixedQ15::from_float(0.5f)};
    math::FixedQ15 mix_sub2_{math::FixedQ15::from_float(0.3f)};
    math::FixedQ15 cutoff_coeff_{math::FixedQ15::from_float(0.2f)};
};

} // namespace aether::embedded
