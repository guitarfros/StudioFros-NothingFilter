// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 GuitarFros
#pragma once

namespace nf
{
// One factory preset. Values are real units:
//   position 0..1, q 2..12, range 1200..3500 Hz (upper sweep limit), level -12..+12 dB,
//   mode 0 = Fixed / 1 = Pedal (MIDI CC), voicing 0 = Vintage / 1 = Modern.
struct FactoryPreset
{
    const char* name;
    float position, q, range, level;
    int mode, voicing;
};

inline constexpr FactoryPreset kFactoryPresets[] =
{
    { "Half Open",        0.50f,  5.0f, 2000.0f,  0.0f, 0, 0 },   // the reference "half-pressed" sound
    { "Half Open Nasal",  0.50f,  7.5f, 2000.0f,  1.5f, 0, 0 },
    { "Low Quack",        0.30f,  6.0f, 1800.0f,  1.0f, 0, 0 },
    { "Cocked Mid",       0.62f,  5.5f, 2200.0f,  0.0f, 0, 0 },
    { "Vocal Open",       0.75f,  4.5f, 2600.0f, -1.0f, 0, 0 },
    { "Toe Down Bright",  0.95f,  4.0f, 3200.0f, -2.0f, 0, 0 },
    { "Heel Down Dark",   0.08f,  3.5f, 1500.0f,  2.0f, 0, 0 },
    { "Funk Chop",        0.45f,  8.5f, 2400.0f,  0.0f, 0, 1 },
    { "Subtle Filter",    0.55f,  2.8f, 2000.0f,  0.0f, 0, 1 },
    { "Sharp Peak",       0.60f, 10.5f, 2800.0f, -2.0f, 0, 1 },
    { "Wide Sweep",       0.50f,  3.2f, 3500.0f, -1.0f, 0, 0 },
    { "Thin Treble",      0.85f,  6.5f, 3000.0f, -1.5f, 0, 1 },
    { "Modern Smooth",    0.50f,  4.0f, 2400.0f,  0.0f, 0, 1 },
    { "Pedal Control",    0.50f,  6.0f, 2600.0f,  0.0f, 1, 0 },
    { "Pedal Modern",     0.50f,  5.0f, 3000.0f,  0.0f, 1, 1 },
    { "Rhythm Wah",       0.40f,  7.0f, 2200.0f,  1.0f, 0, 0 },
    { "Muted Half",       0.35f,  3.0f, 1600.0f,  2.5f, 0, 0 },
    { "Honk",             0.55f, 11.0f, 1900.0f, -3.0f, 0, 0 },
};

inline constexpr int kNumFactoryPresets = (int) (sizeof (kFactoryPresets) / sizeof (kFactoryPresets[0]));
} // namespace nf
