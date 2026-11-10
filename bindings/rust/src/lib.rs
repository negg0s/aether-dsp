//! Safe idiomatic Rust wrapper for AetherDSP Non-Euclidean Physical Modeling Engine.

use std::os::raw::{c_float, c_int, c_void};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Preset {
    HyperbolicBell = 0,
    ChladniGong = 1,
    ChaoticDrone = 2,
    AetherSpace = 3,
    BowedCrystal = 4,
    VonKarmanGong = 5,
    PseudosphereHorn = 6,
    HenonHeilesChaos = 7,
    HyperbolicString = 8,
    SineGordonBreather = 9,
    AcousticBlackHole = 10,
    AcousticWormhole = 11,
    TopologicalMetamaterial = 12,
    KdvSolitonLattice = 13,
}

#[link(name = "aether-dsp")]
extern "C" {
    fn aether_engine_create(sample_rate: u32) -> *mut c_void;
    fn aether_engine_destroy(handle: *mut c_void);
    fn aether_engine_load_preset(handle: *mut c_void, preset_id: c_int);
    fn aether_engine_note_on(handle: *mut c_void, strike_x: c_float, strike_y: c_float, velocity: c_float);
    fn aether_engine_note_off(handle: *mut c_void);
    fn aether_engine_process_interleaved(handle: *mut c_void, out: *mut c_float, num_frames: usize);
}

/// AetherDSP Safe Audio Engine Handle
pub struct AetherEngine {
    handle: *mut c_void,
    sample_rate: u32,
}

impl AetherEngine {
    /// Creates a new AetherDSP synthesis engine instance
    pub fn new(sample_rate: u32) -> Result<Self, &'static str> {
        let handle = unsafe { aether_engine_create(sample_rate) };
        if handle.is_null() {
            Err("Failed to allocate AetherDSP native engine instance")
        } else {
            Ok(Self { handle, sample_rate })
        }
    }

    /// Loads a non-Euclidean preset
    pub fn load_preset(&mut self, preset: Preset) {
        unsafe {
            aether_engine_load_preset(self.handle, preset as c_int);
        }
    }

    /// Triggers note on Poincare disk or modal plate
    pub fn note_on(&mut self, strike_x: f32, strike_y: f32, velocity: f32) {
        unsafe {
            aether_engine_note_on(self.handle, strike_x, strike_y, velocity);
        }
    }

    /// Releases current note envelope
    pub fn note_off(&mut self) {
        unsafe {
            aether_engine_note_off(self.handle);
        }
    }

    /// Renders interleaved stereo frames [L0, R0, L1, R1, ...] into slice
    pub fn process_interleaved(&mut self, output: &mut [f32]) {
        let num_frames = output.len() / 2;
        unsafe {
            aether_engine_process_interleaved(self.handle, output.as_mut_ptr(), num_frames);
        }
    }

    pub fn sample_rate(&self) -> u32 {
        self.sample_rate
    }
}

impl Drop for AetherEngine {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                aether_engine_destroy(self.handle);
            }
        }
    }
}

unsafe impl Send for AetherEngine {}
