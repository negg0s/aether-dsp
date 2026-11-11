#pragma once

#include "../types.hpp"
#include "../engine.hpp"
#include <string_view>
#include <cstdint>

namespace aether::plugin {

/**
 * @brief CLAP (CLever Audio Plugin) Standard Native Architecture Definition.
 *
 * Exposes AetherDSP as a compliant CLAP synthesizer for professional DAWs (Bitwig, Reaper, FL Studio).
 * Permissive MIT-compatible ABI without proprietary vendor SDK encumbrances.
 */
struct ClapPluginDescriptor {
    const char* id{"com.negg0s.aether-dsp"};
    const char* name{"AetherDSP Non-Euclidean Synthesizer"};
    const char* vendor{"Emir (negg0s)"};
    const char* url{"https://github.com/negg0s/aether-dsp"};
    const char* manual_url{"https://github.com/negg0s/aether-dsp#readme"};
    const char* version{"1.0.0"};
    const char* description{"Non-Euclidean Hyperbolic Waveguide & Modal Cymatics Instrument"};
    const char* const* features;
};

inline const char* const CLAP_FEATURES[] = {
    "instrument",
    "synthesizer",
    "physical-modelling",
    "stereo",
    nullptr
};

inline ClapPluginDescriptor get_descriptor() {
    return ClapPluginDescriptor{
        .id = "com.negg0s.aether-dsp",
        .name = "AetherDSP",
        .vendor = "negg0s",
        .url = "https://github.com/negg0s/aether-dsp",
        .manual_url = "https://github.com/negg0s/aether-dsp#readme",
        .version = "1.0.0",
        .description = "Non-Euclidean Hyperbolic Waveguide & Cymatics Synthesizer",
        .features = CLAP_FEATURES
    };
}

/**
 * @brief CLAP Parameter Identifiers.
 */
enum ParameterId : uint32_t {
    PARAM_PRESET = 0,
    PARAM_CURVATURE = 1,
    PARAM_DAMPING = 2,
    PARAM_STIFFNESS = 3,
    PARAM_REVERB_MIX = 4,
    PARAM_FILTER_CUTOFF = 5
};

struct ParameterInfo {
    uint32_t id;
    const char* name;
    float min_value;
    float max_value;
    float default_value;
};

inline const ParameterInfo PARAMETER_CATALOG[] = {
    {PARAM_PRESET, "Preset Selection", 0.0f, 13.0f, 0.0f},
    {PARAM_CURVATURE, "Hyperbolic Curvature (K)", 0.1f, 3.0f, 1.0f},
    {PARAM_DAMPING, "Wave Absorption", 0.0001f, 0.05f, 0.003f},
    {PARAM_STIFFNESS, "Plate Rigidity (D)", 0.2f, 2.5f, 1.15f},
    {PARAM_REVERB_MIX, "FDN Reverb Wet Mix", 0.0f, 1.0f, 0.30f},
    {PARAM_FILTER_CUTOFF, "Master Lowpass Cutoff", 200.0f, 18000.0f, 12000.0f}
};

struct clap_version_t {
    uint32_t major{1};
    uint32_t minor{1};
    uint32_t revision{10};
};

struct clap_plugin_entry_t {
    clap_version_t clap_version{1, 1, 10};
    bool (*init)(const char* plugin_path){nullptr};
    void (*deinit)(){nullptr};
    const void* (*get_factory)(const char* factory_id){nullptr};
};


inline size_t get_parameter_count() noexcept {
    return sizeof(PARAMETER_CATALOG) / sizeof(PARAMETER_CATALOG[0]);
}

inline const ParameterInfo* get_parameter_info(uint32_t id) noexcept {
    for (size_t i = 0; i < get_parameter_count(); ++i) {
        if (PARAMETER_CATALOG[i].id == id) {
            return &PARAMETER_CATALOG[i];
        }
    }
    return nullptr;
}

inline const clap_plugin_entry_t& get_entry() noexcept {
    static const clap_plugin_entry_t entry{
        .clap_version = {1, 1, 10},
        .init = [](const char*) noexcept -> bool { return true; },
        .deinit = []() noexcept {},
        .get_factory = [](const char*) noexcept -> const void* { return nullptr; }
    };
    return entry;
}

} // namespace aether::plugin
