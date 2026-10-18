# DBZ Budokai 3 for PS5

The PS5 port of NovaPowers' [DBZ Budokai 3 HD Collection recomp](https://github.com/novapowers0/DBZ-Budokai-3-HD-Collection).
His game code is in `src/` (as he has it, plus `[ps5 port]` changes); this folder has the PS5 side: the hub, the PS5 start-up, the build and the packaging.

## Build

Needs, side by side:
- the runtime: `dbz-budokai3-ps5-runtime` (the WWE 2K14 PS5 runtime with this game's runtime changes merged in), its `thirdparty/` filled from ReXGlue 0.10.0's submodules
- [ps5-plume-triangle](https://github.com/Nazky/ps5-plume-triangle), built once (`make`)
- your own `yae3_xenon.xex` (the USA disc's `DBZ3/yae3_xenon.xex`) at the project root

```
rexglue codegen dbz3_manifest.toml          # the host build of the runtime's rexglue
python3 ps5/tools/ps5_patch_generated.py
cmake -S . -B out/ps5 -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=ps5/cmake/ps5-toolchain.cmake -DPS5_PAYLOAD_SDK=<plume>/.deps/native/ps5-payload-sdk \
  -DREXSDK_DIR=<runtime> -DREXGLUE_HOST_TOOL=<host rexglue>
ninja -C out/ps5 dbz3
ps5/tools/ps5_package.sh out/ps5 <plume> PPSA93004
```

The app folder lands in `out/ps5/ps5-app/PPSA93004`.

The packaging also draws the PlayStation buttons (`ps5/tools/ps_buttons.py`, `ps_prompts.py`, `ps_controller.py`: Python 3 with Pillow, NumPy, OpenCV and Shapely) into `ps_buttons/` of the app, a texture pack the game uses when the hub's Button Prompts are on PlayStation. The DualSense picture comes from `ps5/art/local/buttons/dualsense.png` (your own art, not in the repo).

## On the PS5

Copy `PPSA93004` to `/data/homebrew/`, then your disc's `DBZ3` folder into `PPSA93004/Assets/`.
