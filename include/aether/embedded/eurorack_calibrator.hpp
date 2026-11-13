#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision Calibration & CV Trimmer
// Corrects hardware ADC/DAC offset voltages and gain scaling errors
// using zero-allocation linear calibration equations: V_cal = (V_raw * gain) >> 15 + offset_mv.
// Essential for exact 1V/Oct pitch tracking across temperature and component tolerance drift.
class EurorackCvCalibrator {
public:
    static constexpr size_t MaxChannels = 4;

    struct CalibrationPoint {
        int16_t offset_mv{0}; // DAC/ADC zero-offset in millivolts
        int32_t scale_gain_q15{32768}; // Unity gain (1.0 in Q15 = 32768)
    };

    EurorackCvCalibrator() {
        reset();
    }

    void reset() noexcept {
        for (auto& cal : calibration_) {
            cal.offset_mv = 0;
            cal.scale_gain_q15 = 32768; // 1.0
        }
    }

    // Two-point calibration from measured reference voltages:
    // measured_v1_mv: actual ADC/DAC read value when theoretical 1.0V (1000 mV) was applied
    // measured_v3_mv: actual ADC/DAC read value when theoretical 3.0V (3000 mV) was applied
    void calibrate_two_point(size_t channel, int16_t measured_v1_mv, int16_t measured_v3_mv) noexcept {
        if (channel >= MaxChannels) return;
        int32_t delta_measured = measured_v3_mv - measured_v1_mv;
        if (delta_measured <= 0) return;

        // Theoretical delta = 2000 mV
        // scale_gain = (2000 << 15) / delta_measured
        int64_t gain = (2000LL << 15) / delta_measured;
        calibration_[channel].scale_gain_q15 = static_cast<int32_t>(gain);

        // offset = 1000 - (measured_v1_mv * gain >> 15)
        int64_t scaled_v1 = (static_cast<int64_t>(measured_v1_mv) * gain) >> 15;
        calibration_[channel].offset_mv = static_cast<int16_t>(1000 - scaled_v1);
    }

    // Apply calibration to raw ADC/CV reading
    [[nodiscard]] int16_t apply(size_t channel, int16_t raw_mv) const noexcept {
        if (channel >= MaxChannels) return raw_mv;
        const auto& cal = calibration_[channel];
        int64_t scaled = (static_cast<int64_t>(raw_mv) * cal.scale_gain_q15) >> 15;
        int64_t calibrated = scaled + cal.offset_mv;

        if (calibrated > 10000) calibrated = 10000;
        if (calibrated < -10000) calibrated = -10000;

        return static_cast<int16_t>(calibrated);
    }

private:
    std::array<CalibrationPoint, MaxChannels> calibration_{};
};

} // namespace aether::embedded
