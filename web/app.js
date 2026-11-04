// AetherDSP Web Visualizer & Real-Time Audio Synthesizer
// Math: Non-Euclidean Hyperbolic Waveguide + Chladni Modal Sand Simulation

let audioCtx = null;
let masterGain = null;

// Presets
let currentPreset = 1;
const PRESET_BELL = 1;
const PRESET_GONG = 2;
const PRESET_CHAOS = 3;
const PRESET_AETHER = 4;

// ----------------------------------------------------
// 1. Web Audio Core
// ----------------------------------------------------
function initAudio() {
    if (!audioCtx) {
        const AudioContext = window.AudioContext || window.webkitAudioContext;
        audioCtx = new AudioContext();
        masterGain = audioCtx.createGain();
        masterGain.gain.setValueAtTime(0.5, audioCtx.currentTime);

        // Soft clipper / waveshaper
        const shaper = audioCtx.createWaveShaper();
        shaper.curve = makeDistortionCurve(10);
        shaper.oversample = '2x';

        masterGain.connect(shaper);
        shaper.connect(audioCtx.destination);

        document.getElementById('status-info').innerText =
            `[AUDIO ENGINE READY]: Sample Rate ${audioCtx.sampleRate}Hz | WebAudio Running`;
    }
    if (audioCtx.state === 'suspended') {
        audioCtx.resume();
    }
}

function makeDistortionCurve(amount) {
    const k = amount;
    const n_samples = 44100;
    const curve = new Float32Array(n_samples);
    const deg = Math.PI / 180;
    for (let i = 0; i < n_samples; ++i) {
        const x = (i * 2) / n_samples - 1;
        curve[i] = ((3 + k) * x * 20 * deg) / (Math.PI + k * Math.abs(x));
    }
    return curve;
}

function triggerAudio(nx, ny, velocity = 0.8) {
    initAudio();
    if (!audioCtx) return;

    const now = audioCtx.currentTime;

    if (currentPreset === PRESET_BELL || currentPreset === PRESET_AETHER) {
        // Hyperbolic Non-Euclidean Inharmonic Bell
        const baseFreq = 220.0 * Math.pow(2, (nx + 1.0));
        const partials = [1.0, 1.414, 2.76, 4.12, 5.43, 7.82, 9.15];

        partials.forEach((ratio, idx) => {
            const osc = audioCtx.createOscillator();
            const gain = audioCtx.createGain();

            // Exponential dispersion characteristic of negative curvature
            const freq = baseFreq * ratio;
            osc.frequency.setValueAtTime(freq, now);
            osc.type = (idx % 2 === 0) ? 'sine' : 'triangle';

            const decay = 2.5 / (1.0 + idx * 0.4);
            const amp = (velocity / (1.0 + idx * 0.5)) * 0.25;

            gain.gain.setValueAtTime(amp, now);
            gain.gain.exponentialRampToValueAtTime(0.0001, now + decay);

            osc.connect(gain);
            gain.connect(masterGain);

            osc.start(now);
            osc.stop(now + decay);
        });
    } else if (currentPreset === PRESET_GONG) {
        // Chladni 2D Thin Plate Inharmonic Modal Spectrum (f_mn ~ m^2 + n^2)
        const baseFreq = 160.0;
        const modes = [
            {m:1, n:2}, {m:2, n:1}, {m:2, n:3}, {m:3, n:1}, {m:3, n:2}, {m:1, n:4}, {m:4, n:2}
        ];

        modes.forEach((mode, idx) => {
            const osc = audioCtx.createOscillator();
            const gain = audioCtx.createGain();

            // Exact Kirchhoff-Love plate dispersion
            const m2n2 = mode.m * mode.m + mode.n * mode.n;
            const freq = baseFreq * (m2n2 / 5.0);

            // Spatial modal coupling at strike coordinate
            const mx = mode.m * Math.PI * (nx * 0.5 + 0.5);
            const my = mode.n * Math.PI * (ny * 0.5 + 0.5);
            const nx_ang = mode.n * Math.PI * (nx * 0.5 + 0.5);
            const ny_ang = mode.m * Math.PI * (ny * 0.5 + 0.5);
            const coupling = Math.abs(Math.cos(mx)*Math.cos(my) - Math.cos(nx_ang)*Math.cos(ny_ang));

            osc.frequency.setValueAtTime(freq, now);
            osc.type = 'sine';

            const decay = 3.5 / (1.0 + m2n2 * 0.05);
            const amp = velocity * coupling * (0.3 / (1.0 + idx * 0.3));

            gain.gain.setValueAtTime(Math.max(amp, 0.0001), now);
            gain.gain.exponentialRampToValueAtTime(0.0001, now + decay);

            osc.connect(gain);
            gain.connect(masterGain);

            osc.start(now);
            osc.stop(now + decay);
        });
    } else if (currentPreset === PRESET_CHAOS) {
        // Chaotic Sub-Harmonic Drone
        const osc = audioCtx.createOscillator();
        const filter = audioCtx.createBiquadFilter();
        const gain = audioCtx.createGain();

        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(65.41 * Math.pow(2, nx), now); // C2 base

        filter.type = 'lowpass';
        filter.frequency.setValueAtTime(800.0 + (ny + 1.0) * 1200.0, now);
        filter.Q.setValueAtTime(8.0, now);

        gain.gain.setValueAtTime(0.4, now);
        gain.gain.exponentialRampToValueAtTime(0.0001, now + 4.0);

        osc.connect(filter);
        filter.connect(gain);
        gain.connect(masterGain);

        osc.start(now);
        osc.stop(now + 4.0);
    }
}

// ----------------------------------------------------
// 2. Poincare Hyperbolic Waveguide Canvas Simulation
// ----------------------------------------------------
const canvasP = document.getElementById('canvas-poincare');
const ctxP = canvasP.getContext('2d');
const W_P = canvasP.width;
const H_P = canvasP.height;
const R_DISK = W_P * 0.46;
const CX_P = W_P / 2;
const CY_P = H_P / 2;

// Wave propagation ripples in Poincare disk
let waves = [];

function strikePoincare(nx, ny) {
    const r2 = nx * nx + ny * ny;
    if (r2 < 0.96) {
        waves.push({
            x: nx,
            y: ny,
            radius: 0.02,
            amplitude: 1.0,
            speed: 0.015
        });
        triggerAudio(nx, ny, 0.9);
    }
}

function drawPoincare() {
    ctxP.fillStyle = '#05070a';
    ctxP.fillRect(0, 0, W_P, H_P);

    // Draw Boundary of Unit Disk
    ctxP.beginPath();
    ctxP.arc(CX_P, CY_P, R_DISK, 0, Math.PI * 2);
    ctxP.strokeStyle = '#00f0ff';
    ctxP.lineWidth = 2;
    ctxP.stroke();

    // Hyperbolic Geodesic Grid (Orthogonal Circular Arcs)
    ctxP.strokeStyle = 'rgba(0, 240, 255, 0.12)';
    ctxP.lineWidth = 1;

    for (let angle = 0; angle < Math.PI * 2; angle += Math.PI / 6) {
        ctxP.beginPath();
        ctxP.moveTo(CX_P, CY_P);
        ctxP.lineTo(CX_P + Math.cos(angle) * R_DISK, CY_P + Math.sin(angle) * R_DISK);
        ctxP.stroke();
    }

    // Geodesic circles (horocycles)
    [0.25, 0.5, 0.75, 0.9].forEach(r_norm => {
        ctxP.beginPath();
        ctxP.arc(CX_P, CY_P, R_DISK * r_norm, 0, Math.PI * 2);
        ctxP.stroke();
    });

    // Update and draw Non-Euclidean Wavefronts
    for (let i = waves.length - 1; i >= 0; --i) {
        const w = waves[i];
        w.radius += w.speed;
        w.amplitude *= 0.965;

        // In Poincare disk, as waves move outward, conformal factor stretches them
        const r_center = Math.sqrt(w.x * w.x + w.y * w.y);
        const stretch = 1.0 / Math.max(0.1, 1.0 - r_center * r_center);

        const screenX = CX_P + w.x * R_DISK;
        const screenY = CY_P + w.y * R_DISK;
        const screenRadius = w.radius * R_DISK * stretch;

        ctxP.save();
        ctxP.beginPath();
        ctxP.arc(CX_P, CY_P, R_DISK - 1, 0, Math.PI * 2);
        ctxP.clip(); // Keep inside disk

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
// 3. 2D Chladni Nodal Plate Sand Simulation
// ----------------------------------------------------
const canvasC = document.getElementById('canvas-chladni');
const ctxC = canvasC.getContext('2d');
const W_C = canvasC.width;
const H_C = canvasC.height;

const NUM_PARTICLES = 5000;
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

function strikeChladni(nx, ny) {
    // Select modes based on strike position
    chladni_m = Math.min(5, Math.max(1, Math.floor(Math.abs(nx) * 5) + 1));
    chladni_n = Math.min(5, Math.max(1, Math.floor(Math.abs(ny) * 5) + 1));
    if (chladni_m === chladni_n) chladni_n = (chladni_m % 5) + 1;

    plateVibration = 1.0;
    triggerAudio(nx, ny, 0.9);
}

function drawChladni() {
    ctxC.fillStyle = 'rgba(10, 12, 16, 0.35)';
    ctxC.fillRect(0, 0, W_C, H_C);

    // Plate Boundary
    ctxC.strokeStyle = 'rgba(255, 215, 0, 0.4)';
    ctxC.lineWidth = 2;
    ctxC.strokeRect(W_C * 0.04, H_C * 0.04, W_C * 0.92, H_C * 0.92);

    plateVibration *= 0.985;

    // Simulate Sand Particles moving to Nodal Lines (where field is 0)
    ctxC.fillStyle = '#ffd700';

    for (let i = 0; i < NUM_PARTICLES; ++i) {
        const p = particles[i];

        // Compute local gradient of vibration energy
        const delta = 0.01;
        const val = evaluateChladniField(p.x, p.y, chladni_m, chladni_n);
        const valX = evaluateChladniField(p.x + delta, p.y, chladni_m, chladni_n);
        const valY = evaluateChladniField(p.x, p.y + delta, chladni_m, chladni_n);

        const gradX = (Math.abs(valX) - Math.abs(val)) / delta;
        const gradY = (Math.abs(valY) - Math.abs(val)) / delta;

        // Sand particles are kicked away from vibrating zones towards nodal lines (grad downhill)
        const kick = plateVibration * 0.006 * Math.abs(val);
        p.vx = (p.vx - gradX * kick) * 0.85 + (Math.random() - 0.5) * kick * 0.2;
        p.vy = (p.vy - gradY * kick) * 0.85 + (Math.random() - 0.5) * kick * 0.2;

        p.x += p.vx;
        p.y += p.vy;

        // Plate boundaries bounce
        if (p.x < -0.92) { p.x = -0.92; p.vx *= -0.5; }
        if (p.x > 0.92) { p.x = 0.92; p.vx *= -0.5; }
        if (p.y < -0.92) { p.y = -0.92; p.vy *= -0.5; }
        if (p.y > 0.92) { p.y = 0.92; p.vy *= -0.5; }

        // Render sand grain
        const sx = (p.x * 0.5 + 0.5) * W_C;
        const sy = (p.y * 0.5 + 0.5) * H_C;

        ctxC.fillRect(sx, sy, 1.5, 1.5);
    }

    requestAnimationFrame(drawChladni);
}

// ----------------------------------------------------
// 4. User Interaction & Handlers
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

// Preset Buttons
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

window.addEventListener('keydown', (e) => {
    if (e.code === 'Space') {
        e.preventDefault();
        const rx = (Math.random() * 2 - 1) * 0.6;
        const ry = (Math.random() * 2 - 1) * 0.6;
        strikePoincare(rx, ry);
        strikeChladni(rx, ry);
    }
});

// Start Render Loops
drawPoincare();
drawChladni();
