#pragma once

#include "../types.hpp"
#include "../engine.hpp"
#include <cstdint>
#include <array>
#include <cstring>

namespace aether::embedded {

/**
 * @brief Zero-allocation hardware control state for embedded synthesizer boards
 * (e.g. Raspberry Pi RP2040, Raspberry Pi 4/5, Daisy Seed, Teensy 4.1).
 * Maps 8 analog potentiometer ADC inputs, 4 digital encoder buttons, and MIDI.
 */
struct HardwareKnobMapping {
    uint8_t target_param{0};
    float min_val{0.0f};
    float max_val{1.0f};
    float current_val{0.0f};
};

class EmbeddedHardwareAdapter {
public:
    static constexpr size_t NUM_KNOBS = 8;
    static constexpr size_t NUM_BUTTONS = 4;

    EmbeddedHardwareAdapter() noexcept {
        init_default_mappings();
    }

    void init_default_mappings() noexcept {
        // Knob 0: Preset selector (0 to 13)
        mappings_[0] = {0, 0.0f, 13.0f, 0.0f};
        // Knob 1: Macro parameter 1 (timbre/stiffness)
        mappings_[1] = {1, 0.0f, 1.0f, 0.5f};
        // Knob 2: Macro parameter 2 (decay/damping)
        mappings_[2] = {2, 0.0f, 1.0f, 0.5f};
        // Knob 3: Reverb Wet mix
        mappings_[3] = {3, 0.0f, 1.0f, 0.35f};
        // Knob 4: Spatializer / Panning azimuth (-180 to 180 deg)
        mappings_[4] = {4, -180.0f, 180.0f, 0.0f};
        // Knob 5: Filter cutoff (20Hz to 18000Hz)
        mappings_[5] = {5, 20.0f, 18000.0f, 12000.0f};
        // Knob 6: Master volume (0.0 to 1.0)
        mappings_[6] = {6, 0.0f, 1.0f, 0.8f};
        // Knob 7: Tuning degree / scale offset (0 to 12)
        mappings_[7] = {7, 0.0f, 12.0f, 0.0f};
    }

    /**
     * @brief Ingest 12-bit ADC raw integer value [0, 4095] from hardware potentiometer.
     */
    void update_adc_knob(size_t knob_index, uint16_t adc_12bit_raw, Engine& engine) noexcept {
        if (knob_index >= NUM_KNOBS) return;
        adc_12bit_raw = std::min<uint16_t>(adc_12bit_raw, 4095);
        const float norm = static_cast<float>(adc_12bit_raw) / 4095.0f;

        auto& mapping = mappings_[knob_index];
        const float scaled = mapping.min_val + norm * (mapping.max_val - mapping.min_val);
        mapping.current_val = scaled;

        // Apply parameter to engine
        if (knob_index == 0) {
            const int preset_id = static_cast<int>(std::round(scaled));
            engine.set_preset(static_cast<Preset>(std::clamp(preset_id, 0, 13)));
        } else if (knob_index == 3) {
            engine.reverb().configure(0.85f, 0.3f, scaled);
        } else if (knob_index == 4) {
            // Map knob 4 to 3D spatializer position azimuth
            const float rad = scaled * static_cast<float>(constants::PI / 180.0);
            engine.spatializer3d().set_source_position({std::cos(rad) * 0.5f, std::sin(rad) * 0.5f, 0.0f});
        }
    }

    /**
     * @brief Handle digital momentary switch push.
     */
    void handle_button_press(size_t button_index, bool pressed, Engine& engine) noexcept {
        if (button_index >= NUM_BUTTONS) return;
        if (pressed && !button_states_[button_index]) {
            // Button rising edge event
            if (button_index == 0) {
                // Trigger strike / note on
                engine.note_on({0.2f, 0.3f}, 0.85f);
            } else if (button_index == 1) {
                // Trigger note off
                engine.note_off();
            } else if (button_index == 2) {
                // Next preset
                auto next_p = static_cast<int>(engine.current_preset()) + 1;
                if (next_p > 13) next_p = 0;
                engine.set_preset(static_cast<Preset>(next_p));
            } else if (button_index == 3) {
                // Reset engine to silence
                engine.reset();
            }
        }
        button_states_[button_index] = pressed;
    }

    [[nodiscard]] float knob_value(size_t index) const noexcept {
        if (index >= NUM_KNOBS) return 0.0f;
        return mappings_[index].current_val;
    }

    [[nodiscard]] bool button_state(size_t index) const noexcept {
        if (index >= NUM_BUTTONS) return false;
        return button_states_[index];
    }

private:
    std::array<HardwareKnobMapping, NUM_KNOBS> mappings_{};
    std::array<bool, NUM_BUTTONS> button_states_{false, false, false, false};
};

} // namespace aether::embedded
