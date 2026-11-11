#!/usr/bin/env bash
# AetherDSP Emscripten WebAssembly Build Script
set -e

echo "[WASM] Compiling AetherDSP to WebAssembly with SIMD and AudioWorklet support..."

if ! command -v emcc &> /dev/null; then
    echo "[ERROR] Emscripten compiler (emcc) not found in PATH."
    exit 1
fi

emcc src/c_api.cpp -Iinclude -std=c++20 -O3 -msimd128 \
    -s MODULARIZE=1 -s EXPORT_NAME="AetherDSPModule" \
    -s EXPORTED_FUNCTIONS="['_aether_engine_create','_aether_engine_destroy','_aether_engine_load_preset','_aether_engine_note_on','_aether_engine_note_off','_aether_engine_note_on_degree','_aether_engine_set_scale','_aether_engine_all_notes_off','_aether_engine_reset','_aether_engine_process_interleaved']" \
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
    -o web/aether_engine.js

echo "[WASM] Done! Generated web/aether_engine.js and web/aether_engine.wasm"
