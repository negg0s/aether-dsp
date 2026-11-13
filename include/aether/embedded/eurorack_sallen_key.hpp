#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Voltage Controlled Sallen-Key 2-Pole State-Filter
// Analog model of the MS-20 vintage dual diode filter with variable resonance and clipping.
// Direct Form II fixed-point topology with zero-heap and zero-allocation.
class EurorackSallenKeyFilter {
public:
    enum class FilterType : uint8_t {
        LowPass = 0,
        HighPass,
        BandPass
    };

    EurorackSallenKeyFilter() {
        reset();
    }

    void reset() noexcept {
        s1_ = 0;
        s2_ = 0;
        cutoff_coeff_ = math::FixedQ15::from_float(0.1f);
        resonance_q15_ = math::FixedQ15(0);
        type_ = FilterType::LowPass;
    }

    // Set filter cutoff coefficient (0.01 to 0.95 in Q15) and resonance gain
    void configure(math::FixedQ15 cutoff, math::FixedQ15 resonance, FilterType type = FilterType::LowPass) noexcept {
        cutoff_coeff_ = cutoff;
        resonance_q15_ = resonance;
        type_ = type;
    }

    // Process a single audio sample through the 2-pole Sallen-Key core
    // with diode soft-clipping in the feedback resonance loop.
    math::FixedQ15 process(math::FixedQ15 input) noexcept {
        // Feedback resonance with non-linear diode saturation
        int64_t fb = (static_cast<int64_t>(s2_) * resonance_q15_.raw) >> 15;

        // Diode soft saturation: fb / (1 + |fb|/32768)
        int64_t abs_fb = (fb < 0) ? -fb : fb;
        int64_t saturated_fb = (fb << 15) / (32768 + abs_fb);

        int64_t in_sig = static_cast<int64_t>(input.raw) - saturated_fb;

        // Stage 1 leaky integrator: s1 += g * (in_sig - s1)
        int64_t d1 = in_sig - s1_;
        s1_ += (d1 * cutoff_coeff_.raw) >> 15;
        if (s1_ > 65536) s1_ = 65536;
        if (s1_ < -65536) s1_ = -65536;

        // Stage 2 leaky integrator: s2 += g * (s1 - s2)
        int64_t d2 = s1_ - s2_;
        s2_ += (d2 * cutoff_coeff_.raw) >> 15;
        if (s2_ > 65536) s2_ = 65536;
        if (s2_ < -65536) s2_ = -65536;

        int64_t out_sig = 0;
        switch (type_) {
            case FilterType::LowPass:
                out_sig = s2_;
                break;
            case FilterType::HighPass:
                out_sig = in_sig - s1_;
                break;
            case FilterType::BandPass:
                out_sig = s1_ - s2_;
                break;
        }

        if (out_sig > 32767) out_sig = 32767;
        if (out_sig < -32768) out_sig = -32768;

        return math::FixedQ15(static_cast<int16_t>(out_sig));
    }

private:
    int32_t s1_{0};
    int32_t s2_{0};
    math::FixedQ15 cutoff_coeff_{math::FixedQ15::from_float(0.1f)};
    math::FixedQ15 resonance_q15_{0};
    FilterType type_{FilterType::LowPass};
};

} // namespace aether::embedded
