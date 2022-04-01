#!/usr/bin/env bash
 
# This file is part of The RetroPie Project
#
# The RetroPie Project is the legal property of its developers, whose names are
# too numerous to list here. Please refer to the COPYRIGHT.md file distributed with this source.
#
# See the LICENSE.md file at the top-level directory of this distribution and
# at https://raw.githubusercontent.com/RetroPie/RetroPie-Setup/master/LICENSE.md
#

rp_module_id="supermodel3"
rp_module_desc="Super Model 3 Emulator"
rp_module_help="ROM Addition: Copy your roms to $romdir/arcade"
rp_module_licence="GPL3 https://raw.githubusercontent.com/DirtBagXon/model3emu-code-sinden/main/Docs/LICENSE.txt"
rp_module_repo="git https://github.com/DirtBagXon/model3emu-code-sinden.git arm"
rp_module_section="exp"
rp_module_flags="sdl2"

function depends_supermodel3() {
    getDepends xinit libsdl2-dev libsdl2-net-dev libsdl2-net-2.0-0 x11-xserver-utils xserver-xorg
    aptRemove xserver-xorg-legacy
}

function sources_supermodel3() {
    gitPullOrClone
}

function build_supermodel3() {
    cp Makefiles/Makefile.UNIX Makefile
    make clean
    make
    cp Docs/LICENSE.txt LICENSE
    cp bin/supermodel supermodel3
    md_ret_require="supermodel3"
}

function install_supermodel3() {
    md_ret_files=(
        'supermodel3'
        'Config'
        'LICENSE'
    )
}

function configure_supermodel3() {

    mkRomDir "arcade"

    local allemu="/opt/retropie/configs/all/emulators.cfg"

    addEmulator 0 "$md_id" "arcade" "XINIT:$md_inst/$md_id -borders=2 %ROM%"
    addSystem "arcade"

    [[ "$md_mode" == "remove" ]] && return

    mkUserDir "$md_inst/Saves"
    mkUserDir "$md_inst/NVRAM"
    mkUserDir "$md_conf_root/$md_id"

    ln -snf "$md_inst/NVRAM" "$home/NVRAM"
    ln -snf "$md_inst/Saves" "$home/Saves"
    ln -snf "$md_conf_root/$md_id" "$home/Config"
    ln -snf "$md_conf_root/$md_id" "$md_inst/LocalConfig"

    copyDefaultConfig "$md_inst/Config/Supermodel.ini" "$md_conf_root/$md_id/Supermodel.ini"
    copyDefaultConfig "$md_inst/Config/Games.xml" "$md_conf_root/$md_id/Games.xml"

    local rom
    for rom in lostwsga lamachin oceanhun swtrilgy; do
          if ! grep -q "arcade_$rom" "$allemu"; then
             addLineToFile "arcade_$rom = \"$md_id\"" $allemu
          fi
    done

    rm -rf "$md_inst/Config"
    chown -R $user:$user "$md_inst"
    chown -R $user:$user "$md_conf_root/$md_id"
}
