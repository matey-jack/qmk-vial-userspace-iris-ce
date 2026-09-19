# Vial firmware for the Keebio Iris CE

This repo only exists to run a GitHub Actions flow for building QMK-VIAL, cloned
from `git@github.com:vial-kb/vial-qmk.git`.

The keyboard is set to keebio/iris_ce.

It is a [QMK external userspace][userspace]: the firmware sources live in
vial-qmk, this repo only carries the build targets (`qmk.json`) and any keymaps
we want to override.

[userspace]: https://docs.qmk.fm/newbs_external_userspace

## What the Actions workflow does

`.github/workflows/build.yml` runs on every push and can also be started by hand
(*Actions -> Build Vial firmware -> Run workflow*).

* **Build** — reuses QMK's own [`qmk_userspace_build.yml`][reusable] reusable
  workflow, pointed at `vial-kb/vial-qmk` (branch `vial`) instead of
  `qmk/qmk_firmware`. It compiles every target listed in `qmk.json` and uploads
  the binaries as a workflow artifact named **Firmware**. This happens on *all*
  branches, so a push to a feature branch gives you a downloadable `.uf2` from
  the run's summary page.
* **Release** — only on `main`. It downloads that artifact and publishes a
  GitHub release named after the build date, e.g. `2026.09.14`. A second build
  on the same day becomes `2026.09.14.2`, a third `2026.09.14.3`, and so on, so
  a release is never overwritten.

[reusable]: https://github.com/qmk/.github/blob/main/.github/workflows/qmk_userspace_build.yml

## Flashing

Download the `.uf2` from a release (or from a branch build's artifact), put the
board into bootloader mode, and copy the file onto the `RPI-RP2` drive that
appears. The Iris CE rev1 is RP2040-based, so that is all there is to it.

## After flashing: run `tools/provision.sh`

Vial keeps part of its configuration in the keyboard's EEPROM, and every flash
wipes it — because the EEPROM-validity magic is a random number regenerated on
each build. The keymap layers are re-seeded from `keymap.c` automatically, which
is what we want, but the **key overrides** (the whole Cozy Shift mapping:
`Shift+9` → `+`, `Shift+0` → `?`, the ä/Tab chameleon key, …) and the
**permissive-hold setting** are not. So after every flash:

```sh
tools/provision.sh
```

It needs [`vitaly`](https://github.com/bskaplou/vitaly), a command-line client
for the VIA/Vial protocol (`cargo install vitaly`, `brew install
bskaplou/tap/vitaly`, or a release binary). No unlocking required — nothing it
writes is behind Vial's lock.

## The `cozy_de` keymap

`keyboards/keebio/iris_ce/keymaps/cozy_de/` is the Vial edition of the `cozy_de`
keymap, forked from
[`qmk_userspace_iris_cozy_keymap`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap).
It is a hard fork: that keymap is being retired, so this is the only copy.

What Vial made us do differently — and why key overrides and permissive hold
live in a script rather than in the firmware — is in
[`docs/porting-cozy-de-to-vial.md`](docs/porting-cozy-de-to-vial.md).

## Build targets

`qmk.json` builds the `cozy_de` keymap:

```json
["keebio/iris_ce/rev1", "cozy_de"]
```

Note that vial-qmk expects the **tuple** form `["keyboard", "keymap"]` here.
Current upstream QMK also accepts `{"keyboard": ..., "keymap": ...}`, but
vial-qmk's userspace schema predates that and will reject it.

To add another keymap, put it at

```
keyboards/keebio/iris_ce/keymaps/<name>/{keymap.c,config.h,rules.mk,vial.json}
```

and add it to `build_targets`. A Vial keymap needs `VIAL_ENABLE = yes` in
`rules.mk`, its own `vial.json`, and a `VIAL_KEYBOARD_UID` in `config.h` —
generate a fresh one with `python3 util/vial_generate_keyboard_uid.py` from a
vial-qmk checkout.

## Building locally

```sh
git clone --recurse-submodules https://github.com/vial-kb/vial-qmk
python3 -m pip install -r vial-qmk/requirements.txt qmk
qmk config user.qmk_home=$PWD/vial-qmk
qmk config user.overlay_dir=$PWD/qmk-vial-userspace-iris-ce
cd qmk-vial-userspace-iris-ce
qmk userspace-compile
```
