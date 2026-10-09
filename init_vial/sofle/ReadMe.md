# `cozy_de` on the Sofle Choc Pro

The `cozy_de` keymap on a **Keebart Sofle Choc Pro**, as an almost pure Vial setup: nearly all of
it is Vial data, loaded over USB, and only a little is compiled into the firmware.

| Part | What it holds |
| --- | --- |
| [`keyboards/keebart/sofle_choc_pro/keymaps/cozy_de/`](../../keyboards/keebart/sofle_choc_pro/keymaps/cozy_de/) | vial-qmk's own Sofle Vial keymap (same `vial.json`, keyboard UID, ten layers and unlock keys), plus the few things Vial cannot set at runtime |
| `cozy_de.vil` | everything Vial *can* set at runtime: the layers, 9 macros, 13 key overrides and the `settings` block (tapping term 500 ms, permissive hold) |

Edit `cozy_de.vil` in the Vial GUI or at <https://vial.rocks>, then *File → Save current layout*
back over this file. The firmware keeps the stock UID so that the `.vil` loads onto stock Vial
firmware just as well as onto this build.

## What is compiled in, and why

Only what Vial has no runtime switch for:

- **`BOTH_SHIFTS_TURNS_ON_CAPS_WORD`** in `config.h`. Vial cannot do this at runtime, and a
  `KC_LSFT + KC_RSFT` combo does not work.
- **The Win+Tab tap of `MC_WINT`** in `process_record_user()`. The `.vil` stores that key as a
  plain `RGUI_T(KC_TAB)`, which the firmware turns into Win+Tab on tap and Win on hold. On stock
  Vial firmware the same key taps a plain Tab.
- **The `.vil`'s layers as `keymap.c` defaults**, so the board is usable straight after
  flashing. They are a copy, so when the layers in the `.vil` change, `keymap.c` should follow.

Since there is a custom build anyway, this is the place for further small compile-time options.

## Why so little needs compiling

- **Vial builds enable the needed features by default.** `builddefs/build_vial.mk` turns on key
  overrides, combos, tap dance, Caps Word and QMK settings for every Vial keymap. The Cozy Shift
  mapping of the number row and the ä/Tab chameleon key are therefore just key-override data.
- **There are enough slots.** On an RP2040 there are 32 key overrides and 16 macros, of which
  `cozy_de` uses 13 and 9 (`quantum/vial.h`).
- **The matrix is the Iris CE's.** Both boards have the same 10×6 split matrix, so every keycode
  of the original Iris CE keymap sits on the same matrix cell. The Sofle adds four outer thumb
  keys and two encoders.
- **German keycodes are plain keycodes.** The `DE_*` aliases of `keymap_german.h` resolve to US
  keycodes with `S()` or `ALGR()`, so the `.vil` can store them. It uses vial-gui's older
  spellings, such as `KC_WH_D` and `OSM(MOD_LSFT)`.
- **The key overrides on `7`, `8`, `0` and `'` are split into a left-Shift and a right-Shift
  pair.** vial-qmk still has the `process_key_override.c` bug from
  [qmk_userspace_iris_cozy_keymap#14](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/issues/14),
  and splitting them works around it in data alone.

## Flashing and loading

Each vial-qmk build gets a random EEPROM ID (`util/build_id.py`), so **flashing a new build resets
the board** to the compiled-in defaults: the layers from `keymap.c`, no macros, no key overrides,
default settings. Load the `.vil` again after every flash, on **both halves** of the board.

Load it in the Vial GUI or at <https://vial.rocks> (*File → Load saved layout*), or with
[`vitaly`](https://github.com/bskaplou/vitaly):

```sh
vitaly lock -u            # hold the two keys it marks until it says unlocked
vitaly load -f cozy_de.vil
```

Unlocking is needed because Vial refuses to write macros while locked. The unlock keys are the
two outer keys of the top row. `load -p` previews without writing.

## Compared with the compiled Iris CE `cozy_de`

The original is
[`keebio/iris_ce/keymaps/cozy_de`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/tree/main/keyboards/keebio/iris_ce/keymaps/cozy_de).
It can type every character this setup can. What this setup lacks, each a candidate for a later
compiled-in customization:

- **The accent macros do not clear modifiers.** Hold Shift on the é macro and you get è, because
  ´ and ` share a key on the German layout.
- **There is no `MX_VERS` key**, which typed out the firmware version.
- **There is no `ENABLE_RGB_MATRIX_KEY_GROUPS`.** That effect exists only in a QMK fork, so the
  standard RGB matrix effects are what there is.
