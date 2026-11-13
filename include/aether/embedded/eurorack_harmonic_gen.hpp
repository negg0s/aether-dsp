#pragma once

#include <cstdint>
#include <cstddef>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack Harmonic Overtone Generator.
 * Generates up to 8 integer harmonics (1f to 8f) with individual Q15 amplitude levels
 * and selectable odd/even harmonic balance or spectral tilt.
 *
 * Uses single-accumulator phase synthesis with integer harmonic multipliers.
 * Q15 fixed-point arithmetic, zero heap allocation.
 */
class EurorackHarmonicGen {
public:
    EurorackHarmonicGen()
        : phase_(0),
          phase_inc_(0),
          tilt_q15_(0) {
        // Default fundamental full scale, others zero
        weights_q15_[0] = 32767;
        for (size_t i = 1; i < 8; ++i) {
            weights_q15_[i] = 0;
        }
    }

    // Set fundamental frequency via 1V/Oct CV in mV (-3000 to +3000 mV, base C3=130.81 Hz at 0 mV)
    // sample_rate = 48000 Hz
    void set_pitch_cv(int16_t cv_mV) {
        // Base phase increment for 130.81 Hz at 48 kHz: (130.81 / 48000) * 65536 = 178
        // Semitone step: 1000 mV = 12 semitones = 1 octave -> 2x inc per 1000 mV
        int32_t oct = cv_mV / 1000;
        int32_t rem = cv_mV % 1000;
        if (rem < 0) {
            rem += 1000;
            oct -= 1;
        }

        // Linear interpolation within octave: 1.0 to 2.0 (178 to 356)
        int32_t base_inc = 178 + ((178 * rem) / 1000);
        if (oct > 0) {
            base_inc <<= oct;
        } else if (oct < 0) {
            base_inc >>= (-oct);
        }

        if (base_inc < 1) base_inc = 1;
        if (base_inc > 16384) base_inc = 16384; // Nyquist limit for 8x harmonics
        phase_inc_ = static_cast<uint16_t>(base_inc);
    }

    // Set individual harmonic amplitude weight (harmonic: 1 to 8, weight: 0 to 32767)
    void set_harmonic_weight(uint8_t harmonic, int16_t weight_q15) {
        if (harmonic >= 1 && harmonic <= 8) {
            weights_q15_[harmonic - 1] = (weight_q15 < 0) ? 0 : weight_q15;
        }
    }

    // Set preset balance: 0 = fundamental only, 16384 = sawtooth decay (1/n), 32767 = square (odd only 1/n)
    void set_preset_spectrum(int16_t mode_q15) {
        if (mode_q15 < 16384) {
            // Morph fundamental to sawtooth
            int32_t morph = mode_q15 * 2; // 0 to 32767
            weights_q15_[0] = 32767;
            for (size_t i = 1; i < 8; ++i) {
                int32_t saw_weight = 32767 / static_cast<int32_t>(i + 1);
                weights_q15_[i] = static_cast<int16_t>((saw_weight * morph) >> 15);
            }
        } else {
            // Morph sawtooth to square (attenuate even harmonics)
            int32_t morph = (mode_q15 - 16384) * 2; // 0 to 32767
            for (size_t i = 0; i < 8; ++i) {
                int32_t base = 32767 / static_cast<int32_t>(i + 1);
                if ((i % 2) != 0) {
                    // Even harmonic (2nd, 4th, 6th, 8th): attenuate with morph
                    int32_t atten = 32767 - morph;
                    weights_q15_[i] = static_cast<int16_t>((base * atten) >> 15);
                } else {
                    weights_q15_[i] = static_cast<int16_t>(base);
                }
            }
        }
    }

    // Render single sample in Q15
    int16_t process() {
        phase_ += phase_inc_;

        int32_t total_out = 0;

        for (uint8_t h = 1; h <= 8; ++h) {
            int16_t w = weights_q15_[h - 1];
            if (w == 0) continue;

            uint16_t h_phase = phase_ * h;
            // High-speed parabolic/triangle sine approximation in Q15
            int16_t sine_val = approx_sine(h_phase);
            total_out += (static_cast<int32_t>(sine_val) * w) >> 15;
        }

        // Output limiter
        if (total_out > 32767) total_out = 32767;
        if (total_out < -32768) total_out = -32768;

        return static_cast<int16_t>(total_out);
    }

    void reset() {
        phase_ = 0;
    }

private:
    // Fast fixed-point sine approximation via 3-segment triangle folding
    static inline int16_t approx_sine(uint16_t phase) {
        int32_t x;
        if (phase < 16384) {
            x = (static_cast<int32_t>(phase) * 2);
        } else if (phase < 49152) {
            x = 32768 - (static_cast<int32_t>(phase - 16384) * 2);
        } else {
            x = -32768 + (static_cast<int32_t>(phase - 49152) * 2);
        }
        if (x > 32767) x = 32767;
        if (x < -32768) x = -32768;
        return static_cast<int16_t>(x);
    }

    uint16_t phase_;
    uint16_t phase_inc_;
    int16_t tilt_q15_;
    int16_t weights_q15_[8];
};

} // namespace embedded
} // namespace aether
