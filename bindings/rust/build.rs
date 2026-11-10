fn main() {
    println!("cargo:rerun-if-env-changed=AETHER_DSP_LIB_DIR");
    if let Ok(dir) = std::env::var("AETHER_DSP_LIB_DIR") {
        println!("cargo:rustc-link-search=native={}", dir);
    } else {
        println!("cargo:rustc-link-search=native=../../build");
        println!("cargo:rustc-link-search=native=../build");
        println!("cargo:rustc-link-search=native=.");
    }
    println!("cargo:rustc-link-lib=dylib=aether-dsp");
}
