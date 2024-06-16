# This file is part of Brainblast-Toolkit.
#
# Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# Brainblast-Toolkit is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# Brainblast-Toolkit is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
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
# - ./build.sh OR ./build.sh build
#     Builds binaries for given TARGET.
# - ./build.sh assembly
#     Builds assembly for given TARGET for analysis.
# - ./build.sh run
#     Runs the REPL binary for given TARGET in an emulator.
# - ./build.sh clean
#     Deletes built files.

# Error on unset variables.
set -u



TARGETS=${TARGETS:-c64}

# Imports build configuration.
. config.sh || exit 1



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
    CFLAGS=${CFLAGS:-'-Osir --static-locals -Wc -W,error'}
    ALL_CFLAGS="$CFLAGS --target $TARGET --include-dir $CC65_DIRECTORY/include --cfg-path $CC65_DIRECTORY/cfg --lib-path $CC65_DIRECTORY/lib -D BASICFUCK_MEMORY_SIZE=${BASICFUCK_MEMORY_SIZE}U -D HISTORY_STACK_SIZE=${HISTORY_STACK_SIZE}U -D TOOLKIT_VERSION=\"$TOOLKIT_VERSION\""

    SOURCE_DIRECTORY=src
    SOURCE=$SOURCE_DIRECTORY/baf-repl.c

    OUT_DIRECTORY=$TARGET
    REPL_OUT=$OUT_DIRECTORY/baf-repl.$BINARY_FILE_EXTENSION


    if [ 0 -eq $# ] || [ build = "$1" ]; then
        set -x
        mkdir -p "$OUT_DIRECTORY"                || exit 1
        $CC $ALL_CFLAGS -o "$REPL_OUT" "$SOURCE" || exit 1

    elif [ assembly = "$1" ]; then
        assembly=${SOURCE%.c}.s; assembly=${assembly#${SOURCE_DIRECTORY}/}
        set -x
        $CC $ALL_CFLAGS -S -o "$assembly" "$SOURCE" || exit 1

    elif [ run = "$1" ]; then
        set -x
        $EMULATOR "$REPL_OUT" || exit 1

    elif [ clean = "$1" ]; then
        set -x
        rm -r "$OUT_DIRECTORY" "$SOURCE_DIRECTORY"/*.o *.s

    else
        echo "$0: Error: Unknown build command '$1'" 1>&2
        exit 1
    fi


    set +x
done



exit 0
