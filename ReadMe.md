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

## Build targets

`qmk.json` currently builds the stock Vial keymap:

```json
["keebio/iris_ce/rev1", "vial"]
```

Note that vial-qmk expects the **tuple** form `["keyboard", "keymap"]` here.
Current upstream QMK also accepts `{"keyboard": ..., "keymap": ...}`, but
vial-qmk's userspace schema predates that and will reject it.

To customise the keymap, copy the stock one out of vial-qmk into this repo and
point the build target at it:

```
keyboards/keebio/iris_ce/keymaps/<name>/{keymap.c,config.h,rules.mk,vial.json}
```

A Vial keymap needs `VIAL_ENABLE = yes` in `rules.mk`, its own `vial.json`, and
a `VIAL_KEYBOARD_UID` in `config.h` — generate a fresh one with
`python3 util/vial_generate_keyboard_uid.py` from a vial-qmk checkout.

## Building locally

```sh
git clone --recurse-submodules https://github.com/vial-kb/vial-qmk
python3 -m pip install -r vial-qmk/requirements.txt qmk
qmk config user.qmk_home=$PWD/vial-qmk
qmk config user.overlay_dir=$PWD/qmk-vial-userspace-iris-ce
cd qmk-vial-userspace-iris-ce
qmk userspace-compile
```

## Porting the `cozy_de` keymap

The next step is bringing the `cozy_de` keymap over from
[`qmk_userspace_iris_cozy_keymap`](https://github.com/matey-jack/qmk_userspace_iris_cozy_keymap).
The plan, including a feature-by-feature check of what Vial does and does not
support and what has to change in the keymap, is in
[`docs/porting-cozy-de-to-vial.md`](docs/porting-cozy-de-to-vial.md).
