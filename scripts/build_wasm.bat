@echo off
REM AetherDSP Emscripten WebAssembly Build Script
echo [WASM] Compiling AetherDSP to WebAssembly with SIMD and AudioWorklet support...

where emcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Emscripten compiler (emcc) not found in PATH.
    echo Install Emscripten via emsdk: https://emscripten.org/docs/getting_started/downloads.html
    exit /b 1
)

emcc src/c_api.cpp -Iinclude -std=c++20 -O3 -msimd128 -s MODULARIZE=1 -s EXPORT_NAME="AetherDSPModule" -s EXPORTED_FUNCTIONS="['_aether_engine_create','_aether_engine_destroy','_aether_engine_load_preset','_aether_engine_note_on','_aether_engine_note_off','_aether_engine_process_interleaved']" -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" -o web/aether_engine.js

echo [WASM] Done! Generated web/aether_engine.js and web/aether_engine.wasm
