#include "aether/c_api.h"
#include "aether/engine.hpp"
#include <vector>

extern "C" {

AETHER_API aether_engine_handle_t aether_engine_create(uint32_t sample_rate) {
    try {
        auto* engine = new aether::Engine(sample_rate);
        return static_cast<aether_engine_handle_t>(engine);
    } catch (...) {
        return nullptr;
    }
}

AETHER_API void aether_engine_destroy(aether_engine_handle_t handle) {
    if (!handle) return;
    auto* engine = static_cast<aether::Engine*>(handle);
    delete engine;
}

AETHER_API void aether_engine_load_preset(aether_engine_handle_t handle, int preset_id) {
    if (!handle) return;
    auto* engine = static_cast<aether::Engine*>(handle);
    switch (preset_id) {
        case AETHER_PRESET_HYPERBOLIC_BELL:
            engine->load_preset(aether::Preset::HyperbolicCosmicBell);
            break;
        case AETHER_PRESET_CHLADNI_GONG:
            engine->load_preset(aether::Preset::ChladniPlateGong);
            break;
        case AETHER_PRESET_CHAOTIC_DRONE:
            engine->load_preset(aether::Preset::ChaoticQuantumDrone);
            break;
        case AETHER_PRESET_AETHER_SPACE:
            engine->load_preset(aether::Preset::HyperbolicAetherSpace);
            break;
        case AETHER_PRESET_BOWED_CRYSTAL:
            engine->load_preset(aether::Preset::BowedCrystalPlate);
            break;
        case AETHER_PRESET_VON_KARMAN_GONG:
            engine->load_preset(aether::Preset::VonKarmanTurbulentGong);
            break;
        case AETHER_PRESET_PSEUDOSPHERE_HORN:
            engine->load_preset(aether::Preset::PseudosphereTractrixHorn);
            break;
        case AETHER_PRESET_HENON_HEILES_CHAOS:
            engine->load_preset(aether::Preset::HenonHeilesChaos);
            break;
        case AETHER_PRESET_HYPERBOLIC_STRING:
            engine->load_preset(aether::Preset::HyperbolicStringJawari);
            break;
        case AETHER_PRESET_SINE_GORDON_BREATHER:
            engine->load_preset(aether::Preset::SineGordonBreather);
            break;
        case AETHER_PRESET_ACOUSTIC_BLACK_HOLE:
            engine->load_preset(aether::Preset::AcousticBlackHole);
            break;
        case AETHER_PRESET_ACOUSTIC_WORMHOLE:
            engine->load_preset(aether::Preset::AcousticWormhole);
            break;
        case AETHER_PRESET_TOPOLOGICAL_METAMATERIAL:
            engine->load_preset(aether::Preset::TopologicalMetamaterial);
            break;
        case AETHER_PRESET_KDV_SOLITON_LATTICE:
            engine->load_preset(aether::Preset::KdvSolitonLattice);
            break;
        default:
            engine->load_preset(aether::Preset::HyperbolicCosmicBell);
            break;
    }
}

AETHER_API void aether_engine_note_on(aether_engine_handle_t handle, float strike_x, float strike_y, float velocity) {
    if (!handle) return;
    auto* engine = static_cast<aether::Engine*>(handle);
    engine->note_on({strike_x, strike_y}, velocity);
}

AETHER_API void aether_engine_note_off(aether_engine_handle_t handle) {
    if (!handle) return;
    auto* engine = static_cast<aether::Engine*>(handle);
    engine->note_off();
}

AETHER_API void aether_engine_process_interleaved(aether_engine_handle_t handle, float* interleaved_out, size_t num_frames) {
    if (!handle || !interleaved_out || num_frames == 0) return;
    auto* engine = static_cast<aether::Engine*>(handle);

    constexpr size_t CHUNK_SIZE = 64;
    std::vector<aether::StereoFrame> chunk(CHUNK_SIZE);

    size_t processed = 0;
    while (processed < num_frames) {
        const size_t to_render = std::min(CHUNK_SIZE, num_frames - processed);
        engine->process_block(std::span<aether::StereoFrame>(chunk.data(), to_render));

        for (size_t i = 0; i < to_render; ++i) {
            interleaved_out[(processed + i) * 2 + 0] = chunk[i].left;
            interleaved_out[(processed + i) * 2 + 1] = chunk[i].right;
        }

        processed += to_render;
    }
}

} // extern "C"
