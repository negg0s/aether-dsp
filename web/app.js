// AetherDSP Web Synthesizer & 3D Cymatics Visualizer
// Core: Non-Euclidean Hyperbolic Waveguide + 3D Kirchhoff-Love Plate Topography + FFT Scope

let audioCtx = null;
let masterGain = null;
let analyser = null;

// Presets
let currentPreset = 1;
const PRESET_BELL = 1;
const PRESET_GONG = 2;
const PRESET_CHAOS = 3;
const PRESET_AETHER = 4;

// ----------------------------------------------------
// 1. Audio Engine & Analyser
// ----------------------------------------------------
function initAudio() {
    if (!audioCtx) {
        const AudioContext = window.AudioContext || window.webkitAudioContext;
        audioCtx = new AudioContext();
        masterGain = audioCtx.createGain();
        masterGain.gain.setValueAtTime(0.45, audioCtx.currentTime);

        analyser = audioCtx.createAnalyser();
        analyser.fftSize = 1024;
        analyser.smoothingTimeConstant = 0.85;

        const shaper = audioCtx.createWaveShaper();
        shaper.curve = makeDistortionCurve(8);
        shaper.oversample = '2x';

        masterGain.connect(analyser);
        analyser.connect(shaper);
        shaper.connect(audioCtx.destination);

        document.getElementById('status-info').innerText =
            `[AUDIO ENGINE ACTIVE]: 48kHz | Non-Euclidean Physical Modeling Running`;
    }
    if (audioCtx.state === 'suspended') {
        audioCtx.resume();
    }
}

function makeDistortionCurve(amount) {
    const n_samples = 44100;
    const curve = new Float32Array(n_samples);
    const deg = Math.PI / 180;
    for (let i = 0; i < n_samples; ++i) {
        const x = (i * 2) / n_samples - 1;
        curve[i] = ((3 + amount) * x * 20 * deg) / (Math.PI + amount * Math.abs(x));
    }
    return curve;
}

function triggerAudio(nx, ny, velocity = 0.8, pitchMult = 1.0) {
    initAudio();
    if (!audioCtx) return;

    const now = audioCtx.currentTime;

    if (currentPreset === PRESET_BELL || currentPreset === PRESET_AETHER) {
        // Hyperbolic Non-Euclidean Inharmonic Bell
        const baseFreq = 220.0 * Math.pow(2, (nx + 1.0)) * pitchMult;
        const partials = [1.0, 1.414, 2.76, 4.12, 5.43, 7.82, 9.15];

        partials.forEach((ratio, idx) => {
            const osc = audioCtx.createOscillator();
            const gain = audioCtx.createGain();

            const freq = baseFreq * ratio;
            osc.frequency.setValueAtTime(freq, now);
            osc.type = (idx % 2 === 0) ? 'sine' : 'triangle';

            const decay = 2.8 / (1.0 + idx * 0.4);
            const amp = (velocity / (1.0 + idx * 0.5)) * 0.22;

            gain.gain.setValueAtTime(Math.max(amp, 0.0001), now);
            gain.gain.exponentialRampToValueAtTime(0.0001, now + decay);

            osc.connect(gain);
            gain.connect(masterGain);

            osc.start(now);
            osc.stop(now + decay);
        });
    } else if (currentPreset === PRESET_GONG) {
        // Chladni 2D Thin Plate Inharmonic Modal Spectrum (f_mn ~ m^2 + n^2)
        const baseFreq = 160.0 * pitchMult;
        const modes = [
            {m:1, n:2}, {m:2, n:1}, {m:2, n:3}, {m:3, n:1}, {m:3, n:2}, {m:1, n:4}, {m:4, n:2}
        ];

        modes.forEach((mode, idx) => {
            const osc = audioCtx.createOscillator();
            const gain = audioCtx.createGain();

            const m2n2 = mode.m * mode.m + mode.n * mode.n;
            const freq = baseFreq * (m2n2 / 5.0);

            const mx = mode.m * Math.PI * (nx * 0.5 + 0.5);
            const my = mode.n * Math.PI * (ny * 0.5 + 0.5);
            const nx_ang = mode.n * Math.PI * (nx * 0.5 + 0.5);
            const ny_ang = mode.m * Math.PI * (ny * 0.5 + 0.5);
            const coupling = Math.abs(Math.cos(mx)*Math.cos(my) - Math.cos(nx_ang)*Math.cos(ny_ang));

            osc.frequency.setValueAtTime(freq, now);
            osc.type = 'sine';

            const decay = 3.2 / (1.0 + m2n2 * 0.05);
            const amp = velocity * coupling * (0.28 / (1.0 + idx * 0.3));

            gain.gain.setValueAtTime(Math.max(amp, 0.0001), now);
            gain.gain.exponentialRampToValueAtTime(0.0001, now + decay);

            osc.connect(gain);
            gain.connect(masterGain);

            osc.start(now);
            osc.stop(now + decay);
        });
    } else if (currentPreset === PRESET_CHAOS) {
        // Chaotic Sub-Harmonic Lorenz Bed
        const osc = audioCtx.createOscillator();
        const filter = audioCtx.createBiquadFilter();
        const gain = audioCtx.createGain();

        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(65.41 * Math.pow(2, nx) * pitchMult, now);

        filter.type = 'lowpass';
        filter.frequency.setValueAtTime(800.0 + (ny + 1.0) * 1200.0, now);
        filter.Q.setValueAtTime(8.0, now);

        gain.gain.setValueAtTime(0.35, now);
        gain.gain.exponentialRampToValueAtTime(0.0001, now + 4.0);

        osc.connect(filter);
        filter.connect(gain);
        gain.connect(masterGain);

        osc.start(now);
        osc.stop(now + 4.0);
    }
}

// ----------------------------------------------------
// 2. Poincare Hyperbolic Waveguide Canvas
// ----------------------------------------------------
const canvasP = document.getElementById('canvas-poincare');
const ctxP = canvasP.getContext('2d');
const W_P = canvasP.width;
const H_P = canvasP.height;
const R_DISK = W_P * 0.46;
const CX_P = W_P / 2;
const CY_P = H_P / 2;

let waves = [];

function strikePoincare(nx, ny, pitchMult = 1.0) {
    const r2 = nx * nx + ny * ny;
    if (r2 < 0.96) {
        waves.push({
            x: nx,
            y: ny,
            radius: 0.02,
            amplitude: 1.0,
            speed: 0.016
        });
        triggerAudio(nx, ny, 0.9, pitchMult);
    }
}

function drawPoincare() {
    ctxP.fillStyle = '#05070a';
    ctxP.fillRect(0, 0, W_P, H_P);

    ctxP.beginPath();
    ctxP.arc(CX_P, CY_P, R_DISK, 0, Math.PI * 2);
    ctxP.strokeStyle = '#00f0ff';
    ctxP.lineWidth = 2;
    ctxP.stroke();

    ctxP.strokeStyle = 'rgba(0, 240, 255, 0.12)';
    ctxP.lineWidth = 1;

    for (let angle = 0; angle < Math.PI * 2; angle += Math.PI / 6) {
        ctxP.beginPath();
        ctxP.moveTo(CX_P, CY_P);
        ctxP.lineTo(CX_P + Math.cos(angle) * R_DISK, CY_P + Math.sin(angle) * R_DISK);
        ctxP.stroke();
    }

    [0.25, 0.5, 0.75, 0.9].forEach(r_norm => {
        ctxP.beginPath();
        ctxP.arc(CX_P, CY_P, R_DISK * r_norm, 0, Math.PI * 2);
        ctxP.stroke();
    });

    for (let i = waves.length - 1; i >= 0; --i) {
        const w = waves[i];
        w.radius += w.speed;
        w.amplitude *= 0.965;

        const r_center = Math.sqrt(w.x * w.x + w.y * w.y);
        const stretch = 1.0 / Math.max(0.1, 1.0 - r_center * r_center);

        const screenX = CX_P + w.x * R_DISK;
        const screenY = CY_P + w.y * R_DISK;
        const screenRadius = w.radius * R_DISK * stretch;

        ctxP.save();
        ctxP.beginPath();
        ctxP.arc(CX_P, CY_P, R_DISK - 1, 0, Math.PI * 2);
        ctxP.clip();

        ctxP.beginPath();
        ctxP.arc(screenX, screenY, screenRadius, 0, Math.PI * 2);
        ctxP.strokeStyle = `rgba(0, 240, 255, ${w.amplitude.toFixed(3)})`;
        ctxP.lineWidth = 3;
        ctxP.stroke();

        ctxP.restore();

        if (w.amplitude < 0.02) {
            waves.splice(i, 1);
        }
    }

    requestAnimationFrame(drawPoincare);
}

// ----------------------------------------------------
// 3. 2D Chladni Sand Simulation
// ----------------------------------------------------
const canvasC = document.getElementById('canvas-chladni');
const ctxC = canvasC.getContext('2d');
const W_C = canvasC.width;
const H_C = canvasC.height;

const NUM_PARTICLES = 6000;
const particles = [];
let chladni_m = 3;
let chladni_n = 2;
let plateVibration = 0.0;

for (let i = 0; i < NUM_PARTICLES; ++i) {
    particles.push({
        x: (Math.random() * 2 - 1) * 0.92,
        y: (Math.random() * 2 - 1) * 0.92,
        vx: 0,
        vy: 0
    });
}

function evaluateChladniField(x, y, m, n) {
    const mx = m * Math.PI * (x * 0.5 + 0.5);
    const my = n * Math.PI * (y * 0.5 + 0.5);
    const nx = n * Math.PI * (x * 0.5 + 0.5);
    const ny = m * Math.PI * (y * 0.5 + 0.5);
    return Math.cos(mx) * Math.cos(my) - Math.cos(nx) * Math.cos(ny);
}

function strikeChladni(nx, ny, pitchMult = 1.0) {
    chladni_m = Math.min(5, Math.max(1, Math.floor(Math.abs(nx) * 5) + 1));
    chladni_n = Math.min(5, Math.max(1, Math.floor(Math.abs(ny) * 5) + 1));
    if (chladni_m === chladni_n) chladni_n = (chladni_m % 5) + 1;

    plateVibration = 1.0;
    triggerAudio(nx, ny, 0.9, pitchMult);
}

function drawChladni() {
    ctxC.fillStyle = 'rgba(7, 9, 14, 0.35)';
    ctxC.fillRect(0, 0, W_C, H_C);

    ctxC.strokeStyle = 'rgba(255, 215, 0, 0.4)';
    ctxC.lineWidth = 2;
    ctxC.strokeRect(W_C * 0.04, H_C * 0.04, W_C * 0.92, H_C * 0.92);

    plateVibration *= 0.985;
    ctxC.fillStyle = '#ffd700';

    for (let i = 0; i < NUM_PARTICLES; ++i) {
        const p = particles[i];
        const delta = 0.01;
        const val = evaluateChladniField(p.x, p.y, chladni_m, chladni_n);
        const valX = evaluateChladniField(p.x + delta, p.y, chladni_m, chladni_n);
        const valY = evaluateChladniField(p.x, p.y + delta, chladni_m, chladni_n);

        const gradX = (Math.abs(valX) - Math.abs(val)) / delta;
        const gradY = (Math.abs(valY) - Math.abs(val)) / delta;

        const kick = plateVibration * 0.007 * Math.abs(val);
        p.vx = (p.vx - gradX * kick) * 0.85 + (Math.random() - 0.5) * kick * 0.2;
        p.vy = (p.vy - gradY * kick) * 0.85 + (Math.random() - 0.5) * kick * 0.2;

        p.x += p.vx;
        p.y += p.vy;

        if (p.x < -0.92) { p.x = -0.92; p.vx *= -0.5; }
        if (p.x > 0.92) { p.x = 0.92; p.vx *= -0.5; }
        if (p.y < -0.92) { p.y = -0.92; p.vy *= -0.5; }
        if (p.y > 0.92) { p.y = 0.92; p.vy *= -0.5; }

        const sx = (p.x * 0.5 + 0.5) * W_C;
        const sy = (p.y * 0.5 + 0.5) * H_C;
        ctxC.fillRect(sx, sy, 1.5, 1.5);
    }

    requestAnimationFrame(drawChladni);
}

// ----------------------------------------------------
// 4. 3D Isometric Plate Wireframe Topography
// ----------------------------------------------------
const canvas3D = document.getElementById('canvas-3d');
const ctx3D = canvas3D.getContext('2d');
const W_3D = canvas3D.width;
const H_3D = canvas3D.height;

const GRID_RES = 24;
let phase3D = 0.0;

function draw3DPlate() {
    ctx3D.fillStyle = '#05070a';
    ctx3D.fillRect(0, 0, W_3D, H_3D);

    phase3D += 0.08 * (1.0 + plateVibration * 2.0);

    const angleX = 0.65;
    const angleZ = 0.78;
    const cosX = Math.cos(angleX);
    const sinX = Math.sin(angleX);
    const cosZ = Math.cos(angleZ);
    const sinZ = Math.sin(angleZ);

    const scale = 140;
    const cx = W_3D / 2;
    const cy = H_3D / 2 + 30;

    autoProject = (x, y, z) => {
        // Isometric 3D rotation
        const rx = x * cosZ - y * sinZ;
        const ry = (x * sinZ + y * cosZ) * cosX - z * sinX;
        const rz = (x * sinZ + y * cosZ) * sinX + z * cosX;
        return {
            px: cx + rx * scale,
            py: cy + ry * scale,
            depth: rz
        };
    };

    const mesh = [];
    for (let gy = 0; gy <= GRID_RES; ++gy) {
        mesh[gy] = [];
        const ny = (gy / GRID_RES) * 2.0 - 1.0;
        for (let gx = 0; gx <= GRID_RES; ++gx) {
            const nx = (gx / GRID_RES) * 2.0 - 1.0;
            const field = evaluateChladniField(nx, ny, chladni_m, chladni_n);
            const z = field * 0.35 * Math.sin(phase3D) * (0.2 + plateVibration * 0.8);
            mesh[gy][gx] = autoProject(nx, ny, z);
        }
    }

    // Draw 3D Wireframe Grid
    for (let gy = 0; gy <= GRID_RES; ++gy) {
        for (let gx = 0; gx <= GRID_RES; ++gx) {
            const p = mesh[gy][gx];

            if (gx < GRID_RES) {
                const right = mesh[gy][gx + 1];
                ctx3D.beginPath();
                ctx3D.moveTo(p.px, p.py);
                ctx3D.lineTo(right.px, right.py);
                ctx3D.strokeStyle = 'rgba(0, 240, 255, 0.35)';
                ctx3D.lineWidth = 1;
                ctx3D.stroke();
            }

            if (gy < GRID_RES) {
                const down = mesh[gy + 1][gx];
                ctx3D.beginPath();
                ctx3D.moveTo(p.px, p.py);
                ctx3D.lineTo(down.px, down.py);
                ctx3D.strokeStyle = 'rgba(168, 85, 247, 0.35)';
                ctx3D.lineWidth = 1;
                ctx3D.stroke();
            }
        }
    }

    requestAnimationFrame(draw3DPlate);
}

// ----------------------------------------------------
// 5. Oscilloscope & FFT Spectrum Visualizer
// ----------------------------------------------------
const canvasScope = document.getElementById('canvas-scope');
const ctxScope = canvasScope.getContext('2d');
const W_S = canvasScope.width;
const H_S = canvasScope.height;

function drawScope() {
    ctxScope.fillStyle = '#080a10';
    ctxScope.fillRect(0, 0, W_S, H_S);

    if (!analyser) {
        requestAnimationFrame(drawScope);
        return;
    }

    const timeData = new Uint8Array(analyser.frequencyBinCount);
    analyser.getByteTimeDomainData(timeData);

    const freqData = new Uint8Array(analyser.frequencyBinCount);
    analyser.getByteFrequencyData(freqData);

    // 1. Draw Frequency Spectrum Bars in background
    const barWidth = (W_S / 128);
    for (let i = 0; i < 128; ++i) {
        const val = freqData[i] / 255.0;
        const barHeight = val * H_S * 0.75;
        ctxScope.fillStyle = `rgba(0, 240, 255, ${0.1 + val * 0.4})`;
        ctxScope.fillRect(i * barWidth, H_S - barHeight, barWidth - 1, barHeight);
    }

    // 2. Draw Oscilloscope Waveform on top
    ctxScope.lineWidth = 2;
    ctxScope.strokeStyle = '#00f0ff';
    ctxScope.beginPath();

    const sliceWidth = W_S / timeData.length;
    let x = 0;

    for (let i = 0; i < timeData.length; ++i) {
        const v = timeData[i] / 128.0;
        const y = v * H_S / 2;

        if (i === 0) ctxScope.moveTo(x, y);
        else ctxScope.lineTo(x, y);

        x += sliceWidth;
    }
    ctxScope.stroke();

    requestAnimationFrame(drawScope);
}

// ----------------------------------------------------
// 6. Virtual Piano Keyboard
// ----------------------------------------------------
const notes = [
    {note: 'C3', black: false, pitch: 0.5, key: 'a'},
    {note: 'C#3', black: true, pitch: 0.529, key: 'w'},
    {note: 'D3', black: false, pitch: 0.561, key: 's'},
    {note: 'D#3', black: true, pitch: 0.594, key: 'e'},
    {note: 'E3', black: false, pitch: 0.629, key: 'd'},
    {note: 'F3', black: false, pitch: 0.667, key: 'f'},
    {note: 'F#3', black: true, pitch: 0.707, key: 't'},
    {note: 'G3', black: false, pitch: 0.749, key: 'g'},
    {note: 'G#3', black: true, pitch: 0.793, key: 'y'},
    {note: 'A3', black: false, pitch: 0.840, key: 'h'},
    {note: 'A#3', black: true, pitch: 0.890, key: 'u'},
    {note: 'B3', black: false, pitch: 0.943, key: 'j'},
    {note: 'C4', black: false, pitch: 1.0, key: 'k'}
];

const pianoContainer = document.getElementById('piano-keys');
notes.forEach(n => {
    const el = document.createElement('div');
    el.className = `key ${n.black ? 'black' : ''}`;
    el.innerText = n.key.toUpperCase();
    el.dataset.pitch = n.pitch;
    el.addEventListener('pointerdown', () => {
        strikePoincare(0.3, 0.4, n.pitch);
        strikeChladni(0.3, 0.4, n.pitch);
    });
    pianoContainer.appendChild(el);
});

window.addEventListener('keydown', (e) => {
    const k = e.key.toLowerCase();
    const found = notes.find(n => n.key === k);
    if (found) {
        strikePoincare(0.3, 0.4, found.pitch);
        strikeChladni(0.3, 0.4, found.pitch);
    } else if (e.code === 'Space') {
        e.preventDefault();
        const rx = (Math.random() * 2 - 1) * 0.6;
        const ry = (Math.random() * 2 - 1) * 0.6;
        strikePoincare(rx, ry);
        strikeChladni(rx, ry);
    }
});

// ----------------------------------------------------
// 7. Interactive Handlers & Fibonacci Sonata Player
// ----------------------------------------------------
canvasP.addEventListener('pointerdown', (e) => {
    const rect = canvasP.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    const nx = (x / rect.width) * 2.0 - 1.0;
    const ny = (y / rect.height) * 2.0 - 1.0;
    strikePoincare(nx, ny);
});

canvasC.addEventListener('pointerdown', (e) => {
    const rect = canvasC.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    const nx = (x / rect.width) * 2.0 - 1.0;
    const ny = (y / rect.height) * 2.0 - 1.0;
    strikeChladni(nx, ny);
});

const buttons = [
    {id: 'btn-p1', p: PRESET_BELL},
    {id: 'btn-p2', p: PRESET_GONG},
    {id: 'btn-p3', p: PRESET_CHAOS},
    {id: 'btn-p4', p: PRESET_AETHER}
];

buttons.forEach(b => {
    document.getElementById(b.id).addEventListener('click', () => {
        buttons.forEach(btn => document.getElementById(btn.id).classList.remove('active'));
        document.getElementById(b.id).classList.add('active');
        currentPreset = b.p;
        triggerAudio(0.2, 0.3, 0.9);
    });
});

document.getElementById('btn-strike').addEventListener('click', () => {
    const rx = (Math.random() * 2 - 1) * 0.6;
    const ry = (Math.random() * 2 - 1) * 0.6;
    strikePoincare(rx, ry);
    strikeChladni(rx, ry);
});

document.getElementById('btn-fib').addEventListener('click', () => {
    initAudio();
    const fibNotes = [0.5, 0.561, 0.629, 0.749, 0.840, 1.0, 1.122, 1.259, 1.498];
    for (let i = 0; i < 16; ++i) {
        setTimeout(() => {
            const pitch = fibNotes[(i * 3) % fibNotes.length];
            const rx = Math.cos(i * 1.618 * Math.PI) * 0.5;
            const ry = Math.sin(i * 1.618 * Math.PI) * 0.5;
            strikePoincare(rx, ry, pitch);
            strikeChladni(rx, ry, pitch);
        }, i * 320);
    }
});

// Start All Render Loops
drawPoincare();
drawChladni();
draw3DPlate();
drawScope();
