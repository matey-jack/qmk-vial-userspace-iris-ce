# Vial firmware for the Keebio Iris CE and Keebart Sofle Choc Pro

This repo only exists to run a GitHub Actions flow for building QMK-VIAL, cloned
from `git@github.com:vial-kb/vial-qmk.git`.

The build targets are keebio/iris_ce and keebart/sofle_choc_pro. Currently, I only really use this for the Sofle. 

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
appears. Both the Iris CE rev1 and the Sofle Choc Pro are RP2040-based, so
that is all there is to it.

## Build targets

`qmk.json` lists the build targets. Note that vial-qmk expects the **tuple** form
`["keyboard", "keymap"]` there. Current upstream QMK also accepts
`{"keyboard": ..., "keymap": ...}`, but vial-qmk's userspace schema predates
that and will reject it.

A keymap of our own lives in `keyboards/<keyboard>/keymaps/<name>/`, as
`{keymap.c,config.h,rules.mk,vial.json}`. QMK reads a userspace `config.h` only
from there, never from `keyboards/<keyboard>/`. A Vial keymap needs
`VIAL_ENABLE = yes` in `rules.mk`, its own `vial.json`, and a
`VIAL_KEYBOARD_UID` in `config.h`; start from the stock one in vial-qmk's
`keyboards/<keyboard>/keymaps/vial/`.

## The Sofle's `cozy_de` keymap

[`init_vial/sofle/`](init_vial/sofle/) explains how it is split between a
small custom firmware and a Vial `.vil` file that is loaded over USB.

## Building locally

```sh
git clone --recurse-submodules https://github.com/vial-kb/vial-qmk
python3 -m pip install -r vial-qmk/requirements.txt qmk
qmk config user.qmk_home=$PWD/vial-qmk
qmk config user.overlay_dir=$PWD/qmk-vial-userspace-iris-ce
cd qmk-vial-userspace-iris-ce
qmk userspace-compile
```
