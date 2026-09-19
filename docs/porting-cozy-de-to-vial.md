# The `cozy_de` Vial keymap: design notes

`keyboards/keebio/iris_ce/keymaps/cozy_de/` is the Vial edition of the `cozy_de` keymap, forked
from [`matey-jack/qmk_userspace_iris_cozy_keymap`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap).
The layers, the German keycodes and the accent macros came across unchanged. This document covers
what Vial made us do differently, and why — the parts a reader of `keymap.c` would otherwise have
to rediscover.

All of it was checked against
[`vial-kb/vial-qmk`](https://github.com/vial-kb/vial-qmk/tree/dd43959ae5c08d8a28d38a1acf7b04e86b14a344)
at `dd43959`, whose QMK base is 0.29.0 (2025-05-25), and the result builds clean (zero warnings,
63.8 kB of flash).

Terms used below:

* **VIA** — a protocol plus GUI that re-maps a keyboard at runtime over raw HID, storing the result
  in the keyboard's EEPROM instead of in the compiled firmware.
* **Vial** — a fork of VIA ([get.vial.today](https://get.vial.today/)) with its own firmware fork,
  `vial-qmk`. It adds runtime-editable tap dance, combos, key overrides and a "QMK Settings" tab.
* **EEPROM** — the keyboard's non-volatile memory. On the Iris CE's RP2040 it is *emulated* in
  flash by QMK's wear-levelling driver: 4096 bytes.
* **`.vil`** — Vial's layout export file.

## 1. The one thing to understand first

**Under Vial, `keymap.c` is a seed, not the keymap.** On the first boot after a flash the firmware
copies `keymaps[][][]` into EEPROM, and from then on the EEPROM copy is what the keyboard types,
editable live in the GUI.

The trigger for that copy is `via_eeprom_is_valid()`
([`via.c:86`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/via.c)), which compares the
stored magic against `BUILD_ID` — and `BUILD_ID` is a **random 24-bit number generated on every
build** by [`util/build_id.py`](https://github.com/vial-kb/vial-qmk/blob/dd43959/util/build_id.py),
injected at `build_keyboard.mk:296`. So:

* **Every flash re-seeds the layers from `keymap.c`.** No `EE_CLR` dance. The old keymap's
  `rules.mk` warning that *"VIA_ENABLE messes with static keymap updates"* does not apply here.
* **Every flash also wipes everything else Vial stores** — key overrides, QMK Settings, macros,
  combos, tap dances. Two of those matter to this keymap, which is what §2 and §3 are about.

`tools/provision.sh` puts them back. Run it after every flash.

## 2. What is not in `keymap.c`, and why

### 2.1 Key overrides — the whole Cozy Shift mapping

`Shift+9 → +`, `Shift+0 → ?`, `Shift+6 → ß`, the `'`/`"` pairing, the ä/Tab chameleon key: nine
QMK Key Overrides, and **none of them are defined in `keymap.c`**.

A static `const key_override_t *key_overrides[]` array does not work under Vial, and fails
silently. QMK reads that array through `key_override_count()` and `key_override_get()`, which are
*weak* in
[`keymap_introspection.c:161,174`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/keymap_introspection.c).
`vial-qmk` defines both *strongly* in
[`vial.c:648,652`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.c), reading
EEPROM instead, and that path is not opt-in:
[`vial.h:142`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.h) enables it for any
build with `KEY_OVERRIDE_ENABLE`, which
[`build_vial.mk:11`](https://github.com/vial-kb/vial-qmk/blob/dd43959/builddefs/build_vial.mk)
defaults to `yes`. Nor is EEPROM seeded from the array: `dynamic_keymap_reset()`
([`dynamic_keymap.c:162`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/dynamic_keymap.c))
fills all 32 override slots with a zeroed, disabled entry.

An array here would therefore compile, link, and do nothing. It is left out, and `keymap.c` carries
the nine overrides as a documentation table instead. `tools/provision.sh` is their executable form.

Everything survives the move: Vial's on-disk record
([`vial_key_override_entry_t`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.h))
carries trigger, replacement, layers, trigger mods, negative mask, suppressed mods and options —
including `ko_option_one_mod`, which the chameleon key needs. Only `custom_action`/`context` are
lost, and this keymap never used them.

### 2.2 Permissive hold

`config.h` still says `#define PERMISSIVE_HOLD`, and under Vial that line does nothing.

Vial enables its QMK Settings framework by default (`build_vial.mk:4`), which moves a set of
tap/hold options from compile time to runtime: `get_permissive_hold()`
([`qmk_settings.c:293`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/qmk_settings.c))
returns a stored bit, and `qmk_settings_reset()` (line 190) sets `tapping_v2 = 0`. So permissive
hold comes up **off** after every flash, on a keymap whose layer-taps on `y`, `-`, `Esc`, `Enter`,
`Del` and `Ins` depend on it at a 500 ms tapping term. `provision.sh` turns it on.

The `#define` stays because it states the intent, and because it is what applies if QMK Settings is
ever switched off.

Three neighbours are fine as they are: `TAPPING_TERM 500` **is** honoured (`qmk_settings_reset()`
seeds the runtime value from it), and `CHORDAL_HOLD` and Flow Tap are compiled in but default off
(`tapping_v2 = 0`, `flow_tap_term = 0`), as does Auto Shift.

## 3. Provisioning: `tools/provision.sh`

The script uses [`vitaly`](https://github.com/bskaplou/vitaly), a command-line client for the
VIA/Vial protocol (Rust, MIT, by @bskaplou — third-party, **not** an official `vial-kb` project).
Install with `cargo install vitaly`, `brew install bskaplou/tap/vitaly`, or a
[release binary](https://github.com/bskaplou/vitaly/releases/latest); on Linux it needs
`libudev-dev`.

It writes the nine key overrides with `vitaly keyoverrides -n <slot> -v '<spec>'` and the
permissive-hold bit with `vitaly settings -q 22 -v true`. Setting 22 is Permissive Hold; the
`vitaly` setting IDs line up exactly with the `DECLARE_STATIC_BITSETTING(22, tapping_v2, …)` in
`qmk_settings.c`.

Two properties worth knowing:

**It needs no unlock, so it runs unattended.** Unlocking is the one thing that cannot be scripted —
`vitaly lock -u` waits for a physical hold of the two keys named by `VIAL_UNLOCK_COMBO_ROWS`/`_COLS`
in `config.h`. But almost nothing here is gated on the lock:

| operation | gated on unlock? | where |
|---|---|---|
| key overrides, combos, tap dances, alt-repeat | **no** | `vial.c:286-320` |
| keymap keycodes | **no** | `via.c` dynamic keymap commands |
| QMK Settings get / set / reset | **no** | `vial.c:210-222` |
| dynamic **macros** | yes | `via.c:403` |
| bootloader jump (`vitaly bootload`) | yes | `via.c:438` |
| matrix tester | yes | `via.c:253` |
| writing `QK_BOOT` as a keycode | yes | `vial.c:81` |

This keymap's "macros" are firmware custom keycodes (`MX_EACU` and friends), not Vial *dynamic*
macros, so its macro set is empty and nothing it needs falls in the gated column.

**It does not use a `.vil` file.** `vitaly load` would restore key overrides, combos, tap dances,
alt-repeat keys, macros and the keymap in one step — but not QMK Settings, which are a separate
subcommand either way; and a `.vil` has to be produced from an already-configured board, which
makes it a build artifact rather than a source. Writing the overrides declaratively keeps the
script itself as the source of truth, reviewable in a diff. Exporting a `.vil` with
`vitaly save -f …` (or the GUI's *File → Save current layout*) remains a perfectly good backup.

## 4. Smaller differences from the non-Vial firmware

* **Caps Word inverts on Shift.** `build_vial.mk:15` adds `-DCAPS_WORD_INVERT_ON_SHIFT`
  unconditionally: pressing Shift during Caps Word inverts the next key instead of ending Caps
  Word. `BOTH_SHIFTS_TURNS_ON_CAPS_WORD` still works. Cannot be switched off from the keymap.
* **RGB goes through the Vial GUI** (`VIALRGB_ENABLE`), and the `RM_*` keycodes on the Fn layer
  keep working. The `#undef ENABLE_RGB_MATRIX_*` block came across unchanged: VialRGB builds its
  effect list from `#ifdef RGB_MATRIX_EFFECT_*` guards
  ([`vialrgb_effects.inc:52`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vialrgb_effects.inc)),
  so disabling animations just shortens the list the GUI offers, and `#undef`-ing an animation
  `vial-qmk` does not have yet is harmless.
* **`ENABLE_RGB_MATRIX_KEY_GROUPS` was dropped.** It matches nothing — not in `vial-qmk`, not in
  [QMK master](https://github.com/qmk/qmk_firmware/blob/master/quantum/rgb_matrix/animations/rgb_matrix_effects.inc).
  It is a dead line in the upstream keymap too.
* **`QK_BOOT` still works** from the Fn layer. Vial only refuses to *write* it from the GUI while
  locked (`vial_keycode_firewall()`, `vial.c:80`); `dynamic_keymap_reset()` unlocks for the
  duration of its seeding, with a comment saying exactly that.
* **The version print still works.** Vial builds force `-DNO_DEBUG`, which compiles out `dprintf`
  but not `println` — that one is guarded by `NO_PRINT` (`print.h:51,74`). The old
  `debug_enable = true` was dropped, since it only ever gated `dprintf`.
* **Custom keycodes are based at `QK_KB_0`**, not `SAFE_RANGE`. Vial exposes `vial.json`'s
  `customKeycodes` array as `QK_KB_0` upward in array order, so the enum and the JSON have to agree
  — verified against the in-tree
  [`nachie/subtext`](https://github.com/vial-kb/vial-qmk/blob/dd43959/keyboards/nachie/subtext/keymaps/vial/keymap.c)
  and `geekboards/macropad_v2` keymaps. With `SAFE_RANGE` (= `QK_USER_0`) they would still work,
  but the GUI could neither name nor place them.
* **`OS_LSFT` and its seven siblings are defined in `keymap.c`.** QMK master has them in
  `quantum/quantum_keycodes.h`; they post-date 0.29.0, so `vial-qmk` does not. The `#ifndef` guard
  means the block disappears by itself when `vial-qmk` rebases past that version.
* **Five layers fit comfortably.** `DYNAMIC_KEYMAP_LAYER_COUNT 5` costs 600 of the RP2040's 4096
  EEPROM bytes; with Vial's 32 slots each of tap dance, combos, key overrides and alt-repeat, about
  2.2 kB is left for dynamic macros. The build's own `STATIC_ASSERT` in `nvm_dynamic_keymap.c`
  confirms it.

## 5. Repository layout

```
qmk.json                                     build target: keebio/iris_ce/rev1 : cozy_de
keyboards/keebio/iris_ce/keymaps/cozy_de/
    keymap.c                                 layers, accent macros, the key-override table
    config.h                                 Vial UID and unlock combo, tapping, RGB
    rules.mk                                 VIA_ENABLE / VIAL_ENABLE / VIALRGB_ENABLE
    vial.json                                from vial-qmk's iris_ce vial keymap + customKeycodes
tools/provision.sh                           restores what a flash wipes (section 3)
```

`build_vial.mk` reads `$(KEYMAP_PATH)/vial.json`, and `KEYMAP_PATH` resolves into the userspace
(`build_keyboard.mk:155-177`), so `vial.json` belongs in the keymap directory. The `vial.json` and
the unlock combo come from `vial-qmk`'s own `keebio/iris_ce/keymaps/vial/`; the UID is a fresh one
from `python3 util/vial_generate_keyboard_uid.py`, deliberately not upstream's.

Building and flashing are covered by the ReadMe; the GitHub Actions workflow builds this target on
every push.

## 6. Discarded alternatives

* **A shared `keymap.c` with the old userspace.** Rejected: that keymap is being retired, and
  `vial-qmk` is about a year of QMK behind, so sharing would have meant `#ifdef` scaffolding for
  every post-0.29 feature. This is a hard fork.
* **Seeding the key overrides from firmware**, writing them into EEPROM from
  `keyboard_post_init_user()` with `dynamic_keymap_set_key_override()`. Works, and needs no
  external tool — but it is ~40 lines of new code in a keymap whose point was to have none, and it
  has to guess when not to stomp a GUI edit. Kept in reserve.
* **`QMK_SETTINGS = no` in `rules.mk`**, which the keymap *can* impose (`build_vial.mk` uses `?=`
  and `common_features.mk:647` includes it after the keymap's `rules.mk`). That would make
  `PERMISSIVE_HOLD` work from `config.h` again, at the price of the GUI's whole QMK Settings tab.
  The fallback if provisioning proves annoying.
* **Moving the Shift mapping into `process_record_user()`.** Honest, GUI-invisible, and it
  re-introduces exactly the custom code the `cozy_de` rewrite set out to delete.
* **A committed `.vil` file** as the provisioning input — see §3.

## 7. Not yet verified on hardware

The firmware builds clean and the reasoning above is all read out of `vial-qmk`'s source, but
nothing here has been run on an actual Iris CE. Worth checking on the first flash:

1. **`provision.sh` end to end.** Try `vitaly keyoverrides` (a plain dump) before trusting the
   write path.
2. **The `layers=65535` value in `provision.sh`.** The firmware side is unambiguous — `layers` is a
   bitmask and `~0` means every layer (`process_key_override.c:267`, and the header says so) — but
   `vitaly`'s README documents the field as `layers=1|2|3` without saying whether that is a raw
   mask or a list of layer indices. The script dumps all nine overrides at the end so the result
   can be read back; if they come out wrong, this is the field to suspect.
3. **Whether `vitaly` accepts the `DE_*` keycode names.** They are `keymap_extras` aliases —
   `DE_AT` is `ALGR(KC_Q)` — and `vitaly` promises "QMK keycodes notation together with aliases"
   without saying how far that reaches. If it rejects one, substitute the underlying keycode
   (`RALT(KC_Q)`), or enter that override in the GUI, which has an International keycode tab.
4. **The Shift mapping**: `Shift+9` → `+`, `Shift+0` → `?`, `Shift+6` → `ß`, `Shift+'` → `"`.
5. **The chameleon key**: hold Alt and tap `ä` repeatedly — the window switcher should stay open
   between taps. That is what the omitted `suppressed_mods` is for.
6. **Permissive hold**, by exercising the layer-taps on `y`, `-`, `Esc`, `Enter`, `Del`, `Ins`.
7. **The accent macros** `é è à ñ ç`, and the level-5 ones `¢ £` (Linux only, like everything
   behind E1's Level-5 latch).

## References

* Vial porting guide — <https://get.vial.today/docs/porting-to-vial.html>
* Vial user manual — <https://get.vial.today/manual/>
* `vitaly`, the VIA/Vial command-line tool — <https://github.com/bskaplou/vitaly>
* `vial-qmk` at `dd43959` — <https://github.com/vial-kb/vial-qmk/tree/dd43959ae5c08d8a28d38a1acf7b04e86b14a344>
  * `builddefs/build_vial.mk` — the build flags Vial forces
  * `quantum/vial.h`, `quantum/vial.c` — dynamic key overrides, the lock, the keycode firewall
  * `quantum/qmk_settings.c` — runtime tap/hold settings and their defaults
  * `quantum/dynamic_keymap.c`, `quantum/nvm/eeprom/nvm_dynamic_keymap.c` — EEPROM seeding and layout
  * `quantum/via.c`, `util/build_id.py` — the per-build EEPROM magic
  * `keyboards/keebio/iris_ce/keymaps/vial/` — the upstream Iris CE Vial keymap
* QMK 0.29.0 changelog — <https://docs.qmk.fm/ChangeLog/20250525>
* QMK Key Overrides — <https://docs.qmk.fm/features/key_overrides>
* QMK Caps Word — <https://docs.qmk.fm/features/caps_word>
* QMK userspace — <https://docs.qmk.fm/newbs_external_userspace>
