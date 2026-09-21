# Plan: running `cozy_de` on a stock-Vial Sofle Choc Pro

**Status:** files written, not yet tried on hardware. Written 2026-09-21.

**Goal:** flash unmodified Vial firmware onto a **Keebart Sofle Choc Pro**, then seed it with the
`cozy_de` keymap from a `.vil` file plus a small script that sets the things a `.vil` does not
carry. No custom `keymap.c` to maintain, no per-change recompile.

This is the Sofle edition of the Iris CE plan in
[`qmk_userspace_iris_cozy_keymap#12`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/pull/12),
and it does not repeat that document's reasoning about keycodes, key overrides, macros or Caps
Word — all of which is board-independent and transfers verbatim. What follows is the part that is
about *this* board: the matrix comparison, the extra hardware, and what stock Vial gives us here
that it did not give us there.

**Verdict: feasible, and easier than on the Iris CE.** The two keyboards have the same 10×6 split
matrix, the Sofle's Vial keymap compiles in **ten** dynamic layers where the Iris CE's has four,
and this repository already builds the firmware, so there is no `make` to run at all.

Terms used below, defined at first use:

- **Vial** — a fork of QMK plus a configurator GUI that keeps the whole keymap in EEPROM
  (the keyboard's non-volatile memory) instead of in compiled firmware, so it can be changed
  live over USB. <https://get.vial.today/>
- **VIA** — the older configurator protocol Vial extends. Vial speaks VIA plus its own commands.
- **`.vil`** — Vial's layout-export file. Despite the extension it is plain JSON.
- **`vitaly`** — a third-party command-line client for the VIA/Vial protocol, written in Rust.
  <https://github.com/bskaplou/vitaly>
- **qsid** — "QMK setting identifier", the numeric id Vial uses to address one runtime setting.
- **`LAYOUT` macro** — the C macro a QMK keymap is written in; it maps the physical key order the
  keymap is typed in onto `[row][col]` cells of the electrical matrix.
- **E1** — the German extended keyboard layout, xkb's `de(e1)`, standardised as DIN 2137-1:2020-11.
  Same meaning as in
  [`cozy_de/keymap.c`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/blob/main/keyboards/keebio/iris_ce/keymaps/cozy_de/keymap.c).

---

## 1. What the stock Vial firmware already gives us

`vial-qmk` has a Sofle Choc Pro port already:
[`keyboards/keebart/sofle_choc_pro/keymaps/vial/`](https://github.com/vial-kb/vial-qmk/tree/vial/keyboards/keebart/sofle_choc_pro/keymaps/vial),
with a `vial.json`, a keyboard UID, per-key RGB and per-layer encoder support:

```make
# keyboards/keebart/sofle_choc_pro/keymaps/vial/rules.mk
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes
ENCODER_MAP_ENABLE = yes
CAPS_WORD_ENABLE   = yes
REPEAT_KEY_ENABLE  = yes
```

```c
/* keyboards/keebart/sofle_choc_pro/keymaps/vial/config.h */
#define VIAL_KEYBOARD_UID \
    { 0x4F, 0x2D, 0x5A, 0x8A, 0x49, 0x7C, 0xDF, 0x1D }

#define VIAL_UNLOCK_COMBO_ROWS { 0, 5 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 0 }

#define DYNAMIC_KEYMAP_LAYER_COUNT 10
```

Everything else is switched on by Vial's own build fragment, without touching the keymap —
`QMK_SETTINGS`, `TAP_DANCE_ENABLE`, `COMBO_ENABLE`, `KEY_OVERRIDE_ENABLE`, `LAYER_LOCK_ENABLE`,
plus `-DCAPS_WORD_INVERT_ON_SHIFT`
(<https://github.com/vial-kb/vial-qmk/blob/vial/builddefs/build_vial.mk>). So **key overrides are
on by default**, which is what makes the thirteen-override Cozy Shift mapping possible at all.

Slot counts come from EEPROM size: above 4000 bytes of `TOTAL_EEPROM_BYTE_COUNT` that is 32 tap
dances, 32 combos, 32 key overrides and 32 alt-repeat keys
(<https://github.com/vial-kb/vial-qmk/blob/vial/quantum/vial.h>). The Sofle Choc Pro is RP2040
with flash-backed EEPROM, so it is in that bracket.

| Resource | Sofle Choc Pro, stock Vial | `cozy_de` needs | Fits |
| --- | --- | --- | --- |
| Layers (`DYNAMIC_KEYMAP_LAYER_COUNT`) | **10** | 4 | with six to spare |
| Key override slots | 32 | 13 | yes |
| Macro slots | 16 | 9 | yes |
| Combo slots | 32 | 1 | yes |

The layer count is the one line where this board is plainly better off than the Iris CE. There
the stock Vial keymap takes `dynamic_keymap.h`'s default of 4
(<https://github.com/vial-kb/vial-qmk/blob/vial/quantum/dynamic_keymap.h>) and `cozy_de` uses
exactly 4, with no headroom for a fifth layer without recompiling. Here the keymap's own
`config.h` raises it to 10.

---

## 2. The two matrices, side by side

Both boards declare a 10×6 matrix — rows 0–4 on the left half, 5–9 on the right — and both wire
the four alpha rows identically. The Iris CE uses 56 of the 60 cells; the Sofle Choc Pro uses all
60. Sources:
[`keebio/iris_ce/rev1/keyboard.json`](https://github.com/vial-kb/vial-qmk/blob/vial/keyboards/keebio/iris_ce/rev1/keyboard.json)
and
[`keebart/sofle_choc_pro/keyboard.json`](https://github.com/vial-kb/vial-qmk/blob/vial/keyboards/keebart/sofle_choc_pro/keyboard.json)
(`LAYOUT` and `LAYOUT_split_4x6_5` respectively), cross-checked against each keymap's `vial.json`,
which lists the same cells in visual order.

| Physical row | Left half | Right half | Same on both? |
| --- | --- | --- | --- |
| 1 (numbers) | `0,0` … `0,5` | `5,5` … `5,0` | yes |
| 2 | `1,0` … `1,5` | `6,5` … `6,0` | yes |
| 3 | `2,0` … `2,5` | `7,5` … `7,0` | yes |
| 4 | `3,0` … `3,5` | `8,5` … `8,0` | yes |
| inner | `4,5` | `9,5` | same cell, see below |
| thumbs | `4,2` `4,3` `4,4` | `9,4` `9,3` `9,2` | yes |
| **extra thumbs** | `4,0` `4,1` | `9,1` `9,0` | **Sofle only** |

So the conversion is not a re-mapping at all: **every cell of the Iris CE `.vil` keeps its
address**, and the four cells the Iris CE writes as `-1` ("no such key") become real keys here.

Two details behind the table:

- **The three thumb keys land in the same places.** Reading the `x` coordinates out of the two
  `keyboard.json` files, the Iris CE's thumbs sit at 3.5, 4.5 and 5.6 and the Sofle's `4,2` `4,3`
  `4,4` at 3.5, 4.5 and 6 — the same three positions, with the Sofle's two extra keys added
  *outboard* of them at 1.5 and 2.5. The right half mirrors this. `KC_LALT`, `LT(2,KC_DEL)` and
  `KC_SPC` therefore fall under the same thumb as before.
- **The inner keys move up one row.** `4,5` and `9,5` are an extra key *below* the bottom row on
  the Iris CE (y = 4, between the halves) and the innermost key *beside* the bottom row on the
  Sofle (y = 3). Same matrix cell, same keycodes — `KC_LGUI` and `G(KC_TAB)` — a slightly
  different reach.

### 2.1 The four extra keys

`4,0`, `4,1`, `9,1` and `9,0` are **`KC_NO` on every layer**. `cozy_de` has nothing to say about
them: they do not exist on the board it was written for, and inventing a mapping here would make
this file something other than "the Iris CE keymap on a Sofle". They are the obvious place to put
something later, in the Vial GUI, without touching anything else.

Note that `-1` would have been wrong. In Vial's own serialisation `-1` is what `save_layout()`
writes for a cell that is not in the keyboard's `rowcol` set at all, and `restore_layout()` skips
any cell it does not recognise
(<https://github.com/vial-kb/vial-gui/blob/main/src/main/python/protocol/keyboard_comm.py>). On
the Sofle those four cells *are* in the set, so `-1` would be fed to the keycode parser as a
keycode. `KC_NO` says the same thing in the one way the board can act on.

### 2.2 Layers 4 to 9

`KC_TRNS` in all 60 cells. Nothing in the four `cozy_de` layers reaches them — there is no
`MO()`, `LT()`, `TO()` or `OSL()` above layer 3 anywhere in the keymap — so the choice only
matters for what happens if you add one later. Transparent means such a layer falls through to
the base layer for every key you have not set yet; `KC_NO` would mean the keyboard goes dead
while it is held.

---

## 3. The extra hardware

### 3.1 The encoders

The Sofle Choc Pro has one rotary encoder per half — `keyboard.json` declares a single
`encoder.rotary` entry and `split.enabled`, which gives two, and the stock Vial keymap's
`encoder_map` has two rows. The Iris CE has none, so `cozy_de` has nothing to put on them.

**They are `KC_NO`, both directions, on all ten layers.** To give them the stock Vial keymap's
own assignment instead — `ENCODER_CCW_CW(KC_VOLD, KC_VOLU)` and `ENCODER_CCW_CW(KC_MPRV, KC_MNXT)`
— uncomment the four lines in [`seed-cozy-de.sh`](seed-cozy-de.sh), or set them in the Vial GUI.

The direction order in the `.vil` is worth writing down, because vial-gui's own variable names
have it backwards. `encoder_layout[layer][encoder]` is a two-element list indexed by the protocol's
direction byte, and `vial.c` fills its reply with `dynamic_keymap_get_encoder(layer, idx, 0)`
first and `(…, 1)` second (<https://github.com/vial-kb/vial-qmk/blob/vial/quantum/vial.c>,
`case vial_get_encoder`). That third argument is `clockwise`, so **element 0 is
counter-clockwise and element 1 is clockwise** — while `save_layout()` in
`keyboard_comm.py` calls its index-0 variable `cw`. The names are cosmetic (it round-trips
either way), but they are the wrong way round for anyone writing a `.vil` by hand.

### 3.2 RGB

60 per-key LEDs, 30 per half, driven by WS2812 with `max_brightness` 50 and `sleep` enabled, and
`VIALRGB_ENABLE` means effect, speed and hue/sat/val live in EEPROM and are set over the protocol
(`vitaly rgb -e <n> -c '#rrggbb' -p`, where `-p` persists across restarts).

As on the Iris CE, `ENABLE_RGB_MATRIX_KEY_GROUPS` is **not** among the choices: that colour mode
exists only in a fork of QMK, so no stock Vial build can offer it, at runtime or otherwise. The
standard effects are what there is; Solid Color is the closest match. Nothing about RGB is pinned
in `seed-cozy-de.sh` — it persists on its own once set — but the script carries a commented
`vitaly rgb` line showing how to fix an effect and colour if you want them reproducible.

---

## 4. What carries over unchanged from the Iris CE plan

Everything that is about the keymap rather than the board. In short, and with the detail in
[§2 and §3 of the Iris CE plan](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/pull/12/files):

- **All keycodes.** The German aliases in `keymap_german.h` are not special keycodes; they resolve
  to plain US keycodes, `S(...)` or `ALGR(...)`, and a `.vil` stores each key as a string that
  Vial parses with `simpleeval`
  (<https://github.com/vial-kb/vial-gui/blob/main/src/main/python/any_keycode.py>). Two need an
  older spelling: `MS_WHLD`/`MS_WHLU` are written `KC_WH_D`/`KC_WH_U`, and `OS_LSFT` and friends
  are written `OSM(MOD_LSFT)`.
- **All thirteen key overrides**, including the four that are split into per-Shift-side pairs
  ([issue #14](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/issues/14)) because
  `vial-qmk` still carries the pre-fix `process_key_override.c`. The fix is pure data, so it
  transfers; this board runs the same `vial-qmk`, so it needs it just as much.
- **The nine macros** `M0`…`M8`, each a dead key (or E1's Level-5 latch) plus one more key with a
  10 ms gap.
- **The Caps Word combo** `KC_LSFT + KC_RSFT → CW_TOGG`, standing in for
  `BOTH_SHIFTS_TURNS_ON_CAPS_WORD`, which is not one of Vial's runtime settings.
- **The two tap-hold settings**, qsid 7 (`TAPPING_TERM`, 500) and qsid 22 (`PERMISSIVE_HOLD`,
  true) (<https://github.com/vial-kb/vial-qmk/blob/vial/quantum/qmk_settings.h>).

## 5. What is lost

The same four things as on the Iris CE, for the same reasons, and none of them is a character you
can no longer type: the exact both-shifts Caps Word behaviour, the modifier-clearing in
`send_prefixed_key()`, the `MX_VERS` version key, and `ENABLE_RGB_MATRIX_KEY_GROUPS`. The fifth
item on that list — *no headroom for a fifth layer* — does **not** apply here, see §1.

---

## 6. How this file was made

`cozy_de.vil` is the Iris CE file `vial/cozy_de.vil` from
[pull request #12](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/pull/12/files)
with exactly four changes. They are listed here rather than kept as a script, for the same reason
the original generator was not kept: the conversion happens once.

1. **`uid`** → `2152575802202008911`, the Sofle's `VIAL_KEYBOARD_UID` bytes
   `{0x4F, 0x2D, 0x5A, 0x8A, 0x49, 0x7C, 0xDF, 0x1D}` read back as a little-endian `uint64`.
   (The same reading turns the Iris CE's `{0x45, 0xEE, …}` into the `5617177301808770629` in the
   source file, which is how the byte order was confirmed.)
2. **The four `-1` cells** — `4,0`, `4,1`, `9,0`, `9,1`, in all four layers — became `"KC_NO"`.
   Every other cell is byte-identical.
3. **Six more layers**, `KC_TRNS` in all 60 cells each, bringing `layout` to
   10 × 10 rows × 6 cols.
4. **`encoder_layout`** went from `[[], [], [], []]` (four layers, no encoders) to ten layers of
   two `["KC_NO", "KC_NO"]` pairs.

`macro`, `tap_dance`, `combo`, `key_override`, `alt_repeat_key`, `settings`, `layout_options`,
`vial_protocol` and `via_protocol` are unchanged. `layout_options` stays `-1` because the Sofle's
`vial.json` declares no layout labels.

---

## 7. Order of work

1. Flash `keebart/sofle_choc_pro:vial`, **both halves**. This repository builds it already — the
   target is in [`qmk.json`](../../qmk.json) and the `.uf2` is attached to every
   [release](https://github.com/matey-jack/qmk-vial-userspace-iris-ce/releases) — so put each half
   into bootloader mode and copy the file onto the `RPI-RP2` drive. To build it yourself:
   `make keebart/sofle_choc_pro:vial` from a `vial-qmk` checkout.
2. `vitaly devices` — confirm the product id and that the board answers. `keyboard.json` declares
   `pid` `0x0000` and `vid` `0xFEED`, so the id to expect is `0`; vitaly's `-i` is an
   `Option<u16>`, so `-i 0` really does mean "product id 0" and not "unset"
   (<https://github.com/bskaplou/vitaly/blob/main/src/main.rs>).
3. `vitaly load -f cozy_de.vil -p` to preview before writing anything.
4. Run [`seed-cozy-de.sh`](seed-cozy-de.sh). The unlock keys are compiled in as matrix `(0,0)` and
   `(5,0)` — the two outer keys of the top row, Esc and Backspace in this keymap. `vitaly lock -u`
   draws which to hold.
5. Test pass, against the README's list: the Shift mapping of the number row, the ä/Tab chameleon
   under Ctrl/Alt/Gui, the five accent macros, ^ and `, the Level-5 items (¢ £ and `DE_LVL5`
   itself), Caps Word via the combo, and the 500 ms tapping term on the four `LT()` keys.
6. Then the Sofle-specific checks: the four outer thumb keys and both encoders do nothing, and
   `KC_LGUI` / `G(KC_TAB)` are where §2 says they are.

---

## References

- Vial: <https://get.vial.today/> · manual: <https://get.vial.today/manual/>
- vial-qmk: <https://github.com/vial-kb/vial-qmk>
  - Sofle Choc Pro Vial keymap: `keyboards/keebart/sofle_choc_pro/keymaps/vial/`
  - Sofle Choc Pro board definition: `keyboards/keebart/sofle_choc_pro/keyboard.json`
  - Iris CE board definition: `keyboards/keebio/iris_ce/rev1/keyboard.json`
  - Vial build defaults: `builddefs/build_vial.mk`
  - Slot counts: `quantum/vial.h` · runtime settings: `quantum/qmk_settings.h`
  - Encoder protocol: `quantum/vial.c`, `case vial_get_encoder`
  - Layer count default: `quantum/dynamic_keymap.h`
- vial-gui (defines the `.vil` format): <https://github.com/vial-kb/vial-gui>
  - `src/main/python/protocol/keyboard_comm.py` — `save_layout()` / `restore_layout()`
  - `src/main/python/any_keycode.py` — which keycode expressions parse
- vitaly: <https://github.com/bskaplou/vitaly> · crate: <https://crates.io/crates/vitaly>
- The Iris CE original: <https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/pull/12>
- QMK Key Overrides: <https://docs.qmk.fm/features/key_overrides>
- QMK Caps Word: <https://docs.qmk.fm/features/caps_word>
- QMK Encoders: <https://docs.qmk.fm/features/encoders>
