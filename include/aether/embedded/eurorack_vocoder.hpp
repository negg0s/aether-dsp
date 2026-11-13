#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 16-channel Vocoder Filterbank with envelope analysis & synthesis.
 * Microcontroller friendly (Daisy Seed / Cortex-M7), zero heap allocations.
 * Analysis bands: 16 tuned 2-pole bandpass filters analyzing modulator (voice).
 * Synthesis bands: 16 matching bandpass filters processing carrier (synth).
 * Envelopes: Leaky integrator peak followers on analysis bands modulate synthesis bands.
 */
class EurorackVocoder {
public:
    static constexpr size_t kNumBands = 16;

    EurorackVocoder() {
        init_filterbank();
        reset();
    }

    void reset() {
        for (size_t i = 0; i < kNumBands; ++i) {
            ana_s1_[i] = 0;
            ana_s2_[i] = 0;
            syn_s1_[i] = 0;
            syn_s2_[i] = 0;
            follower_[i] = 0;
        }
        formant_shift_q15_ = 0; // 0 = 1:1 matching
        release_q15_ = 31000;   // ~0.94 release coefficient
    }

    /**
     * @brief Set envelope release rate (smoothing) [0, 32767].
     */
    void set_release(math::FixedQ15 rel) {
        if (rel.raw < 0) rel.raw = 0;
        release_q15_ = rel.raw;
    }

    /**
     * @brief Formant shift shifts modulator envelope mapping to synthesis bands [-15, +15].
     */
    void set_formant_shift(int shift) {
        if (shift < -15) shift = -15;
        if (shift > 15) shift = 15;
        formant_shift_ = shift;
    }

    /**
     * @brief Process one sample of modulator (e.g. mic/voice) and carrier (e.g. synth chord).
     */
    math::FixedQ15 process(math::FixedQ15 modulator, math::FixedQ15 carrier) {
        int32_t out_accum = 0;

        for (size_t i = 0; i < kNumBands; ++i) {
            // 1. Analyze modulator band energy
            int32_t ana_bp = process_biquad(modulator.raw, ana_s1_[i], ana_s2_[i], coeff_k_[i], coeff_r_[i]);
            int32_t rect = (ana_bp >= 0) ? ana_bp : -ana_bp;

            // Leaky peak follower
            if (rect > follower_[i]) {
                follower_[i] = rect; // Instant attack
            } else {
                follower_[i] = (static_cast<int64_t>(follower_[i]) * release_q15_) >> 15;
            }

            // 2. Map analysis follower to synthesis band with formant shifting
            int target_band = static_cast<int>(i) + formant_shift_;
            if (target_band >= 0 && target_band < static_cast<int>(kNumBands)) {
                int32_t syn_bp = process_biquad(carrier.raw, syn_s1_[target_band], syn_s2_[target_band], 
                                                coeff_k_[target_band], coeff_r_[target_band]);
                
                // Modulate carrier synthesis band by follower amplitude
                int32_t modulated = (static_cast<int64_t>(syn_bp) * follower_[i]) >> 15;
                out_accum += modulated;
            }
        }

        // Tame master level (sum of 16 bands)
        out_accum >>= 1;

        if (out_accum > 32767) out_accum = 32767;
        if (out_accum < -32768) out_accum = -32768;

        return math::FixedQ15::from_raw(static_cast<int16_t>(out_accum));
    }

private:
    int32_t ana_s1_[kNumBands];
    int32_t ana_s2_[kNumBands];
    int32_t syn_s1_[kNumBands];
    int32_t syn_s2_[kNumBands];
    int32_t follower_[kNumBands];

    int16_t coeff_k_[kNumBands]; // Q14 frequency tuning
    int16_t coeff_r_[kNumBands]; // Q14 resonance damping

    int16_t formant_shift_q15_{0};
    int formant_shift_{0};
    int16_t release_q15_{31000};

    void init_filterbank() {
        // Pre-computed logarithmic frequencies for 16 bands from 150 Hz to 7500 Hz
        // Resonant Chamberlin SVF tuning parameters
        // coeff_k = 2 * sin(pi * f / fs) in Q14
        const int16_t k_table[16] = {
            160, 210, 280, 370, 500, 660, 880, 1170,
            1560, 2080, 2770, 3690, 4920, 6560, 8750, 11660
        };
        // Moderate Q for musical vocal articulation
        const int16_t r_coeff = 2400; // Q14 resonance damping (~0.15)

        for (size_t i = 0; i < kNumBands; ++i) {
            coeff_k_[i] = k_table[i];
            coeff_r_[i] = r_coeff;
        }
    }

    // Chamberlin SVF single bandpass filter tick (Q14 arithmetic)
    inline int32_t process_biquad(int32_t in, int32_t& s1, int32_t& s2, int16_t k, int16_t r) {
        // hp = in - s1 - r * s2
        int32_t hp = in - (s1 >> 14) - ((static_cast<int64_t>(r) * (s2 >> 14)) >> 14);
        // bp = s2 + k * hp
        int32_t bp = s2 + static_cast<int32_t>((static_cast<int64_t>(k) * hp));
        // lp = s1 + k * bp
        int32_t lp = s1 + static_cast<int32_t>((static_cast<int64_t>(k) * (bp >> 14)));

        s1 = lp;
        s2 = bp;

        return bp >> 14;
    }
};

} // namespace aether::embedded
