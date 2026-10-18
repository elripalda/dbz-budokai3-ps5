#!/usr/bin/env bash
# [ps5 port] Link the PS5 build of DBZ Budokai 3 and package it as a PS5
# homebrew app folder (eboot.bin, sce_module/libc.prx, sce_sys/param.json).
#
#   ps5/tools/ps5_package.sh <cmake build dir> <ps5-plume-triangle checkout> [title id]
#
# The link and packaging reuse the PS5 tooling of Nazky's ps5-plume-triangle
# (mihawk-99's RADV link recipe, app CRT, libc.prx builder and FSELF signer):
# build that project once first (its build/ holds app_crt.o, app_cpp_runtime.o,
# the libSceAgc stubs and ps5-native-tool). No game files are packaged: the
# disc's DBZ3 folder goes into Assets on the PS5.
set -euo pipefail

build=$(realpath "${1:?cmake build dir}")
tri=$(realpath "${2:?ps5-plume-triangle checkout}")
title_id=${3:-PPSA93004}
here=$(cd "$(dirname "$0")" && pwd)
port=$(cd -- "$here/.." && pwd)        # ps5/
root=$(cd -- "$port/.." && pwd)        # the project

sdk_root=${PS5_PAYLOAD_SDK:-$tri/.deps/native/ps5-payload-sdk}
radv_archive=${RADV_ARCHIVE:-$tri/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a}
tool=$tri/build/host/ps5-native-tool
for f in "$sdk_root/bin/prospero-lld" "$radv_archive" "$tool" "$tri/build/obj/app_crt.o" \
         "$tri/build/obj/app_cpp_runtime.o" "$tri/build/stubs/libSceAgc.so" "$tri/runtime/libc.prx"; do
    [[ -e $f ]] || { echo "missing $f (build ps5-plume-triangle first)" >&2; exit 2; }
done

# Staging root for radv_link_recipe: it reads tooling/ and vendor/ and writes build/.
stage=$build/ps5-link
mkdir -p "$stage/build"
ln -sfn "$tri/tooling" "$stage/tooling"
ln -sfn "$tri/vendor" "$stage/vendor"

# The objects and static libraries CMake links into dbz3, in its order.
link_cmd=$(ninja -C "$build" -t commands dbz3 | tail -1)
mapfile -t inputs < <(cd "$build" && for w in $link_cmd; do
    case $w in
        *.o|*.a) [[ $w = /* ]] && echo "$w" || echo "$build/$w" ;;
    esac
done)
echo "==> [link] ${#inputs[@]} objects and libraries from CMake"

export PS5_CLANG=${PS5_CLANG:-$(command -v clang-19 || command -v clang)}
source "$tri/tools/radv-link.sh"
radv_link_recipe "$stage" "$sdk_root" "$radv_archive"
# (the recipe's layout, with a fixed gap for the process parameters)
radv_linker_script=(-T "$here/ps5-pie-budokai3.ld")

echo "==> [link] DBZ Budokai 3 + RADV into a PS5 PIE ELF"
"$sdk_root/bin/prospero-lld" "${radv_linker_script[@]}" --eh-frame-hdr "${radv_link_flags[@]}" \
    --version-script "$tri/tooling/native/app-symbols.map" --exclude-libs=ALL \
    -e _start -o "$stage/build/llvm-pie.elf" \
    "$tri/build/obj/app_crt.o" "$tri/build/obj/app_cpp_runtime.o" \
    --start-group "${inputs[@]}" --end-group \
    "$tri/build/stubs/libSceAgc.so" "$tri/build/stubs/libSceAgcDriver.so" \
    "${radv_link_inputs[@]}" \
    --as-needed "$sdk_root"/target/lib/*.so

# A symbol the console doesn't have traps the first time it's called. So every
# import must be one a working build already had (ps5-known-imports.txt: the
# SvR, NBA Jam, Skate 3 and WWE ports'); a new one is listed and the build
# stops, unless PS5_ALLOW_NEW_IMPORTS=1 (a test build, to try them).
new_imports=$("$sdk_root/bin/llvm-nm" -u "$stage/build/llvm-pie.elf" | awk '{print $2}' | sort -u |
              comm -23 - "$here/ps5-known-imports.txt")
if [ -n "$new_imports" ]; then
  echo "new imports, not known to work on the PS5 (it may trap on them):" $new_imports >&2
  [[ ${PS5_ALLOW_NEW_IMPORTS:-} == 1 ]] || exit 1
fi

# The debug info stays out of the app: kept beside it, for reading crash
# addresses (llvm-pie.debug.elf).
cp "$stage/build/llvm-pie.elf" "$stage/build/llvm-pie.debug.elf"
"$sdk_root/bin/llvm-strip" --strip-debug "$stage/build/llvm-pie.elf"

echo "==> [package] eboot.bin for $title_id"
"$tool" link --in "$stage/build/llvm-pie.elf" --out "$stage/build/eboot.elf" \
    --stub-dir "$sdk_root/target/lib" --stub "$tri/build/stubs/libSceAgc.so" \
    --stub "$tri/build/stubs/libSceAgcDriver.so" --module-sdk 0x02000009 \
    --companion-sdk 0x08050001 --file-name eboot.elf
# The conversion must leave the game's data as linked.
python3 "$here/check_eboot_data.py" "$stage/build/llvm-pie.elf" "$stage/build/eboot.elf"
app=$build/ps5-app/$title_id
rm -rf -- "$app"
mkdir -p "$app/sce_sys" "$app/sce_module"
"$tool" self --sign --in "$stage/build/eboot.elf" --out "$app/eboot.bin" --magic 0x1D3D154F
cp "$tri/runtime/libc.prx" "$app/sce_module/libc.prx"
# Where the game's files go (the disc's DBZ3 folder), and the mods folder.
mkdir -p "$app/Assets" "$app/mods"
cp "$here/assets-readme.txt" "$app/Assets/README.txt"
cp "$here/how-to-install.txt" "$app/HOW TO INSTALL.txt"
# The in-game screens' fonts (Inter and Oswald, SIL Open Font License).
mkdir -p "$app/fonts"
cp "$port"/fonts/*.ttf "$port"/fonts/OFL-*.txt "$app/fonts/"
# The PlayStation buttons (a texture pack, ps_buttons/pack): drawn by the tools
# here, the DualSense picture from art/local/buttons/dualsense.png. The BACK
# button's texture is made on the console from the game's (ps_button_pack.cpp).
mkdir -p "$app/ps_buttons/pack"
python3 "$here/ps_buttons.py" "$app/ps_buttons/pack/AEA34DDB47477372_512x64_RGBA8.png"
python3 "$here/ps_prompts.py" "$app/ps_buttons/pack" "$app/ps_buttons/touch_pad.png"
# The controller screen's pointer lines are drawn again for it (ps_pointers.py).
if [[ -f $port/art/local/buttons/dualsense.png ]]; then
    python3 "$here/ps_controller.py" "$port/art/local/buttons/dualsense.png" \
        "$app/ps_buttons/pack/355BB8CF9E381EA7_512x256_RGBA8.png"
    python3 "$here/ps_pointers.py" "$app/ps_buttons/pack"
fi
# A shader cache to start from (art/local/shader_cache: the files from a PS5
# that has played a lot, its user_data/dbz3/cache/shaders/shareable). The game
# copies them in on a first start, so effects don't stutter the first time.
if [[ -d $port/art/local/shader_cache ]]; then
    mkdir -p "$app/shader_cache"
    cp "$port"/art/local/shader_cache/* "$app/shader_cache/"
fi
# SDL's controller mappings (the PC port ships it next to the exe).
if [[ -f $root/gamecontrollerdb.txt ]]; then cp "$root/gamecontrollerdb.txt" "$app/"; fi
cat > "$app/sce_sys/param.json" <<EOF
{
  "ageLevel": { "default": 0 },
  "applicationCategoryType": 0,
  "applicationDrmType": "free",
  "attribute": 0,
  "attribute2": 0,
  "attribute3": 0,
  "conceptId": "${title_id:4}",
  "contentBadgeType": 1,
  "contentId": "UP9000-${title_id}_00-DBZBUDOKAI3PS500",
  "contentVersion": "01.000.000",
  "downloadDataSize": 256,
  "gameIntent": { "permittedIntents": [ { "intentType": "launchActivity" } ] },
  "localizedParameters": {
    "defaultLanguage": "en-US",
    "en-US": { "titleName": "Dragon Ball Z Budokai 3" }
  },
  "masterVersion": "01.00",
  "pubtools": { "creationDate": "2026-10-17 00:00:00", "loudnessSnd0": "-28.00", "toolVersion": "2.00" },
  "requiredSystemSoftwareVersion": "0x0000000000000000",
  "sdkVersion": "0x0000000000000000",
  "titleId": "${title_id}",
  "versionFileUri": ""
}
EOF
# The hub's art and menu music (art/local/hub: logo.png, hero.jpg, buttons/,
# music/ - your own, never in the repo) -> <app>/hub
if [[ -d $port/art/local/hub ]]; then
    cp -r "$port/art/local/hub" "$app/hub"
fi
# The hub's button pictures (hub/buttons), from the PS5 button icons' SVGs
# (art/local/hub-buttons: the white "full solid" set, your own).
if [[ -d $port/art/local/hub-buttons ]]; then
    python3 "$here/hub_buttons.py" "$port/art/local/hub-buttons" "$app/hub/buttons"
fi
# Home screen art (art/local/, your own; art/ the placeholders): icon0.png
# 512x512, pic0.dds / pic1.dds the 3840x2160 BC7 background and launch splash.
for asset in icon0.png pic0.dds pic1.dds; do
    if [[ -f $port/art/local/$asset ]]; then
        cp "$port/art/local/$asset" "$app/sce_sys/$asset"
    elif [[ -f $port/art/$asset ]]; then
        cp "$port/art/$asset" "$app/sce_sys/$asset"
    fi
done
"$tool" self --inspect --file "$app/eboot.bin" | tail -3
echo "==> $app ($(stat -c %s "$app/eboot.bin") bytes)"
