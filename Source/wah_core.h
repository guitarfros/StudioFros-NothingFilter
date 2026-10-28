// wah_core.h  -  header-only wah filter core (no JUCE, no external deps)
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 GuitarFros
//
// Model: 2nd-order resonant band-pass (TPT / zero-delay-feedback state-variable
// filter) whose centre frequency follows an exponential pedal-position law and
// whose Q and gain follow the position, as in a typical inductor-based wah.
// Written from public DSP theory only.

#pragma once
#include <algorithm>
#include <cmath>

namespace wah {

constexpr double kPi = 3.14159265358979323846;

struct Params {
    double position = 0.5;   // 0..1  (0 = toe up / heel down, 1 = toe down)
    double fMin     = 300.0; // Hz at position 0
    double fMax     = 2000.0;// Hz at position 1
    double qLow     = 4.0;   // Q at position 0
    double qHigh    = 7.0;   // Q at position 1
    double peakDb   = 0.0;   // extra makeup in dB (Level pot)
    bool   vintage  = true;  // voicing switch
};

// Centre frequency: exponential sweep between fMin and fMax.
inline double centreFreq(const Params& p) {
    const double pos = std::clamp(p.position, 0.0, 1.0);
    return p.fMin * std::pow(p.fMax / p.fMin, pos);
}

// Q rises slightly with position (vintage) or stays constant (modern).
inline double qAt(const Params& p) {
    const double pos = std::clamp(p.position, 0.0, 1.0);
    if (!p.vintage) return 0.5 * (p.qLow + p.qHigh);
    return p.qLow + (p.qHigh - p.qLow) * pos;
}

class Channel {
public:
    void reset() { s1 = s2 = 0.0; }

    void setSampleRate(double sr) { fs = sr; }

    // Call once per block (or per sample if you smooth position yourself).
    void update(const Params& p) {
        double fc = std::clamp(centreFreq(p), 20.0, 0.45 * fs);
        const double Q = std::max(0.3, qAt(p));
        g   = std::tan(kPi * fc / fs);
        k   = 1.0 / Q;
        a1  = 1.0 / (1.0 + g * (g + k));
        a2  = g * a1;
        a3  = g * a2;
        // Normalise so the band-pass peak is ~0 dB, then apply Level.
        norm = std::pow(10.0, p.peakDb / 20.0);
    }

    inline double process(double x) {
        const double v3 = x - s2;
        const double v1 = a1 * s1 + a2 * v3;      // band-pass (scaled by 1/k below)
        const double v2 = s2 + a2 * s1 + a3 * v3; // low-pass
        s1 = 2.0 * v1 - s1;
        s2 = 2.0 * v2 - s2;
        return v1 * k * norm;                     // unity-gain band-pass at peak
    }

private:
    double fs = 48000.0;
    double g = 0, k = 1, a1 = 1, a2 = 0, a3 = 0, norm = 1;
    double s1 = 0, s2 = 0;
};

} // namespace wah
