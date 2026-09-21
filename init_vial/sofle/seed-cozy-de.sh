#!/bin/sh
#
# Seed a stock-Vial Sofle Choc Pro with the cozy_de keymap.
#
# `cozy_de.vil` carries the layers, the nine macros, the thirteen key overrides and the Caps Word
# combo. It also carries a `settings` block, but vitaly's `load` does not apply it -- only the
# Vial GUI does -- so the two tap-hold settings are set explicitly further down. Running this
# after a GUI restore is harmless: every step is idempotent.
#
# Usage:  ./seed-cozy-de.sh            (keyboard plugged in, both halves)
#         VIAL_ID=1234 ./seed-cozy-de.sh
#
set -eu

# The Sofle Choc Pro really does declare product id 0x0000 (with vid 0xFEED) in
# keyboards/keebart/sofle_choc_pro/keyboard.json, so 0 is the right filter, not a missing value:
# vitaly's -i is an Option<u16>, and `-i 0` means "product id 0", not "no filter".
# Override if `vitaly devices` reports something else.
ID="${VIAL_ID:-0}"

cd "$(dirname "$0")"

command -v vitaly > /dev/null || {
    echo "vitaly not found. Install it with 'cargo install vitaly' or from" >&2
    echo "https://github.com/bskaplou/vitaly/releases/latest" >&2
    exit 1
}
[ -f cozy_de.vil ] || { echo "cozy_de.vil is missing next to this script" >&2; exit 1; }

echo "== the keyboard vitaly sees =="
vitaly -i "$ID" devices

# Vial refuses to write macros, key overrides and combos while locked. The unlock keys are
# compiled in as matrix (0,0) and (5,0) -- the two outer keys of the top row, which this keymap
# has Esc and Backspace on. vitaly draws which ones to hold and counts down.
echo
echo "== unlocking: hold the two keys vitaly marks until it says unlocked =="
vitaly -i "$ID" lock -u

echo
echo "== loading the keymap, macros, key overrides and the Caps Word combo =="
vitaly -i "$ID" load -f cozy_de.vil

# Not in what `load` writes, so set here. These are `config.h`'s TAPPING_TERM 500 and
# PERMISSIVE_HOLD, which stock Vial keeps as runtime settings instead of compiling in.
echo
echo "== tap-hold settings =="
vitaly -i "$ID" settings -q 7  -v 500
vitaly -i "$ID" settings -q 22 -v true

# The two encoders are KC_NO on every layer in the .vil, because cozy_de has nothing to put on
# them -- the Iris CE has no encoders at all. To give them the stock Vial keymap's volume and
# track control on the base layer, uncomment these. `-l` is the layer and `-p` is
# <encoder>,<direction>, where direction 0 is counter-clockwise and 1 is clockwise.
#   vitaly -i "$ID" encoders -l 0 -p 0,0 -v KC_VOLD
#   vitaly -i "$ID" encoders -l 0 -p 0,1 -v KC_VOLU
#   vitaly -i "$ID" encoders -l 0 -p 1,0 -v KC_MPRV
#   vitaly -i "$ID" encoders -l 0 -p 1,1 -v KC_MNXT

# RGB is per-taste and persists on its own, so nothing is forced here. To pin it down, add
# something like the line below: `-e` picks an effect from `vitaly -i "$ID" rgb -i`, and `-p`
# makes it survive a replug. Effect 2 is Solid Color.
#   vitaly -i "$ID" rgb -e 2 -c '#20304a' -p

echo
echo "== locking again =="
vitaly -i "$ID" lock -l

echo
echo "Done. Worth checking by hand: Shift+2 gives @, Shift+0 gives ?, holding Alt and tapping"
echo "ä walks the window list, both Shifts together toggle Caps Word, and the L_COMBINE thumb"
echo "keys type é and è. The four outer thumb keys and both encoders are meant to do nothing."
