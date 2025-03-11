#!/bin/sh

# This file is part of Brainblast-Toolkit.
#
# Copyright (c) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# Brainblast-Toolkit is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# Brainblast-Toolkit is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# Brainblast-Toolkit. If not, see <https://www.gnu.org/licenses/>.

# TODO Desired additional targets:
# apple2
# apple2enh
# atmos
# creativision - apparntly has keyboard.
# telestrat

# See 'config.sh' for extra build configuration options.
#
# Enviroment variable parameters:
# - TARGETS
#     Target platforms to build for. Space separated list. Defaults to c64.
#     Availible targets:
#     - c64 (Commodore 64, emulator: VICE)
#     - c128 (Commodore 128, emulator: VICE)
#     - pet (Commodore PET, emulator: VICE)
#     - plus4 (Commodore Plus/4, emulator: VICE)
#     - cx16 (Commander X16, emulator: x16-emulator)
#     - atari (All 8-bit Atari computers, emulator: atari800)
#     - atarixl (8-bit Atari computers, XL or newer, except for the 600XL,
#       emulator: atari800)
#
# Available commands:
# - ./build.sh targets
#     Lists availabe build targets
# - ./build.sh OR ./build.sh build
#     Builds binaries for given TARGETS.
# - ./build.sh assembly
#     Builds assembly for given TARGETS for analysis.
# - ./build.sh run
#     Runs the REPL binary for given TARGETS in an emulator.
# - ./build.sh clean
#     Deletes built files from given TARGETS.

# Error on unset variables.
set -u



if [ 0 -ne $# ] && [ targets = "$1" ]; then
    echo c64 c128 pet plus4 cx16 atari atarixl
    exit 0
fi



# Imports build configuration.
. ./config.sh || exit 1

TARGETS=${TARGETS:-c64}

for TARGET in $TARGETS; do
    if [ c64 = "$TARGET" ]; then
        BASICFUCK_MEMORY_SIZE=$C64_CELL_MEMORY_SIZE
        BINARY_FILE_EXTENSION=$C64_BINARY_FILE_EXTENSION
        EMULATOR=$C64_EMULATOR
    elif [ c128 = "$TARGET" ]; then
        BASICFUCK_MEMORY_SIZE=$C128_CELL_MEMORY_SIZE
        BINARY_FILE_EXTENSION=$C128_BINARY_FILE_EXTENSION
        EMULATOR=$C128_EMULATOR
    elif [ plus4 = "$TARGET" ]; then
        BASICFUCK_MEMORY_SIZE=$PLUS4_CELL_MEMORY_SIZE
        BINARY_FILE_EXTENSION=$PLUS4_BINARY_FILE_EXTENSION
        EMULATOR=$PLUS4_EMULATOR
    elif [ pet = "$TARGET" ]; then
        BASICFUCK_MEMORY_SIZE=$PET_CELL_MEMORY_SIZE
        BINARY_FILE_EXTENSION=$PET_BINARY_FILE_EXTENSION
        EMULATOR=$PET_EMULATOR
    elif [ cx16 = "$TARGET" ]; then
        BASICFUCK_MEMORY_SIZE=$CX16_CELL_MEMORY_SIZE
        BINARY_FILE_EXTENSION=$CX16_BINARY_FILE_EXTENSION
        EMULATOR=$CX16_EMULATOR
    elif [ atari = "$TARGET" ]; then
        BASICFUCK_MEMORY_SIZE=$ATARI_CELL_MEMORY_SIZE
        BINARY_FILE_EXTENSION=$ATARI_BINARY_FILE_EXTENSION
        EMULATOR=$ATARI_EMULATOR
    elif [ atarixl = "$TARGET" ]; then
        BASICFUCK_MEMORY_SIZE=$ATARIXL_CELL_MEMORY_SIZE
        BINARY_FILE_EXTENSION=$ATARIXL_BINARY_FILE_EXTENSION
        EMULATOR=$ATARIXL_EMULATOR
    else
        echo "$0: Error: No build configuration for target '$TARGET'" 1>&2
        exit 1
    fi

    CC65_DIRECTORY=${CC65DIR:-/usr/share/cc65}
    CC=cl65
    CFLAGS=${CFLAGS:-'-Osir -Cl -Wc -W,error,-W,struct-param'}
    # shellcheck disable=SC2089 # We want \" treated literally.
    ALL_CFLAGS="$CFLAGS -t $TARGET -I $CC65_DIRECTORY/include --asm-include-dir $CC65_DIRECTORY/asminc -L $CC65_DIRECTORY/lib --cfg-path $CC65_DIRECTORY/cfg  -D BASICFUCK_MEMORY_SIZE=${BASICFUCK_MEMORY_SIZE}U -D HISTORY_STACK_SIZE=${HISTORY_STACK_SIZE}U -D TOOLKIT_VERSION=\"$TOOLKIT_VERSION\""

    SOURCE=baf-repl.c

    OUT_DIRECTORY=$TARGET
    REPL_OUT=$OUT_DIRECTORY/baf-repl.$BINARY_FILE_EXTENSION


    if [ 0 -eq $# ] || [ build = "$1" ]; then
        object=${SOURCE%.c}.o
        set -x
        # shellcheck disable=SC2086,SC2090 # We want word splitting.
        $CC $ALL_CFLAGS -c -o "$object" "$SOURCE"  || exit 1
        mkdir -p "$OUT_DIRECTORY"                  || exit 1
        # shellcheck disable=SC2086,SC2090 # We want word splitting.
        $CC $ALL_CFLAGS -o "$REPL_OUT" "$object"   || exit 1

    elif [ assembly = "$1" ]; then
        assembly=${SOURCE%.c}.s;
        set -x
        # shellcheck disable=SC2086,SC2090 # We want word splitting.
        $CC $ALL_CFLAGS -g -S -o "$assembly" "$SOURCE" || exit 1

    elif [ run = "$1" ]; then
        set -x
        $EMULATOR "$REPL_OUT" || exit 1

    elif [ clean = "$1" ]; then
        set -x
        rm -rf "$OUT_DIRECTORY" ./*.o ./*.s

    else
        echo "$0: Error: Unknown build command '$1'" 1>&2
        exit 1
    fi


    set +x
done
