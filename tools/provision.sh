#!/usr/bin/env bash
#
# Restore the Vial-side configuration that a firmware flash wipes.
#
# Vial's EEPROM-validity magic is BUILD_ID, a random number generated on every build, so every
# new firmware invalidates the stored configuration. The keymap layers are re-seeded automatically
# from keymap.c, which is what we want -- but key overrides and QMK Settings are reset to empty
# and to their defaults, and those carry the Cozy Shift mapping and permissive hold. Run this
# after every flash. See docs/porting-cozy-de-to-vial.md.
#
# Needs vitaly, the VIA/Vial command-line tool: https://github.com/bskaplou/vitaly
#     cargo install vitaly      # or: brew install bskaplou/tap/vitaly, or a release binary
#
# No unlock needed: key overrides and QMK Settings can be written to a locked keyboard. Only
# dynamic macros, the bootloader jump and the matrix tester are gated, and this keymap uses none
# of them (its "macros" are firmware custom keycodes, not Vial dynamic macros).
#
# Usage:  tools/provision.sh [-i <device-id>]
#         Without -i, vitaly acts on every connected VIA/Vial keyboard. Run `vitaly devices` to
#         find the id if more than one is plugged in.

set -euo pipefail

VITALY=${VITALY:-vitaly}
ARGS=("$@")

vial() { "$VITALY" ${ARGS[@]+"${ARGS[@]}"} "$@"; }

if ! command -v "$VITALY" > /dev/null; then
    echo "error: $VITALY not found on PATH -- see https://github.com/bskaplou/vitaly" >&2
    exit 1
fi

echo '== Devices =='
vial devices

# --- Key overrides: the Cozy Shift mapping -----------------------------------------------------
#
# `layers` is a bitmask over layers, not a layer number: 65535 is every layer, which is what
# ko_make_basic() means by ~0. Slots 0-7 suppress the trigger's Shift so the replacement fully
# determines what the host sees.
#
# Slot 8, the "chameleon" ä key, is deliberately different: it suppresses nothing, because a
# suppressed trigger modifier is re-added as a "weak" mod that gets dropped when the replacement
# is unregistered -- which would close the window switcher between two taps of ä. `suppressed_mods`
# is therefore left out entirely, which is how vitaly spells "none". It also sets
# ko_option_one_mod, so that any one of Ctrl/Alt/Gui triggers it rather than all three at once.
#
# The DE_* names are keymap_extras aliases (DE_AT is ALGR(KC_Q) and so on). vitaly documents
# "QMK keycodes notation together with aliases", but whether that stretches to the German set is
# unverified -- if it rejects a name, substitute the underlying keycode, e.g. RALT(KC_Q) for DE_AT.
echo
echo '== Key overrides =='
OPTS='ko_enabled|ko_option_activation_trigger_down|ko_option_activation_required_mod_down|ko_option_activation_negative_mod_up'

shift_override() { # slot trigger replacement
    vial keyoverrides -n "$1" -v "trigger=$2; replacement=$3; layers=65535; \
trigger_mods=MOD_BIT_LSHIFT|MOD_BIT_RSHIFT; suppressed_mods=MOD_BIT_LSHIFT|MOD_BIT_RSHIFT; \
options=$OPTS"
}

shift_override 0 KC_2    DE_AT     # @ instead of "
shift_override 1 KC_3    DE_HASH   # # instead of §
shift_override 2 KC_6    DE_SS     # ß instead of &
shift_override 3 KC_7    DE_AMPR   # & instead of /
shift_override 4 KC_8    DE_ASTR   # * instead of (
shift_override 5 KC_9    DE_PLUS   # + instead of )
shift_override 6 KC_0    DE_QUES   # ? instead of =
shift_override 7 DE_QUOT DE_DQUO   # the US ANSI ' / " pairing

# The chameleon ä: Tab whenever Ctrl, Alt or Gui is held; plain ä and Shift+ä stay ä and Ä.
vial keyoverrides -n 8 -v "trigger=DE_ADIA; replacement=KC_TAB; layers=65535; \
trigger_mods=MOD_BIT_LCTRL|MOD_BIT_RCTRL|MOD_BIT_LALT|MOD_BIT_RALT|MOD_BIT_LGUI|MOD_BIT_RGUI; \
options=$OPTS|ko_option_one_mod"

# --- QMK Settings ------------------------------------------------------------------------------
#
# Not carried in a .vil file, so they need setting separately. Setting 22 is Permissive Hold,
# which config.h asks for but Vial's runtime settings default to off. Setting 7 (Tapping Term) is
# only read back: qmk_settings_reset() seeds it from TAPPING_TERM, so it should already be 500.
echo
echo '== QMK Settings =='
vial settings -q 22 -v true
vial settings -q 7

echo
echo '== Done. Verify below that all nine overrides read back as expected =='
vial keyoverrides
