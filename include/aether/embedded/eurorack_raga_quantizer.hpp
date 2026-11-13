#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Voltage Quantizer with Indian Classical Raga Micro-Tuning.
 * Features:
 * - 22-Shruti Indian classical microtonal scale models (Raga Bhairav, Yaman, Kafi, Darbari).
 * - Exact rational shruti intervals (Komal Re, Shuddha Dha, Tivra Ma, etc.).
 * - Pure integer arithmetic in millivolts, zero heap allocation.
 */
class EurorackRagaQuantizer {
public:
    enum class Raga : uint8_t {
        Bhairav = 0, // Sa, Komal Re, Shuddha Ga, Ma, Pa, Komal Dha, Ni
        Yaman = 1,   // Sa, Re, Ga, Tivra Ma, Pa, Dha, Ni
        Kafi = 2,    // Sa, Re, Komal Ga, Ma, Pa, Dha, Komal Ni
        Darbari = 3  // Sa, Re, Ati-Komal Ga, Ma, Pa, Ati-Komal Dha, Komal Ni
    };

    EurorackRagaQuantizer() {
        init_scales();
        reset();
    }

    void reset() {
        raga_ = Raga::Bhairav;
        root_mV_ = 0;
        hysteresis_mV_ = 6;
        last_quantized_mV_ = 0;
    }

    void set_raga(Raga r) {
        raga_ = r;
    }

    void set_root(int16_t root_mV) {
        root_mV_ = root_mV;
    }

    void set_hysteresis(int16_t hyst_mV) {
        if (hyst_mV < 0) hyst_mV = 0;
        if (hyst_mV > 25) hyst_mV = 25;
        hysteresis_mV_ = hyst_mV;
    }

    int16_t process(int16_t cv_in_mV) {
        int16_t rel_cv = cv_in_mV - root_mV_;

        // Hysteresis window check
        int16_t diff = rel_cv - last_quantized_mV_;
        if (diff > -hysteresis_mV_ && diff < hysteresis_mV_) {
            return last_quantized_mV_ + root_mV_;
        }

        // Octave folding (1000 mV = 1 Octave = 1200 cents)
        int32_t oct = rel_cv / 1000;
        int32_t rem = rel_cv % 1000;
        if (rem < 0) {
            rem += 1000;
            oct -= 1;
        }

        // Find closest tone in currently selected Raga
        const int16_t* scale = scales_[static_cast<size_t>(raga_)];
        int32_t best_dist = 10000;
        int32_t best_pitch = 0;

        for (size_t i = 0; i < 7; ++i) {
            int32_t target_mV = scale[i];
            int32_t d = rem - target_mV;
            if (d < 0) d = -d;
            if (d < best_dist) {
                best_dist = d;
                best_pitch = target_mV;
            }
        }

        int32_t quantized = oct * 1000 + best_pitch;
        last_quantized_mV_ = static_cast<int16_t>(quantized);
        return last_quantized_mV_ + root_mV_;
    }

private:
    Raga raga_{Raga::Bhairav};
    int16_t root_mV_{0};
    int16_t hysteresis_mV_{6};
    int16_t last_quantized_mV_{0};
    int16_t scales_[4][7];

    void init_scales() {
        // Cents converted to mV: mV = cents * 1000 / 1200
        // Bhairav: Sa(0), Komal Re(112c = 93mV), Ga(386c = 322mV), Ma(498c = 415mV), Pa(702c = 585mV), Komal Dha(814c = 678mV), Ni(1088c = 907mV)
        const int16_t bhairav[7] = { 0, 93, 322, 415, 585, 678, 907 };
        // Yaman: Sa(0), Re(204c = 170mV), Ga(386c = 322mV), Tivra Ma(590c = 492mV), Pa(702c = 585mV), Dha(906c = 755mV), Ni(1088c = 907mV)
        const int16_t yaman[7]   = { 0, 170, 322, 492, 585, 755, 907 };
        // Kafi: Sa(0), Re(204c = 170mV), Komal Ga(294c = 245mV), Ma(498c = 415mV), Pa(702c = 585mV), Dha(906c = 755mV), Komal Ni(996c = 830mV)
        const int16_t kafi[7]    = { 0, 170, 245, 415, 585, 755, 830 };
        // Darbari: Sa(0), Re(204c = 170mV), Ati-Komal Ga(280c = 233mV), Ma(498c = 415mV), Pa(702c = 585mV), Ati-Komal Dha(795c = 663mV), Komal Ni(996c = 830mV)
        const int16_t darbari[7] = { 0, 170, 233, 415, 585, 663, 830 };

        for (size_t i = 0; i < 7; ++i) {
            scales_[0][i] = bhairav[i];
            scales_[1][i] = yaman[i];
            scales_[2][i] = kafi[i];
            scales_[3][i] = darbari[i];
        }
    }
};

} // namespace aether::embedded
