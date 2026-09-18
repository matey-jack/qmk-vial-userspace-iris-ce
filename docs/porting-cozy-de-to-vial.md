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

Vial can carry this keymap, but not unchanged. Four things need work, and one of them (§2.1)
silently changes behaviour rather than failing to compile.

| | |
|---|---|
| Ports as-is | layer structure, all four layers, `LT`/`OSL`/`TO`/`OSM`, Caps Word, the German keycodes, the accent macros, `process_record_user`, RGB Matrix, split-halves layer sync |
| Needs a small code change | one-shot modifier aliases `OS_LSFT` … (§2.4), custom keycode numbering (§2.3) |
| Needs provisioning after each flash | key overrides (§2.1), `PERMISSIVE_HOLD` (§2.2) — scriptable, see §6 |
| Does not port | nothing |

Both of the open questions from the first draft are now settled: the keymap is a **hard fork**
(§5), and the two "needs a decision" items are handled by a **provisioning script** built on the
`vitaly` Vial CLI rather than by firmware code (§6).

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

## 2. The four things that need work

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

The good news is that all nine are *representable* in Vial's world. Its on-disk override record
([`vial_key_override_entry_t`](https://github.com/vial-kb/vial-qmk/blob/dd43959/quantum/vial.h))
carries trigger, replacement, layers, trigger mods, negative mask, suppressed mods and options —
including `ko_option_one_mod` — so **`ko_adia_tab` with its `suppressed_mods = 0` trick survives
too**. Only `custom_action`/`context` are lost, and this keymap does not use them.

Three ways out:

1. **Provision them from a script after each flash** — a `.vil` file in the repo, applied with the
   `vitaly` CLI. Zero firmware code, and §6 shows it needs no manual unlock for this particular
   keymap. **Chosen.**
2. **Seed them from firmware**: keep the nine overrides as data in `keymap.c` and write them into
   EEPROM from `keyboard_post_init_user()` with `dynamic_keymap_set_key_override()`, then call
   `vial_init()` to reload Vial's RAM copy, guarded by a sentinel so a GUI edit is not stomped on
   every boot. ~40 lines, in a keymap whose stated goal was to have none. Keep in reserve: it is
   the answer if the script turns out to be a nuisance in practice.
3. **Move the Shift mapping into `process_record_user()`.** Honest, GUI-invisible, and it
   re-introduces exactly the custom code the `cozy_de` rewrite set out to delete. Rejected.

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

1. **Keep QMK Settings and set the bit from the provisioning script**: `vitaly settings -q 22 -v
   true`. Setting 22 is Permissive Hold — the `vitaly` setting IDs line up exactly with the
   `DECLARE_STATIC_BITSETTING(22, tapping_v2, …)` in `qmk_settings.c`. The Vial GUI keeps its
   QMK Settings tab. **Chosen**, see §6.
2. **`QMK_SETTINGS = no` in `rules.mk`.** `build_vial.mk` uses `?=`, and `common_features.mk:647`
   includes it *after* the keymap's `rules.mk`, so the keymap can veto it (confirmed). The `QS_*`
   macros then fall back to the compile-time `#define`s and `config.h` behaves exactly as it does
   today — no script needed for this one. Cost: no QMK Settings tab in the Vial GUI at all, and
   Auto Shift / one-shot timeouts / mouse-key speeds stop being runtime-tunable.

Recommendation: **(1)**, because the whole point of the port is to get the Vial GUI. (2) stays the
fallback if the settings keep getting lost in practice.

One catch that decides the shape of §6: **`.vil` files do not carry QMK Settings.** `vitaly load`
restores macros, key overrides, alt-repeat keys, combos, tap dances and keys — settings are a
separate subcommand. So the provisioning script needs both halves.

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

This is one instance of a *class* of problem: **`vial-qmk` is roughly a year of QMK behind the
`cozy` userspace**, so anything QMK added after 0.29.0 is missing here. The hard-fork decision in
§5 is what keeps that from becoming an ongoing tax.

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

Since the first draft of this document, `main` has grown the scaffolding and a working build:
`qmk.json`, the passthrough `Makefile`, and `.github/workflows/build.yml`, which reuses
[`qmk/.github/.github/workflows/qmk_userspace_build.yml@main`](https://github.com/qmk/.github/blob/main/.github/workflows/qmk_userspace_build.yml)
pointed at `vial-kb/vial-qmk` branch `vial`, plus a date-tagged release job on `main`.

That settles the two things this section previously listed as "verify on the first run": the
reusable workflow does check out `vial-qmk` usably for an RP2040 build, and `vial-qmk`'s
`requirements.txt` installs in the `qmk_cli` container. The build is green on `main` and was also
reproduced locally, producing a 295-block RP2040 `.uf2`. **Step 2 of the old plan — "get a Vial
build green before porting anything" — is done**, against the stock `vial` keymap.

One constraint worth repeating from the ReadMe: `vial-qmk`'s userspace schema only accepts the
tuple form `["keyboard", "keymap"]` in `qmk.json`. The object form current upstream QMK also takes
is rejected here.

### 4.1 What the port adds

```
qmk.json                                     build target: "vial" -> "cozy_de"
keyboards/keebio/iris_ce/keymaps/cozy_de/
    keymap.c                                 forked from the source keymap
    config.h                                 forked, plus Vial UID and unlock combo
    rules.mk                                 forked, plus VIA_ENABLE / VIAL_ENABLE / VIALRGB_ENABLE
    vial.json                                from vial-qmk's iris_ce vial keymap + customKeycodes
tools/
    cozy_de.vil                              the Vial-side config: key overrides et al (§6)
    provision.sh                             applies the .vil and the QMK Settings after a flash (§6)
```

`build_vial.mk` reads `$(KEYMAP_PATH)/vial.json`, and `KEYMAP_PATH` resolves into the userspace
(`build_keyboard.mk:155-177`), so `vial.json` belongs in the keymap directory here — confirmed,
not assumed.

### 4.2 Steps

1. **Fork the keymap**: copy `keymap.c`, `config.h` and `rules.mk` across; copy `vial.json`,
   `VIAL_KEYBOARD_UID` and the unlock combo out of `vial-qmk`'s `keebio/iris_ce/keymaps/vial/`.
   Generate a *fresh* UID with `python3 util/vial_generate_keyboard_uid.py` — the UID identifies
   this firmware to the GUI and should not be shared with upstream's keymap.
2. **Make it compile**: add the eight `OS_*` defines (§2.4), re-base the custom-keycode enum on
   `QK_KB_0` (§2.3), and list those ten keycodes in `vial.json`'s `customKeycodes` in the same
   order. Drop `ENABLE_RGB_MATRIX_KEY_GROUPS` (§3.2) and the now-pointless `debug_enable = true`
   (§3.4). Keep `DYNAMIC_KEYMAP_LAYER_COUNT 5` — it fits (§1.2).
3. **Point `qmk.json` at it** and get a green CI build with a downloadable `.uf2`.
4. **Flash and check the layers** in the Vial GUI: all four layers present, the ten custom keycodes
   named, RGB controllable. At this point the Shift mapping is still wrong — that is expected.
5. **Provision** (§6): configure the nine key overrides once, `vitaly save` them to
   `tools/cozy_de.vil`, commit it, and write `tools/provision.sh`.
6. **Test on hardware**: `Shift+9` → `+`, `Shift+0` → `?`, `Shift+6` → `ß`, the accent macros
   (`é è à ñ ç`), and `Alt` held while tapping `ä` repeatedly walking the window switcher without
   the list closing between taps. Then re-test the layer-taps to confirm permissive hold took.
7. **Version string** becomes e.g. `"Cozy-DE-Vial, rev01"`. The release job on `main` is date-based
   and needs no version gate, so the old repo's version ladder does not come across.
8. **ReadMe**: what differs from the non-Vial firmware — Caps Word inverts on Shift (§3.1), every
   flash resets the Vial side (§2.5), and that `provision.sh` is the answer to that.

## 5. Decision: hard fork

**The keymap is copied, not shared.** The `cozy_de` keymap in
`qmk_userspace_iris_cozy_keymap` is planned to retire once this one works, so the copy here becomes
the only copy and there is nothing to keep in sync. `cozy` (the US-ANSI keymap, which does use the
`getreuer/custom_shift_keys` community module) stays where it is.

This removes what would otherwise have been the awkward part of the port: `vial-qmk` sits on QMK
0.29.0 while the old userspace builds against QMK master, so a shared `keymap.c` would have needed
`#ifdef` scaffolding for every post-0.29 QMK feature — starting with §2.4 and growing over time.
A fork simply targets `vial-qmk` and is written against what `vial-qmk` has.

Two consequences worth acting on:

* The `#ifndef OS_LSFT` guard in §2.4 is still worth keeping, not for the old repo's sake but so
  the file keeps compiling when `vial-qmk` eventually rebases past that QMK version.
* Until the old `cozy_de` is actually retired, a fix made in one place does not reach the other.
  Keep the window short, and treat the old keymap as frozen once this one is flashed in anger.

## 6. Provisioning the Vial side from a script

This is the answer to both §2.1 (key overrides) and §2.2 (permissive hold), and it replaces the
"click it in the GUI after every flash" chore that §2.5 would otherwise impose.

### 6.1 The tool

[`vitaly`](https://github.com/bskaplou/vitaly) is a command-line client for the VIA/Vial protocol
(Rust, MIT, by @bskaplou — third-party, **not** an official `vial-kb` project). Install with
`cargo install vitaly`, `brew install bskaplou/tap/vitaly`, or a
[prebuilt binary](https://github.com/bskaplou/vitaly/releases/latest); on Linux it needs
`libudev-dev`.

It has the two subcommands this needs:

* `vitaly save -f x.vil` / `vitaly load -f x.vil` — `load` restores, in its own words, *"Macros,
  Key overrides, Alt repeat keys, Combos, TapDances, Keys"*. `-p` previews a file instead of
  writing it.
* `vitaly settings -q <qsid> -v <value>` — reads and writes QMK Settings by ID, with `-r` to reset
  all. The IDs match `qmk_settings.c` exactly: **22 = Permissive Hold**, 7 = Tapping Term,
  26 = Chordal Hold, 27 = Flow Tap.

Also useful: `vitaly devices` lists connected boards (`-i <id>` selects one), `vitaly lock` reports
and toggles the lock, and `vitaly bootload` drops the board into the bootloader.

### 6.2 Does it need the keyboard unlocked?

Mostly **no**, which is what makes an unattended script possible. Unlocking is not scriptable —
`vitaly lock -u` prints *"Push marked buttons and keep then pushed to unlock"* and waits for a
physical key hold on the two keys named by `VIAL_UNLOCK_COMBO_ROWS` / `_COLS`.

Reading `vial-qmk`, exactly four things are gated on the lock:

| operation | gated? | where |
|---|---|---|
| key overrides, combos, tap dances, alt-repeat | **no** | `vial.c:286-320` |
| keymap keycodes | **no** | `via.c` dynamic keymap commands |
| QMK Settings get / set / reset | **no** | `vial.c:210-222` |
| dynamic **macros** | yes | `via.c:403` |
| bootloader jump (`vitaly bootload`) | yes | `via.c:438` |
| matrix tester | yes | `via.c:253` |
| writing `QK_BOOT` as a keycode | yes | `vial.c:81` |

So for `cozy_de` the whole provisioning run works on a locked keyboard. The keymap's "macros" are
firmware custom keycodes (`MX_EACU` and friends), not Vial *dynamic* macros, so its macro set is
empty and the one gated part of `vitaly load` writes nothing. Note that the firmware silently skips
a gated write rather than erroring, so `vitaly` will still print "Macros restored" — do not read
that as proof that macros were written. If dynamic macros are ever added, the script needs a
`vitaly lock -u` and a human holding two keys.

### 6.3 The script

Produce `tools/cozy_de.vil` once, from a board configured the way you want it:

```sh
vitaly -i <id> save -f tools/cozy_de.vil
```

Commit it — it is the Vial-side counterpart to `keymap.c`, and it is the one artifact that says
what the nine key overrides are. Then `tools/provision.sh`, run after every flash:

```sh
#!/usr/bin/env bash
# Restore the Vial-side configuration that a firmware flash wipes (see docs/, section 2.5).
# Needs https://github.com/bskaplou/vitaly on PATH. No unlock required: nothing here is a
# dynamic macro, a bootloader jump or a QK_BOOT keycode write.
set -euo pipefail
cd "$(dirname "$0")"

vitaly devices                            # sanity check: exactly one Iris CE, and its id
vitaly load -f cozy_de.vil                # key overrides, combos, tap dances, and the keymap
vitaly settings -q 22 -v true             # Permissive Hold -- not carried in the .vil
vitaly settings -q 7                      # echo Tapping Term; expect 500 from config.h
```

Add `-i <id>` to each call once the board's USB product ID is known, otherwise `vitaly` acts on
every connected VIA/Vial keyboard.

### 6.4 Caveats

* **Unverified on this hardware.** Every claim above about `vitaly` comes from its README and from
  reading `vial-qmk`; none of it has been run against an Iris CE. Try `vitaly load -p` (preview)
  and a `vitaly settings` read before trusting the write path.
* **`vitaly load` also rewrites the keymap layers**, not just the extras. That is usually what you
  want after a flash, but it means a stale `.vil` will quietly undo a `keymap.c` change. Re-`save`
  the `.vil` whenever the layers change, or be ready to explain to yourself why the new layout did
  not take.
* **Third-party tool.** The fallback, if `vitaly` disappoints, is the Vial GUI's own
  *File -> Save/Load current layout*, plus ticking Permissive Hold by hand — i.e. the manual
  version of the same two steps.
* If provisioning proves annoying enough that it gets skipped, that is the signal to switch to
  firmware seeding (§2.1 option 2) and `QMK_SETTINGS = no` (§2.2 option 2), which need no tooling
  at all.

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
* `vitaly`, the VIA/Vial command-line tool — <https://github.com/bskaplou/vitaly>
* Vial user manual (GUI save/load of `.vil` files) — <https://get.vial.today/manual/>
