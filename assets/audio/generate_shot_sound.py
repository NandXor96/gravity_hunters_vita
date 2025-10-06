# Sounds like shit rn, but better than nothing :D
import numpy as np, soundfile as sf, os, math

sr = 44100

def onepole_lowpass(x, cutoff_hz, sr=44100):
    # simple lowpass; cutoff_hz in (0, sr/2)
    x = np.asarray(x, dtype=np.float32)
    y = np.zeros_like(x)
    # convert cutoff to alpha
    # alpha = 1 - exp(-2*pi*fc/sr)
    alpha = 1.0 - math.exp(-2.0*math.pi*cutoff_hz/sr)
    for i in range(1, len(x)):
        y[i] = y[i-1] + alpha*(x[i]-y[i-1])
    return y

def onepole_highpass(x, cutoff_hz, sr=44100):
    # simple highpass via onepole + subtract
    x = np.asarray(x, dtype=np.float32)
    lp = onepole_lowpass(x, cutoff_hz, sr)
    return x - lp

def sample_hold_bitcrush(x, hold_samples=4):
    if hold_samples <= 1:
        return x
    y = np.copy(x)
    for i in range(0, len(x), hold_samples):
        y[i:i+hold_samples] = x[i]
    return y

def normalize(sig, headroom=0.92):
    peak = np.max(np.abs(sig)) + 1e-12
    return headroom * sig / peak

def save_ogg(filename, sig, sr=44100):
    ogg_path = os.path.join(".", filename)
    sf.write(ogg_path, sig.astype(np.float32), sr, format="OGG", subtype="VORBIS")
    return ogg_path

def synth_shot(
    dur=0.36, start_f=1100.0, end_f=240.0,
    attack=0.0025, hold=0.01, decay=0.18, release=0.12,
    body_mix=(0.55, 0.45),  # sine, softsquare
    click_amt=0.5, click_hp=1800,  # bright initial click
    noise_amt=0.10, noise_hp=1000,
    crush=3, dist=2.2,
    tail_lp_end=2200, # end-of-tail lowpass cutoff (start higher automatically)
    vib_freq=0.0, vib_depth=0.0
):
    n = int(sr*dur); t = np.arange(n)/sr
    # exponential glide
    f = start_f * (end_f/start_f) ** (t/t[-1])
    if vib_freq>0 and vib_depth>0:
        f = f * (1.0 + vib_depth*np.sin(2*np.pi*vib_freq*t))
    phase = 2*np.pi*np.cumsum(f)/sr
    sine = np.sin(phase)
    softsquare = np.tanh(3.0*np.sin(phase))
    tone = body_mix[0]*sine + body_mix[1]*softsquare

    # ADRH envelope (short hold for a snappier body)
    a = int(sr*attack)
    h = a + int(sr*hold)
    d = h + int(sr*decay)
    r = n
    env = np.zeros(n, dtype=np.float32)
    if a>0: env[:a] = np.linspace(0, 1, a, endpoint=False)
    if h>a: env[a:h] = 1.0
    if d>h: env[h:d] = np.linspace(1.0, 0.55, d-h, endpoint=False)
    if r>d: env[d:r] = np.linspace(0.55, 0.0, r-d, endpoint=True)
    env = env**1.2

    # Click: tiny exponential-decay impulse, highpassed
    click = np.zeros(n, dtype=np.float32)
    click_len = int(0.01*sr)
    click[:click_len] = np.exp(-np.linspace(0, 8, click_len)).astype(np.float32)
    click = onepole_highpass(click, click_hp, sr)
    click *= click_amt

    # Short noise burst, mostly HF
    rng = np.random.default_rng(123)
    noise = rng.standard_normal(n).astype(np.float32)
    noise = onepole_highpass(noise, noise_hp, sr)
    noise_env = np.exp(-t/0.04)
    noise *= noise_env * noise_amt

    dry = tone*env + click + noise

    # Slight time-varying lowpass (open at the start, closes toward tail)
    # interpolate cutoff from 6000 Hz -> tail_lp_end
    start_cut = 6000.0
    cut_series = start_cut + (tail_lp_end - start_cut)*(t/t[-1])
    # apply per-sample one-pole with changing alpha
    y = np.zeros_like(dry)
    prev = 0.0
    for i, x in enumerate(dry):
        alpha = 1.0 - math.exp(-2.0*math.pi*cut_series[i]/sr)
        prev = prev + alpha*(x - prev)
        y[i] = prev

    # Bitcrush (sample & hold) then soft clip for bite
    crushed = sample_hold_bitcrush(y, crush)
    wet = np.tanh(dist * crushed)

    return normalize(wet, 0.92)

# --- Variant A ---
a = synth_shot(
    dur=0.20, start_f=1100, end_f=280,
    attack=0.002, hold=0.012, decay=0.18, release=0.12,
    body_mix=(0.70, 0.30), click_amt=0.2, noise_amt=0.08,
    crush=1, dist=1.8, tail_lp_end=1200
)
path_shot2 = save_ogg("shot2.ogg", a, sr)

# --- Variant B ---
c = synth_shot(
    dur=0.20, start_f=1200, end_f=180,
    attack=0.002, hold=0.010, decay=0.16, release=0.14,
    body_mix=(0.40, 0.60), click_amt=0.2, noise_amt=0.10,
    crush=1, dist=1.4, tail_lp_end=900
)
path_shot = save_ogg("shot.ogg", c, sr)

path_shot2, path_shot
