use aether_dsp::{AetherEngine, Preset};

fn main() -> Result<(), &'static str> {
    println!("=== AetherDSP Rust FFI Example ===");
    let sample_rate = 48000;
    let mut engine = AetherEngine::new(sample_rate)?;

    println!("Engine created. Triggering note on Poincare Conformal Disk...");
    engine.load_preset(Preset::HyperbolicBell);
    engine.note_on(0.35, 0.45, 0.85);

    let num_frames = sample_rate as usize * 2; // 2 seconds
    let mut buffer = vec![0.0f32; num_frames * 2];

    engine.process_interleaved(&mut buffer);

    println!("Successfully rendered {} stereo frames through Rust FFI!", num_frames);
    Ok(())
}
