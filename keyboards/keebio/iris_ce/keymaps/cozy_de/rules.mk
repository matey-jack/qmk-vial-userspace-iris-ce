# Vial. VIAL_ENABLE requires VIA_ENABLE; VIALRGB hands the per-key RGB to the Vial GUI
# while leaving the RM_* keycodes on the Fn layer working.
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes

CONSOLE_ENABLE = yes
CAPS_WORD_ENABLE = yes

# Replaces the getreuer/custom_shift_keys community module of the `cozy` keymap.
# Note that under Vial the overrides themselves live in EEPROM, not in this firmware --
# `tools/provision.sh` writes them. See docs/porting-cozy-de-to-vial.md, section 3.
# vial-qmk turns this on by default anyway; it is spelled out here because the keymap needs it.
KEY_OVERRIDE_ENABLE = yes

# No Unicode input here: this keymap only sends keycodes of the standard German OS layout.

RGB_MATRIX_ENABLE = yes        # per-key RGB
