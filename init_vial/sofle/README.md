# `cozy_de` on stock Vial, Sofle Choc Pro edition

Three files that put the `cozy_de` keymap on a **Keebart Sofle Choc Pro** running an unmodified
[Vial](https://get.vial.today/) build, with no keymap of our own compiled into the firmware.

| File | What it is |
| --- | --- |
| [`PLAN.md`](PLAN.md) | why this works, what changed against the Iris CE version, and every claim sourced |
| `cozy_de.vil` | the keymap itself: 4 layers (of 10), 9 macros, 13 key overrides, 1 combo |
| `seed-cozy-de.sh` | loads the `.vil` and sets what it does not carry |

`cozy_de.vil` is the Iris CE file from
[`qmk_userspace_iris_cozy_keymap#12`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap/pull/12),
re-addressed for this board. Both keyboards have the same 10×6 split matrix, so **every keycode
sits on the same matrix cell as it does on the Iris CE** — see
[§2 of `PLAN.md`](PLAN.md#2-the-two-matrices-side-by-side) for the cell-by-cell comparison.
From here on the `.vil` is the thing to edit — in the Vial GUI, then *File → Save current layout*
back over this file.

## Using it

Flash the stock Vial firmware once, on **both halves**. This repository already builds it: the
`keebart/sofle_choc_pro:vial` target in [`qmk.json`](../../qmk.json) is the unmodified Vial keymap
out of `vial-qmk`, so take the `.uf2` from the latest
[release](https://github.com/matey-jack/qmk-vial-userspace-iris-ce/releases), put each half into
bootloader mode, and copy the file onto the `RPI-RP2` drive that appears.

To build it yourself instead, it is one `make` from a `vial-qmk` checkout:

```sh
git clone --recurse-submodules https://github.com/vial-kb/vial-qmk.git
cd vial-qmk
make keebart/sofle_choc_pro:vial
```

Then either open `cozy_de.vil` in the Vial GUI (*File → Load saved layout*), or:

```sh
cargo install vitaly      # or a binary from https://github.com/bskaplou/vitaly/releases
./seed-cozy-de.sh
```

The GUI route applies the `settings` block too; `vitaly load` does not, which is why the script
sets the tapping term and permissive hold explicitly afterwards.

## What is different from the Iris CE

The Sofle Choc Pro has **four keys and two encoders the Iris CE does not**, and six more layers
to put them on. All of it is deliberately left empty, so that this file is the Iris CE keymap and
nothing else:

- **The four extra thumb keys are `KC_NO`** on every layer: matrix `4,0` and `4,1` (the two outer
  keys of the left thumb cluster) and `9,1` and `9,0` (the same two on the right). They are the
  cells the Iris CE simply does not wire up.
- **Both encoders are `KC_NO`**, counter-clockwise and clockwise, on all ten layers.
  [§3.1 of `PLAN.md`](PLAN.md#31-the-encoders) has the one-line `vitaly` call that gives them
  volume and track control if you want it.
- **Layers 4…9 are `KC_TRNS` throughout.** The Sofle's Vial keymap compiles in ten layers where
  the Iris CE has four, so there is headroom now; transparent rather than `KC_NO` means a later
  `MO(4)` falls through to the base layer instead of going dead.

Two keys move a little physically, without changing the matrix cell they sit on: `4,5` (`KC_LGUI`)
and `9,5` (`G(KC_TAB)`) are an inner key *below* the bottom row on the Iris CE, and the innermost
key *beside* the bottom row on the Sofle. Everything else, thumbs included, lands at the same
place on the board.

## What differs from the compiled keymap

Four deliberate differences, inherited unchanged from the Iris CE version — §4 and §5 of
`PLAN.md` have the reasoning:

- **Caps Word** is a `KC_LSFT + KC_RSFT → CW_TOGG` combo instead of
  `BOTH_SHIFTS_TURNS_ON_CAPS_WORD`, which stock Vial has no runtime switch for. It *toggles*, so
  both Shifts again turns it off, and stock Vial compiles in `CAPS_WORD_INVERT_ON_SHIFT`, so
  Shift inside Caps Word gives a lowercase letter.
- **The accent macros do not clear modifiers.** `send_prefixed_key()` did; a Vial macro cannot.
  Hold Shift while pressing the é key and you get è, because ´ and ` share a key on the German
  layout. They are minuscule-only macros, so this only bites when you were already off-script.
- **`MX_VERS` is gone.** It printed the firmware version and build date, and there is no longer a
  per-keymap build to identify.
- **`ENABLE_RGB_MATRIX_KEY_GROUPS` is gone.** That colour mode lives in a fork of QMK, so no
  stock Vial build has it. The standard RGB matrix effects are all available instead, across all
  60 per-key LEDs.

Everything else — the Cozy Shift mapping of the number row, the ä/Tab chameleon key, the five
accented letters, the Level-5 characters, the 500 ms tapping term — carries over unchanged.
