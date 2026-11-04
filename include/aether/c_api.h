#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
    #if defined(AETHER_BUILD_DLL)
        #define AETHER_API __declspec(dllexport)
    #else
        #define AETHER_API
    #endif
#else
    #define AETHER_API __attribute__((visibility("default")))
#endif

typedef void* aether_engine_handle_t;

enum aether_preset_t {
    AETHER_PRESET_HYPERBOLIC_BELL = 0,
    AETHER_PRESET_CHLADNI_GONG = 1,
    AETHER_PRESET_CHAOTIC_DRONE = 2,
    AETHER_PRESET_AETHER_SPACE = 3
};

/**
 * @brief Create a new AetherDSP engine instance.
 */
AETHER_API aether_engine_handle_t aether_engine_create(uint32_t sample_rate);

/**
 * @brief Destroy engine instance.
 */
AETHER_API void aether_engine_destroy(aether_engine_handle_t handle);

/**
 * @brief Load one of the non-Euclidean presets.
 */
AETHER_API void aether_engine_load_preset(aether_engine_handle_t handle, int preset_id);

/**
 * @brief Strike or excite the physical modeling surface.
 * @param strike_x X coordinate inside [-1.0, 1.0]
 * @param strike_y Y coordinate inside [-1.0, 1.0]
 * @param velocity Note velocity (0.0 to 1.0)
 */
AETHER_API void aether_engine_note_on(aether_engine_handle_t handle, float strike_x, float strike_y, float velocity);

/**
 * @brief Release current note envelope.
 */
AETHER_API void aether_engine_note_off(aether_engine_handle_t handle);

/**
 * @brief Render interleaved stereo floating-point samples.
 * @param interleaved_out Output buffer of size (num_frames * 2) floats
 * @param num_frames Number of stereo frames to render
 */
AETHER_API void aether_engine_process_interleaved(aether_engine_handle_t handle, float* interleaved_out, size_t num_frames);

#ifdef __cplusplus
}
#endif
