# Porting the `cozy_de` keymap to QMK-Vial

Status: **plan / feasibility study**. Nothing is ported yet.

Source: [`matey-jack/qmk_userspace_iris_cozy_keymap`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap),
keymap `keyboards/keebio/iris_ce/keymaps/cozy_de`.

Target: this repository, built against
[`vial-kb/vial-qmk`](https://github.com/vial-kb/vial-qmk) (branch `vial`).

Terms used below:

* **QMK** — Quantum Mechanical Keyboard firmware, the upstream keyboard firmware this all derives from.
* **VIA** — a protocol plus GUI that lets a keyboard's keymap be re-mapped at runtime over raw HID,
  storing the result in the keyboard's EEPROM instead of in the compiled firmware.
* **Vial** — a fork of VIA (GUI: [vial.rocks](https://get.vial.today/)) with its own firmware fork,
  `vial-qmk`. It adds runtime-editable tap dance, combos, key overrides and a "QMK Settings" tab.
* **EEPROM** — the keyboard's small non-volatile memory. On the Iris CE's RP2040 it is *emulated*
  in flash by QMK's wear-levelling driver, not a real EEPROM chip.
* **Static keymap** — the `keymaps[][][]` array compiled into `keymap.c`. Under Vial it is only the
  *seed* for the EEPROM copy, which is what the firmware actually reads at runtime.

## 0. Verdict up front

Vial can carry this keymap, but not unchanged. Four things need a decision before any code is
written, and one of them (§2.1) silently changes behaviour rather than failing to compile.

| | |
|---|---|
| Ports as-is | layer structure, all four layers, `LT`/`OSL`/`TO`/`OSM`, Caps Word, the German keycodes, the accent macros, `process_record_user`, RGB Matrix, split-halves layer sync |
| Needs a small code change | one-shot modifier aliases `OS_LSFT` … (§2.4), custom keycode numbering (§2.3) |
| Needs a design decision | key overrides (§2.1), `PERMISSIVE_HOLD` (§2.2) |
| Does not port | nothing |

## 1. What was checked, and how

`vial-qmk` was cloned at commit
[`dd43959`](https://github.com/vial-kb/vial-qmk/commit/dd43959ae5c08d8a28d38a1acf7b04e86b14a344)
(2026-07-27) and every keycode, `#define` and build flag used by `cozy_de/keymap.c`,
`config.h` and `rules.mk` was grepped against it.

Two facts frame everything else:

1. **`vial-qmk` tracks QMK 0.29.0 (2025-05-25)**, the newest entry in its
   [`docs/ChangeLog`](https://github.com/vial-kb/vial-qmk/tree/dd43959/docs/ChangeLog).
   The `cozy` userspace builds against `qmk/qmk_firmware` **master**, which by now is well ahead of
   that. Anything added to QMK after 0.29.0 is unavailable here — that is exactly what bites in §2.4.
2. **The Iris CE is already a Vial keyboard upstream.** `vial-qmk` carries
   [`keyboards/keebio/iris_ce/keymaps/vial/`](https://github.com/vial-kb/vial-qmk/tree/dd43959/keyboards/keebio/iris_ce/keymaps/vial)
   with a ready `vial.json`, `config.h` (UID + unlock combo) and `rules.mk`. The physical layout,
   the `LAYOUT` macro and the 10×6 matrix are identical to what `cozy_de` uses, so the port starts
   from a known-good definition rather than from a hand-written one.

Everything below that says "confirmed" was read in the `vial-qmk` source, not recalled.

### 1.1 Feature-by-feature result

| Feature used by `cozy_de` | In `vial-qmk`? | Note |
|---|---|---|
| `LAYOUT` macro, 10×6 matrix, RP2040, `.uf2` output | yes | same keyboard files as upstream QMK |
| 4 layers, `DYNAMIC_KEYMAP_LAYER_COUNT 5` | yes | EEPROM budget checked, see §1.2 |
| `LT()`, `OSL()`, `TO()`, `MO`-style holds | yes | core QMK |
| `OSM()` one-shot mods | yes | but the `OS_LSFT` *aliases* are missing — §2.4 |
| `CAPS_WORD_ENABLE`, `BOTH_SHIFTS_TURNS_ON_CAPS_WORD` | yes | `process_caps_word.c:99`; Vial additionally forces `CAPS_WORD_INVERT_ON_SHIFT` — §3.1 |
| `KEY_OVERRIDE_ENABLE` + `key_overrides[]` array | **partly** | the feature is there; the *static array* is ignored — §2.1 |
| `PERMISSIVE_HOLD`, `TAPPING_TERM 500` | **partly** | `TAPPING_TERM` is honoured, `PERMISSIVE_HOLD` is not — §2.2 |
| custom keycodes + `process_record_user()` | yes | numbering must change — §2.3 |
| `keymap_german.h`, `sendstring_german.h` | yes | all 45 `DE_*` keycodes used by the keymap verified present |
| `tap_code16_delay()`, `send_string_with_delay()` | yes | `quantum.h:282`, `send_string.h:66` |
| `RGB_MATRIX_ENABLE`, `RM_*` keycodes, `RGB_MATRIX_TIMEOUT`, `RGB_MATRIX_SLEEP` | yes | §3.2 |
| `MS_WHLU` / `MS_WHLD` mouse-wheel keycodes | yes | `keycodes.h:967`; `mousekey` is on in the Iris CE `keyboard.json` |
| `EE_CLR`, `QK_BOOT` | yes | `QK_BOOT` from the static keymap is fine — §3.3 |
| `SPLIT_LAYER_STATE_ENABLE` | yes | core QMK |
| `CONSOLE_ENABLE` + `println()` for `MX_VERS` | yes | Vial forces `-DNO_DEBUG`, which kills `dprintf` but **not** `println` — §3.4 |
| community modules (`getreuer/custom_shift_keys`) | n/a | `cozy_de` deliberately uses none; only plain `cozy` does |
| Unicode input | n/a | `cozy_de` deliberately uses none |
| QMK userspace build (`qmk userspace-compile`) | yes | §4 |

### 1.2 EEPROM budget (checked, fits)

The RP2040 gets 4096 bytes of logical EEPROM (`WEAR_LEVELING_BACKING_SIZE` 8192, logical =
backing / 2, see
[`wear_leveling_rp2040_flash_config.h`](https://github.com/vial-kb/vial-qmk/blob/dd43959/platforms/chibios/drivers/wear_leveling/wear_leveling_rp2040_flash_config.h)).
Because that is above the 4000-byte threshold in
[`vial.h`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.h), Vial allocates its
maximum of 32 entries each for tap dance, combos, key overrides and alt-repeat. With
`DYNAMIC_KEYMAP_LAYER_COUNT 5`:

| region | bytes |
|---|---|
| VIA config header | ~38 |
| keymap, 5 × 10 × 6 × 2 | 600 |
| QMK Settings | 40 |
| tap dance / combos / key overrides, 32 × 10 each | 960 |
| alt-repeat, 32 entries | ~192 |
| **left for dynamic macros** | **~2260** |

So five layers are not a problem. (Layout per
[`nvm_dynamic_keymap.c`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/nvm/eeprom/nvm_dynamic_keymap.c).)

## 2. The four things that need a decision

### 2.1 Key overrides: the static `key_overrides[]` array is silently ignored

This is the important one, because **it compiles and links cleanly and then does nothing**.

`cozy_de` expresses its whole Shift mapping — `Shift+2 → @`, `Shift+6 → ß`, `Shift+9 → +`,
`Shift+0 → ?`, the `'`/`"` pairing, and the ä/Tab "chameleon" key — as nine entries in
`const key_override_t *key_overrides[]`. QMK reads that array through two functions that are
declared *weak* in
[`keymap_introspection.c:161,174`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/keymap_introspection.c):
`key_override_count()` and `key_override_get()`.

`vial-qmk` defines both of them *strongly* in
[`vial.c:648,652`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.c), reading the
overrides out of EEPROM instead. And that path is not opt-in:
[`vial.h:142`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.h) turns
`VIAL_KEY_OVERRIDE_ENABLE` on for any build that has `KEY_OVERRIDE_ENABLE` — which
[`build_vial.mk:11`](https://github.com/vial-kb/vial-qmk/blob/dd43959/builddefs/build_vial.mk)
sets to `yes` by default in every Vial build.

Worse, EEPROM key overrides are **not** seeded from the static array. `dynamic_keymap_reset()`
([`dynamic_keymap.c:162-170`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/dynamic_keymap.c))
seeds the *keymap layers* from `keymaps[][][]` in flash — so the layers do survive the trip — but
it fills all 32 key-override slots with a zeroed, disabled entry.

Net effect of a naive port: the keymap looks right in Vial, and `Shift+9` types `)`.

Three ways out, in order of preference:

1. **Seed the overrides from firmware.** Keep the nine overrides as data in `keymap.c`, and in
   `keyboard_post_init_user()` write them into EEPROM with `dynamic_keymap_set_key_override()`,
   then call `vial_init()` to reload Vial's RAM copy. Guard it with a sentinel (slot 0 still at its
   zeroed reset value ⇒ the EEPROM was just wiped ⇒ re-seed), so a user who edits overrides in the
   GUI is not overwritten on every boot. Vial's on-disk override record
   ([`vial_key_override_entry_t`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.h))
   carries trigger, replacement, layers, trigger mods, negative mask, suppressed mods and options —
   including `ko_option_one_mod` — so **all nine overrides are representable, `ko_adia_tab` with
   its `suppressed_mods = 0` trick included**. The only fields lost are `custom_action`/`context`,
   which this keymap does not use.
   Cost: ~40 lines of new code, in a keymap whose stated goal was to have none.
2. **Move the Shift mapping into `process_record_user()`.** Honest, GUI-invisible, and it
   re-introduces exactly the custom code the `cozy_de` rewrite set out to delete.
3. **Enter the nine overrides by hand in the Vial GUI, once, and keep a `.vil` layout file in the
   repo** to re-load them after each flash. Zero code. But see §2.5: *every* firmware flash wipes
   them, so this is a manual step on every single update.

Recommendation: **(1)**, with the `.vil` file from (3) kept as a convenience export.

### 2.2 `PERMISSIVE_HOLD` is ignored; `TAPPING_TERM` is not

Vial builds enable its "QMK Settings" framework by default
([`build_vial.mk:4`](https://github.com/vial-kb/vial-qmk/blob/dd43959/builddefs/build_vial.mk)),
which moves a set of tap/hold options from compile time to runtime. `get_permissive_hold()`
([`qmk_settings.c:293`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/qmk_settings.c))
then returns a *stored bit*, and `qmk_settings_reset()` (same file, line 190) sets
`QS.tapping_v2 = 0` — permissive hold **off** — no matter what `config.h` says.

`#define TAPPING_TERM 500` does survive: `qmk_settings_reset()` seeds `QS.tapping_term` from it.
Two other forced flags default to harmless values: `CHORDAL_HOLD` is off (same `tapping_v2` byte)
and Flow Tap is off (`QS.flow_tap_term = 0`, despite `-DFLOW_TAP_TERM=321`). Auto Shift is
compiled in but off.

For a keymap that puts layer-taps on `y`, `-`, `Esc`, `Enter`, `Del` and `Ins` and relies on
permissive hold to make them usable at a 500 ms tapping term, losing it is not a detail.

Options:

1. **`QMK_SETTINGS = no` in `rules.mk`.** `build_vial.mk` uses `?=`, and `common_features.mk:647`
   includes it *after* the keymap's `rules.mk`, so the keymap can veto it (confirmed). The
   `QS_*` macros then fall back to the compile-time `#define`s and `config.h` behaves exactly as
   it does today. Cost: no QMK Settings tab in the Vial GUI, and Auto Shift / layer lock /
   alt-repeat stop being runtime-configurable.
2. **Keep QMK Settings and tick "Permissive Hold" in the GUI** — and re-tick it after every flash
   (§2.5), or seed the bit from firmware the same way as §2.1.

Recommendation: **(1)** if the point of the port is a Vial *keymap editor*; **(2)** if the point is
the full Vial settings experience. This is the main open question for you.

### 2.3 Custom keycodes must start at `QK_KB_0`, not `SAFE_RANGE`

`cozy_de` declares `MX_VERS = SAFE_RANGE` and nine more custom keycodes. `SAFE_RANGE` is
`QK_USER_0` = `0x7E40` (`quantum_keycodes.h:37`). Vial's GUI, however, exposes the custom keycodes
declared in `vial.json`'s `customKeycodes` array as `QK_KB_0` = `0x7E00` upward, in array order —
confirmed against the two in-tree examples,
[`nachie/subtext`](https://github.com/vial-kb/vial-qmk/blob/dd43959/keyboards/nachie/subtext/keymaps/vial/keymap.c)
(`JWRDL = QK_KB_0`) and `geekboards/macropad_v2` (`ALT_TAB = QK_KB_0`).

So: change the enum base to `QK_KB_0` and list all ten keycodes in `vial.json` in the same order.
`QK_KB_0 … QK_KB_31` gives 32 slots; ten is fine. Keeping `SAFE_RANGE` would still *work* in the
firmware but the keycodes would be un-nameable and un-placeable in the GUI, and would show as raw
hex.

Suggested `customKeycodes` order (matching the existing enum):
`MX_VERS, MX_EACU, MX_EGRV, MX_AGRV, MX_NTIL, MX_CCED, MX_HAT, MX_BTIC, MX_CENT, MX_PND`.

### 2.4 `OS_LSFT` and friends do not exist in `vial-qmk`

The `L_FN` layer uses `OS_LSFT`, `OS_RSFT`, `OS_LCTL`, `OS_RCTL`, `OS_LGUI`, `OS_RGUI`, `OS_LALT`,
`OS_RALT`. These aliases live in `quantum/quantum_keycodes.h` on QMK **master** (verified:
`#define OS_LSFT OSM(MOD_LSFT)` and siblings), but they are **absent from `vial-qmk`** — a
whole-tree grep finds the name only inside one unrelated keyboard's keymap, which defines it
itself. They post-date QMK 0.29.0.

This is a hard compile error, and the fix is three lines of comment and eight of code in
`keymap.c`:

```c
/* Not in vial-qmk (QMK 0.29.0); added to QMK master later. */
#ifndef OS_LSFT
#    define OS_LCTL OSM(MOD_LCTL)
#    define OS_LSFT OSM(MOD_LSFT)
/* ... and the other six */
#endif
```

The `#ifndef` guard means the same file keeps compiling if `vial-qmk` later rebases past that QMK
version.

This one is worth treating as a *class* of problem, not a single bug: **`vial-qmk` is roughly a
year of QMK behind the `cozy` userspace**, so the two keymaps will drift. See §5.

### 2.5 Every flash wipes the EEPROM (this cuts both ways)

Under Vial, the EEPROM-validity magic is `BUILD_ID`
([`via.c:86-98`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/via.c)), and `BUILD_ID`
is a **random 24-bit number generated at every build** by
[`util/build_id.py`](https://github.com/vial-kb/vial-qmk/blob/dd43959/util/build_id.py), injected
in `build_keyboard.mk:296`.

Consequences:

* **Good:** the `rules.mk` comment in `cozy_de` — *"VIA_ENABLE messes with static keymap updates
  (from keymap.c)"* — stops being true. Every new firmware invalidates the magic, so
  `dynamic_keymap_reset()` re-seeds the layers from `keymap.c`. No manual `EE_CLR` needed.
* **Bad:** it also wipes everything the user configured in Vial — macros, combos, tap dances, key
  overrides, QMK Settings. Which is the strongest argument for seeding overrides and settings from
  firmware (§2.1, §2.2) rather than clicking them in.

One quirk to be aware of when testing: `vial_init()` runs in `keyboard_setup()` *before*
`via_init()` runs `dynamic_keymap_reset()` in `keyboard_init()`, and nothing reloads Vial's RAM
copies afterwards. So on the **first** boot after a flash, key overrides / combos / tap dances in
RAM can be stale; a power cycle settles it. Seeding in `keyboard_post_init_user()` and calling
`vial_init()` there (§2.1) happens to fix this too.

## 3. Smaller things, no decision needed

### 3.1 `CAPS_WORD_INVERT_ON_SHIFT` is forced on

`build_vial.mk:15` adds `-DCAPS_WORD_INVERT_ON_SHIFT` unconditionally. With it, pressing Shift
during Caps Word inverts the shift for the next key instead of ending Caps Word.
`BOTH_SHIFTS_TURNS_ON_CAPS_WORD` still works. This is a behaviour change from the current
firmware, but a mild and arguably pleasant one. Cannot be switched off from the keymap; note it in
the ReadMe so it is not mistaken for a bug.

### 3.2 RGB

`VIALRGB_ENABLE = yes` (as upstream's Iris CE Vial keymap has it) hands RGB control to the Vial
GUI while leaving the `RM_*` keycodes working. The `#undef ENABLE_RGB_MATRIX_*` block in
`config.h` ports unchanged: VialRGB builds its effect list from `#ifdef RGB_MATRIX_EFFECT_*`
guards ([`vialrgb_effects.inc:52ff`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vialrgb_effects.inc)),
so disabling animations just shortens the list the GUI offers. `#undef` of an animation that
`vial-qmk` does not have yet (e.g. `FLOWER_BLOOMING`) is harmless.

Two notes while we are in there:

* `#define ENABLE_RGB_MATRIX_KEY_GROUPS` **does not correspond to anything**. It exists neither in
  `vial-qmk` nor in current QMK master
  ([`rgb_matrix_effects.inc`](https://github.com/qmk/qmk_firmware/blob/master/quantum/rgb_matrix/animations/rgb_matrix_effects.inc)
  has no such effect). It is a dead line in the *existing* keymap; it should be dropped here, and
  probably upstream too.
* `SPLIT_LAYER_STATE_ENABLE` is commented "needed for layer-aware RGB keylights", but the keymap
  has no `rgb_matrix_indicators_advanced_user()`. Harmless, just unexplained.

### 3.3 `QK_BOOT` and the Vial lock

Vial refuses to *write* `QK_BOOT` into the keymap from the GUI unless the keyboard is unlocked
(`vial_keycode_firewall()`, `vial.c:80`). A `QK_BOOT` that comes from the static keymap is fine:
`dynamic_keymap_reset()` unlocks for the duration of the seeding, with a comment saying exactly
that. So the two `QK_BOOT` keys on `L_FN` keep working. Unlocking is done with the combo in
`config.h`; upstream's Iris CE keymap uses `VIAL_UNLOCK_COMBO_ROWS {0,9}` / `COLS {0,5}`.

### 3.4 Console output

Vial builds force `-DNO_DEBUG`, which compiles out `dprintf`/`dprint` but **not** `println`, which
is guarded by `NO_PRINT` (`print.h:51,74`). So `MX_VERS` keeps working with `CONSOLE_ENABLE = yes`.
`debug_enable = true` in `keyboard_post_init_user()` becomes pointless, though — it only gates
`dprintf`. Drop it, and with it the "DOESN'T WORK YET!" comment.

## 4. Repository and build plan

### 4.1 Layout

Mirror the source userspace, so the two stay diff-able:

```
qmk.json                                     userspace_version 1.1, build target keebio/iris_ce/rev1 : cozy_de
Makefile                                     same passthrough Makefile as the cozy userspace
keyboards/keebio/iris_ce/keymaps/cozy_de/
    keymap.c                                 ported from the source keymap
    config.h                                 ported, plus Vial UID and unlock combo
    rules.mk                                 ported, plus VIA_ENABLE / VIAL_ENABLE / VIALRGB_ENABLE
    vial.json                                copied from vial-qmk's iris_ce vial keymap + customKeycodes
    cozy_de.vil                              optional: exported Vial layout, for re-import after a flash
.github/workflows/build-on-push.yaml         adapted from the source repo
docs/porting-cozy-de-to-vial.md              this file
```

`vial-qmk` supports external userspaces: `QMK_USERSPACE` is handled in its `Makefile` and
`build_keyboard.mk:155-177`, `lib/python/qmk/cli/userspace/` provides `qmk userspace-compile`, and
`data/schemas/user_repo_v1_1.jsonschema` matches the `"userspace_version": "1.1"` the source repo
uses. `build_vial.mk` reads `$(KEYMAP_PATH)/vial.json`, and `KEYMAP_PATH` resolves into the
userspace, so `vial.json` belongs in the keymap directory here (confirmed).

### 4.2 CI

The source workflow calls the reusable
[`qmk/.github/.github/workflows/qmk_userspace_build.yml@main`](https://github.com/qmk/.github/blob/main/.github/workflows/qmk_userspace_build.yml),
which takes `qmk_repo`, `qmk_ref` and `preparation_command` inputs and runs
`qmk userspace-compile` in the `ghcr.io/qmk/qmk_cli:latest` container. So:

```yaml
    uses: qmk/.github/.github/workflows/qmk_userspace_build.yml@main
    with:
      qmk_repo: vial-kb/vial-qmk
      qmk_ref: vial              # vial-qmk's default branch, not "master"
```

Two things to verify on the first CI run rather than assume:

* whether the reusable workflow checks out `qmk_firmware` **with submodules** — an RP2040 build
  needs `lib/chibios` and `lib/chibios-contrib`. If not, `preparation_command: make git-submodule`
  is the escape hatch the workflow provides for exactly this.
* whether `vial-qmk`'s `requirements.txt` installs cleanly in that container.

Start with a bare build workflow. The source repo's version/tag/release machinery is good but
independent of the port; port it afterwards, in its own change, with `cozy` dropped from the
two-keymap logic.

### 4.3 Steps

1. **Skeleton**: `qmk.json`, `Makefile`, `.gitignore`, keymap directory. Copy `vial.json`,
   `VIAL_KEYBOARD_UID` and the unlock combo from `vial-qmk`'s `keebio/iris_ce/keymaps/vial/`.
   Generate a *fresh* UID with `python3 util/vial_generate_keyboard_uid.py` — the UID identifies
   this firmware to the GUI and should not be shared with upstream's keymap.
2. **Minimal build**: bare `rules.mk` (`VIA_ENABLE`, `VIAL_ENABLE`, `VIALRGB_ENABLE`,
   `RGB_MATRIX_ENABLE`, `CAPS_WORD_ENABLE`, `KEY_OVERRIDE_ENABLE`) and the upstream `vial` keymap's
   `keymap.c`, unchanged. Get CI green and a `.uf2` artifact **before** porting anything. This
   isolates "does Vial build in a userspace at all" from "does `cozy_de` port".
3. **Port `keymap.c`**: all four layers verbatim, plus the `OS_*` defines (§2.4) and the
   `QK_KB_0` enum base (§2.3). Fill in `customKeycodes` in `vial.json` to match.
4. **Port `config.h`**: unchanged except — drop `ENABLE_RGB_MATRIX_KEY_GROUPS` (§3.2), add the Vial
   UID/unlock defines, keep `DYNAMIC_KEYMAP_LAYER_COUNT 5` (it fits, §1.2).
5. **Decide §2.2**, then either set `QMK_SETTINGS = no` or add the settings-seeding code.
6. **Decide §2.1**, then implement the key-override seeding (or its alternative). Test on hardware:
   `Shift+9` → `+`, `Shift+0` → `?`, `Shift+6` → `ß`, and `Alt`-held repeated `ä` walking the
   window switcher without the list closing between taps.
7. **Version string**: `VERSION_STRING` becomes e.g. `"Cozy-DE-Vial, rev01"`, and the release
   workflow's version gate is ported to a single keymap.
8. **ReadMe**: what differs from the non-Vial firmware — Caps Word inverts on Shift (§3.1), every
   flash resets Vial customisations (§2.5), and where the `.vil` file lives.

Steps 1-4 are mechanical. Steps 5-6 are the actual work.

## 5. The thing to decide before any of this: how the two keymaps stay in sync

`vial-qmk` sits on QMK 0.29.0; the `cozy` userspace builds on QMK master. §2.4 is the first symptom
and will not be the last — a keycode or a feature added to QMK after 2025-05 is simply not
available here. Three postures:

* **Fork and drift.** Copy `cozy_de` here, fix it up, accept that the two diverge. Simplest now,
  worst later.
* **One source, two builds.** Keep `keymap.c` identical in both repositories, with the `vial-qmk`
  gaps behind `#ifndef`/`#ifdef VIAL_ENABLE` guards (as sketched in §2.4), and a script or
  submodule that copies it across. The `cozy_de` keymap is already written in a style that makes
  this realistic: no community modules, almost no custom code.
* **Vial only.** If the Vial build turns out to be strictly better — GUI editing, no `EE_CLR`
  dance — retire the non-Vial `cozy_de` and keep `cozy` (the US-ANSI one, which does use a
  community module) where it is.

Worth answering before step 3, because it decides whether the ported `keymap.c` is a copy or a
shared file.

## References

* Vial porting guide — <https://get.vial.today/docs/porting-to-vial.html>
* `vial-qmk` at `dd43959` — <https://github.com/vial-kb/vial-qmk/tree/dd43959ae5c08d8a28d38a1acf7b04e86b14a344>
  * `builddefs/build_vial.mk` — the forced build flags
  * `quantum/vial.h`, `quantum/vial.c` — dynamic tap dance / combos / key overrides
  * `quantum/qmk_settings.c` — runtime tap/hold settings and their defaults
  * `quantum/dynamic_keymap.c`, `quantum/nvm/eeprom/nvm_dynamic_keymap.c` — EEPROM seeding and layout
  * `quantum/via.c`, `util/build_id.py` — the per-build EEPROM magic
  * `keyboards/keebio/iris_ce/keymaps/vial/` — the upstream Iris CE Vial keymap
* QMK 0.29.0 changelog — <https://docs.qmk.fm/ChangeLog/20250525>
* QMK Key Overrides — <https://docs.qmk.fm/features/key_overrides>
* QMK Caps Word — <https://docs.qmk.fm/features/caps_word>
* QMK userspace — <https://docs.qmk.fm/newbs_external_userspace>
* Reusable userspace build workflow — <https://github.com/qmk/.github/blob/main/.github/workflows/qmk_userspace_build.yml>
