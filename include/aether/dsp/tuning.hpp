#pragma once

#include "../types.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <fstream>
#include <algorithm>

namespace aether::dsp {

/**
 * @brief Microtonal Tuning System & Scala (.scl) Format Parser.
 *
 * Implements non-Euclidean scale systems suited for inharmonic modal plates,
 * including Bohlen-Pierce (13 steps per tritave 3:1) and Golden Ratio scales.
 */
class TuningSystem {
public:
    enum class PredefinedScale {
        TwelveToneEqualTemperament,
        BohlenPierce,         // 13 divisions of 3:1 (tritave)
        JustIntonation7Limit, // Pure integer harmonic ratios
        GoldenRatioHarmonic   // Microtonal scale based on powers of phi
    };

    explicit TuningSystem(PredefinedScale scale = PredefinedScale::TwelveToneEqualTemperament) {
        set_scale(scale);
    }

    void set_scale(PredefinedScale scale) {
        ratios_.clear();
        ratios_.push_back(1.0); // Base root note ratio

        switch (scale) {
            case PredefinedScale::TwelveToneEqualTemperament:
                period_ratio_ = 2.0;
                for (int i = 1; i < 12; ++i) {
                    ratios_.push_back(std::pow(2.0, i / 12.0));
                }
                break;

            case PredefinedScale::BohlenPierce:
                // Tritave (3:1) divided into 13 equal intervals
                period_ratio_ = 3.0;
                for (int i = 1; i < 13; ++i) {
                    ratios_.push_back(std::pow(3.0, i / 13.0));
                }
                break;

            case PredefinedScale::JustIntonation7Limit:
                period_ratio_ = 2.0;
                ratios_ = {
                    1.0,           // 1/1
                    8.0 / 7.0,     // Supermajor second
                    9.0 / 8.0,     // Major second
                    6.0 / 5.0,     // Minor third
                    5.0 / 4.0,     // Major third
                    4.0 / 3.0,     // Perfect fourth
                    7.0 / 5.0,     // Sub-fifth
                    3.0 / 2.0,     // Perfect fifth
                    8.0 / 5.0,     // Minor sixth
                    5.0 / 3.0,     // Major sixth
                    7.0 / 4.0,     // Harmonic seventh
                    15.0 / 8.0     // Major seventh
                };
                break;

            case PredefinedScale::GoldenRatioHarmonic:
                period_ratio_ = 1.618033988749895; // phi
                for (int i = 1; i < 8; ++i) {
                    ratios_.push_back(std::pow(period_ratio_, i / 8.0));
                }
                break;
        }
    }

    /**
     * @brief Parse a standard Scala (.scl) microtonal tuning file.
     */
    bool load_scala_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::string line;
        std::string new_description;
        bool has_description = false;
        int note_count = -1;
        std::vector<double> new_ratios;
        new_ratios.push_back(1.0);

        try {
            while (std::getline(file, line)) {
                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                if (line.empty() || line[0] == '!') continue; // Comment

                if (!has_description) {
                    new_description = line;
                    has_description = true;
                    continue;
                }

                if (note_count < 0) {
                    note_count = std::stoi(line);
                    if (note_count <= 0 || note_count > 1024) return false;
                    continue;
                }

                // Parse cents or ratio (e.g., "1200.0" or "3/2")
                if (line.find('.') != std::string::npos) {
                    // Cents
                    double cents = std::stod(line);
                    new_ratios.push_back(std::pow(2.0, cents / 1200.0));
                } else if (line.find('/') != std::string::npos) {
                    // Fraction ratio
                    std::stringstream ss(line);
                    double num = 1.0, den = 1.0;
                    char slash = '/';
                    ss >> num >> slash >> den;
                    if (den == 0.0) return false;
                    new_ratios.push_back(num / den);
                } else {
                    // Integer ratio
                    double val = std::stod(line);
                    new_ratios.push_back(val);
                }

                if (static_cast<int>(new_ratios.size()) - 1 == note_count) {
                    break;
                }
            }
        } catch (...) {
            return false;
        }

        if (new_ratios.size() > 1 && static_cast<int>(new_ratios.size()) - 1 == note_count) {
            description_ = std::move(new_description);
            period_ratio_ = new_ratios.back();
            new_ratios.pop_back(); // Remove repeat period
            ratios_ = std::move(new_ratios);
            return true;
        }

        return false;
    }

    /**
     * @brief Get exact frequency for a given degree index and reference frequency.
     */
    [[nodiscard]] float get_frequency(int degree, float root_freq = 220.0f) const noexcept {
        const int num_degrees = static_cast<int>(ratios_.size());
        if (num_degrees == 0) return root_freq;

        int octave = degree / num_degrees;
        int index = degree % num_degrees;

        if (index < 0) {
            index += num_degrees;
            octave -= 1;
        }

        octave = std::clamp(octave, -10, 10);
        const double ratio = ratios_[index] * std::pow(period_ratio_, octave);
        const float freq = static_cast<float>(root_freq * ratio);
        return std::isfinite(freq) ? freq : root_freq;
    }

    void load_scale(double period, std::vector<double> ratios, std::string description = "") {
        period_ratio_ = (std::isfinite(period) && period > 1.0001) ? std::min(period, 100.0) : 2.0;
        if (ratios.empty()) ratios = {1.0};
        for (auto& r : ratios) {
            if (!std::isfinite(r) || r <= 0.0) {
                r = 1.0;
            }
        }
        ratios_ = std::move(ratios);
        description_ = std::move(description);
    }

    [[nodiscard]] size_t size() const noexcept {
        return ratios_.size();
    }

private:
    double period_ratio_{2.0};
    std::vector<double> ratios_{1.0};
    std::string description_{};
};

} // namespace aether::dsp
