#pragma once

#include "types.hpp"
#include "engine.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>

namespace aether {

struct AetherPresetData {
    std::string name{"Default"};
    int preset_id{0};
    float wave_speed{0.45f};
    float damping{0.003f};
    float curvature_scale{1.0f};
    float plate_fundamental{220.0f};
    float plate_stiffness{1.0f};
    float reverb_decay{0.85f};
    float reverb_mix{0.30f};
    std::string tuning_scale{"12-EDO"};
};

/**
 * @brief Human-Readable .aether Preset File Serializer and Parser (Pure C++20).
 */
class PresetManager {
public:
    static bool save_preset(const std::string& filepath, const AetherPresetData& data) {
        std::ofstream out(filepath);
        if (!out.is_open()) return false;

        out << "# AetherDSP Preset Configuration File\n";
        out << "version = 1.0\n";
        out << "name = " << data.name << "\n";
        out << "preset_id = " << data.preset_id << "\n";
        out << "wave_speed = " << data.wave_speed << "\n";
        out << "damping = " << data.damping << "\n";
        out << "curvature_scale = " << data.curvature_scale << "\n";
        out << "plate_fundamental = " << data.plate_fundamental << "\n";
        out << "plate_stiffness = " << data.plate_stiffness << "\n";
        out << "reverb_decay = " << data.reverb_decay << "\n";
        out << "reverb_mix = " << data.reverb_mix << "\n";
        out << "tuning_scale = " << data.tuning_scale << "\n";

        return true;
    }

    static bool load_preset(const std::string& filepath, AetherPresetData& data) {
        std::ifstream in(filepath);
        if (!in.is_open()) return false;

        std::string line;
        while (std::getline(in, line)) {
            // Trim
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            if (line.empty() || line[0] == '#') continue;

            const size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string key = line.substr(0, eq_pos);
            std::string val = line.substr(eq_pos + 1);

            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);

            try {
                if (key == "name") data.name = val;
                else if (key == "preset_id") data.preset_id = std::stoi(val);
                else if (key == "wave_speed") data.wave_speed = std::stof(val);
                else if (key == "damping") data.damping = std::stof(val);
                else if (key == "curvature_scale") data.curvature_scale = std::stof(val);
                else if (key == "plate_fundamental") data.plate_fundamental = std::stof(val);
                else if (key == "plate_stiffness") data.plate_stiffness = std::stof(val);
                else if (key == "reverb_decay") data.reverb_decay = std::stof(val);
                else if (key == "reverb_mix") data.reverb_mix = std::stof(val);
                else if (key == "tuning_scale") data.tuning_scale = val;
            } catch (...) {
                // Ignore malformed key-value line safely
                continue;
            }
        }

        data.wave_speed = std::clamp(data.wave_speed, 0.01f, 0.99f);
        data.damping = std::clamp(data.damping, 0.0f, 0.5f);
        data.curvature_scale = std::clamp(data.curvature_scale, 0.01f, 10.0f);
        data.plate_fundamental = std::clamp(data.plate_fundamental, 10.0f, 10000.0f);
        data.plate_stiffness = std::clamp(data.plate_stiffness, 0.01f, 100.0f);
        data.preset_id = std::clamp(data.preset_id, 0, 1000);
        data.reverb_decay = std::clamp(data.reverb_decay, 0.0f, 0.999f);
        data.reverb_mix = std::clamp(data.reverb_mix, 0.0f, 1.0f);

        return true;
    }
};

} // namespace aether
