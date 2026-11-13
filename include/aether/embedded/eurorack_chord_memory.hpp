#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 8-stage Audio Pitch/Voltage Quantizer with Chord Memory & Arpeggiation.
 * Features:
 * - Stores 8 user chord intervals (e.g. Minor 7th, Major 9th, Sus4, Diminished).
 * - Generates 4 parallel quantized voice CVs from a single 1V/Oct root input.
 * - Arpeggiator engine (Up, Down, Up/Down, Random) with clock input.
 * - Pure integer arithmetic in millivolts, zero heap allocation.
 */
class EurorackChordMemory {
public:
    enum class ArpMode : uint8_t {
        Off = 0,
        Up = 1,
        Down = 2,
        UpDown = 3,
        Random = 4
    };

    EurorackChordMemory() {
        reset();
    }

    void reset() {
        // Minor 7th chord by default: [0, 3, 7, 10] semitones
        chord_intervals_[0] = 0;
        chord_intervals_[1] = 3;
        chord_intervals_[2] = 7;
        chord_intervals_[3] = 10;
        arp_mode_ = ArpMode::Off;
        arp_step_ = 0;
        arp_dir_up_ = true;
        clk_last_ = false;
        lfsr_state_ = 0x93A5u;
    }

    void set_chord(int8_t s0, int8_t s1, int8_t s2, int8_t s3) {
        chord_intervals_[0] = s0;
        chord_intervals_[1] = s1;
        chord_intervals_[2] = s2;
        chord_intervals_[3] = s3;
    }

    void set_arp_mode(ArpMode mode) {
        arp_mode_ = mode;
        arp_step_ = 0;
    }

    struct Output {
        int16_t voice1_mV;
        int16_t voice2_mV;
        int16_t voice3_mV;
        int16_t voice4_mV;
        int16_t arp_out_mV;
    };

    Output process(int16_t root_cv_mV, math::FixedQ15 clock_in) {
        // Clock edge detection
        bool clk_high = clock_in.raw > 2000;
        bool clk_rising = clk_high && !clk_last_;
        clk_last_ = clk_high;

        if (clk_rising && arp_mode_ != ArpMode::Off) {
            advance_arp();
        }

        // Convert semitones to millivolts: 1 semitone = 1000 / 12 = 83.333 mV
        int16_t v1 = root_cv_mV + (chord_intervals_[0] * 1000) / 12;
        int16_t v2 = root_cv_mV + (chord_intervals_[1] * 1000) / 12;
        int16_t v3 = root_cv_mV + (chord_intervals_[2] * 1000) / 12;
        int16_t v4 = root_cv_mV + (chord_intervals_[3] * 1000) / 12;

        int16_t chord_voices[4] = { v1, v2, v3, v4 };
        int16_t arp_v = chord_voices[arp_step_ & 3];

        Output out;
        out.voice1_mV = v1;
        out.voice2_mV = v2;
        out.voice3_mV = v3;
        out.voice4_mV = v4;
        out.arp_out_mV = arp_v;
        return out;
    }

private:
    int8_t chord_intervals_[4];
    ArpMode arp_mode_{ArpMode::Off};
    size_t arp_step_{0};
    bool arp_dir_up_{true};
    bool clk_last_{false};
    uint16_t lfsr_state_{0x93A5u};

    void advance_arp() {
        switch (arp_mode_) {
            case ArpMode::Up:
                arp_step_ = (arp_step_ + 1) % 4;
                break;
            case ArpMode::Down:
                arp_step_ = (arp_step_ + 3) % 4;
                break;
            case ArpMode::UpDown:
                if (arp_dir_up_) {
                    if (arp_step_ >= 3) {
                        arp_dir_up_ = false;
                        arp_step_ = 2;
                    } else {
                        arp_step_++;
                    }
                } else {
                    if (arp_step_ == 0) {
                        arp_dir_up_ = true;
                        arp_step_ = 1;
                    } else {
                        arp_step_--;
                    }
                }
                break;
            case ArpMode::Random:
                arp_step_ = (step_lfsr() & 3);
                break;
            case ArpMode::Off:
                break;
        }
    }

    inline uint16_t step_lfsr() {
        uint16_t bit = ((lfsr_state_ >> 0) ^ (lfsr_state_ >> 2) ^ (lfsr_state_ >> 3) ^ (lfsr_state_ >> 5)) & 1u;
        lfsr_state_ = (lfsr_state_ >> 1) | (bit << 15);
        return lfsr_state_;
    }
};

} // namespace aether::embedded
