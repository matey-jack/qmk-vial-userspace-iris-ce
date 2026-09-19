// Copyright 2025 Robert Jack Will
// SPDX-License-Identifier: GPL-2.0-or-later

// Vial's own identity for this firmware. Generated with `python3 util/vial_generate_keyboard_uid.py`
// from a vial-qmk checkout; deliberately not the one upstream's `keebio/iris_ce:vial` keymap uses.
#define VIAL_KEYBOARD_UID {0xAA, 0x9A, 0xCD, 0xD7, 0xFA, 0xBB, 0xD6, 0x32}
// Hold these two keys to unlock the keyboard for Vial (top-left and bottom-right of the two halves).
// Same combo as upstream's Vial keymap. Only needed for dynamic macros, the bootloader jump and the
// matrix tester -- key overrides and QMK Settings can be written while locked.
#define VIAL_UNLOCK_COMBO_ROWS {0,9}
#define VIAL_UNLOCK_COMBO_COLS {0,5}

#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD

// This value is much higher than many of my actual hold-applications.
// But that should be fine, since those are handled by the "permissive hold" feature, not the actual timer.
// On the other hand, the high value avoids misreadings of a key roll (overlapping typing of "ys", for example) as a hold.
// The only reason to have this smaller than infinite is because sometimes the hold is used together with the mouse.
// (Ctrl+Scroll for zoom and Ctrl+Click.)
#define TAPPING_TERM 500

// Switch to Hold mode before tapping term, when other key is pressed _and_ released,
// all before the tap/hold key is released.
//
// Caution: under Vial this line does NOT take effect. Vial's QMK Settings framework turns
// permissive hold into a runtime bit that defaults to off, so `tools/provision.sh` has to set it
// after every flash. The line stays because it states the intent, and because it is what applies
// if QMK Settings is ever switched off. (TAPPING_TERM above *is* honoured: it seeds the runtime
// value.) See docs/porting-cozy-de-to-vial.md, section 3.
#define PERMISSIVE_HOLD

// Needed for VIA/Vial, since the default is only 4. Five layers cost 600 of the RP2040's 4096
// EEPROM bytes and leave about 2.2 kB for dynamic macros, so this is not tight.
#define DYNAMIC_KEYMAP_LAYER_COUNT 5

// Needed for layer-aware RGB keylights.
#define SPLIT_LAYER_STATE_ENABLE

#define RGB_MATRIX_TIMEOUT (5*60*1000) // number of milliseconds to wait until rgb automatically turns off
#define RGB_MATRIX_SLEEP               // turn off effects when suspended

// RGB_MATRIX_SOLID_COLOR ==> is always on (has no ENABLE_ flag)

// Disable all other animations so that we can simply toggle between the solid color and the mods&toggles.
#undef ENABLE_RGB_MATRIX_ALPHAS_MODS
#undef ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
#undef ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
#undef ENABLE_RGB_MATRIX_BREATHING
#undef ENABLE_RGB_MATRIX_BAND_SAT
#undef ENABLE_RGB_MATRIX_BAND_VAL
#undef ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
#undef ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
#undef ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
#undef ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL
#undef ENABLE_RGB_MATRIX_CYCLE_ALL
#undef ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#undef ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
#undef ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
#undef ENABLE_RGB_MATRIX_CYCLE_OUT_IN
#undef ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
#undef ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
#undef ENABLE_RGB_MATRIX_CYCLE_SPIRAL
#undef ENABLE_RGB_MATRIX_DUAL_BEACON
#undef ENABLE_RGB_MATRIX_RAINBOW_BEACON
#undef ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS
#undef ENABLE_RGB_MATRIX_FLOWER_BLOOMING
#undef ENABLE_RGB_MATRIX_RAINDROPS
#undef ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
#undef ENABLE_RGB_MATRIX_HUE_BREATHING
#undef ENABLE_RGB_MATRIX_HUE_PENDULUM
#undef ENABLE_RGB_MATRIX_HUE_WAVE
#undef ENABLE_RGB_MATRIX_PIXEL_FRACTAL
#undef ENABLE_RGB_MATRIX_PIXEL_FLOW
#undef ENABLE_RGB_MATRIX_PIXEL_RAIN

#undef ENABLE_RGB_MATRIX_TYPING_HEATMAP
#undef ENABLE_RGB_MATRIX_DIGITAL_RAIN

#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE
#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE
#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS
#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS
#undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS
#undef ENABLE_RGB_MATRIX_SPLASH
#undef ENABLE_RGB_MATRIX_MULTISPLASH
#undef ENABLE_RGB_MATRIX_SOLID_SPLASH
#undef ENABLE_RGB_MATRIX_SOLID_MULTISPLASH
