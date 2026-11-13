#pragma once

#include <cstdint>
#include <cstddef>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack Morphing Spectral Formant Vowel Filter.
 * Models human vocal tract resonances using a 3-formant parallel bandpass resonator bank.
 * Interpolates smoothly between 5 classic vowels: /a/, /e/, /i/, /o/, /u/.
 *
 * All operations use Q15 fixed-point arithmetic, zero heap allocation.
 */
class EurorackFormantVowel {
public:
    EurorackFormantVowel()
        : vowel_position_q15_(0),
          resonance_q15_(24000) { // ~0.73 Q15
        reset();
    }

    // Set vowel position from 0 to 32767:
    // 0 = /a/, 8192 = /e/, 16384 = /i/, 24576 = /o/, 32767 = /u/
    void set_vowel(int16_t vowel_q15) {
        vowel_position_q15_ = (vowel_q15 < 0) ? 0 : vowel_q15;
    }

    // Set resonance / bandwidth sharpness (8192 to 32000)
    void set_resonance(int16_t res_q15) {
        resonance_q15_ = (res_q15 < 4096) ? 4096 : ((res_q15 > 32000) ? 32000 : res_q15);
    }

    // Process single audio sample
    int16_t process(int16_t in_sample) {
        // Compute formant targets based on vowel morphing
        int16_t f1, f2, f3;
        compute_formant_frequencies(vowel_position_q15_, f1, f2, f3);

        // Process through 3 parallel 2-pole bandpass resonators
        int16_t out1 = bp1_.process(in_sample, f1, resonance_q15_);
        int16_t out2 = bp2_.process(in_sample, f2, resonance_q15_);
        int16_t out3 = bp3_.process(in_sample, f3, resonance_q15_);

        // Weighted mix: 0.5 * f1 + 0.3 * f2 + 0.2 * f3
        int32_t mix = ((static_cast<int32_t>(out1) * 16384) >> 15) +
                      ((static_cast<int32_t>(out2) * 9830) >> 15) +
                      ((static_cast<int32_t>(out3) * 6553) >> 15);

        if (mix > 32767) mix = 32767;
        if (mix < -32768) mix = -32768;

        return static_cast<int16_t>(mix);
    }

    void reset() {
        bp1_.reset();
        bp2_.reset();
        bp3_.reset();
    }

private:
    struct Resonator {
        int16_t y1 = 0;
        int16_t y2 = 0;

        void reset() {
            y1 = 0;
            y2 = 0;
        }

        // Simple tuned 2-pole bandpass resonator
        int16_t process(int16_t in, int16_t freq_coeff_q15, int16_t damp_q15) {
            // y[n] = 2*r*cos(w)*y[n-1] - r^2*y[n-2] + (1-r)*x[n]
            int32_t diff = in - y1;
            int32_t v = (diff * freq_coeff_q15) >> 15;
            int32_t acc = y1 + v - ((static_cast<int32_t>(y2) * (32767 - damp_q15)) >> 16);

            if (acc > 32767) acc = 32767;
            if (acc < -32768) acc = -32768;

            y2 = y1;
            y1 = static_cast<int16_t>(acc);
            return y1;
        }
    };

    static void compute_formant_frequencies(int16_t pos_q15, int16_t& f1, int16_t& f2, int16_t& f3) {
        // 5 vowel formant frequency coefficients (approximate normalized frequencies for 48kHz)
        // /a/: 800 Hz, 1200 Hz, 2500 Hz
        // /e/: 400 Hz, 1800 Hz, 2600 Hz
        // /i/: 250 Hz, 2200 Hz, 3000 Hz
        // /o/: 500 Hz, 900 Hz,  2400 Hz
        // /u/: 300 Hz, 800 Hz,  2200 Hz
        static const int16_t kF1[5] = { 2184, 1092,  682, 1365,  819 };
        static const int16_t kF2[5] = { 3276, 4915, 6007, 2457, 2184 };
        static const int16_t kF3[5] = { 6826, 7100, 8192, 6553, 6007 };

        int32_t scaled = (static_cast<int32_t>(pos_q15) * 4); // 0 to 131068
        int32_t idx = scaled >> 15;
        int32_t frac = scaled & 0x7FFF;

        if (idx >= 4) {
            f1 = kF1[4];
            f2 = kF2[4];
            f3 = kF3[4];
            return;
        }

        f1 = static_cast<int16_t>(kF1[idx] + (((kF1[idx + 1] - kF1[idx]) * frac) >> 15));
        f2 = static_cast<int16_t>(kF2[idx] + (((kF2[idx + 1] - kF2[idx]) * frac) >> 15));
        f3 = static_cast<int16_t>(kF3[idx] + (((kF3[idx + 1] - kF3[idx]) * frac) >> 15));
    }

    int16_t vowel_position_q15_;
    int16_t resonance_q15_;
    Resonator bp1_;
    Resonator bp2_;
    Resonator bp3_;
};

} // namespace embedded
} // namespace aether
