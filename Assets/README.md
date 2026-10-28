# Assets

Images embedded into the plugin at build time (every `*.png` / `*.jpg` here is picked up by CMake).

| File | Use |
|---|---|
| `panel.jpg` | Front panel (2500 px wide), built from the knob-less base artwork: knob bases (knurled skirt, shadows, tick marks) are drawn into it, labels re-spaced, LCD narrowed with two arrow keys (previous / next preset) added, LCD text removed (drawn live), baked "50%" removed, "ACTIEE" typo fixed. |
| `knob_N_albedo.png` | Knob cap texture (N = 1 Position, 2 Q/Peak, 3 Range, 4 Level): fine detail only, with NO baked light or shadow. |
| `knob_N_relief.png` | Knob relief map: R,G = surface tilt, B = wear mask. Used to light scratches/dents in real time. |
| `sw_up.png`, `sw_down.png` | Toggle-switch lever states (taken from the panel artwork). |
| `led_off.png` | Dim LED shown over the "active" LED when bypassed. |

The knobs are rendered in real time (`KnobRenderer` in `Source/PluginEditor.h`): the light is fixed,
the texture and pointer rotate. Layout coordinates live in `Source/PluginEditor.cpp`
(pixels of a 3376 x 608 panel). See `../ASSETS_LICENSE.md` for licensing.
