#!/bin/sh

# This file is part of BASICfuck.
#
# Copyright (c) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# BASICfuck is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# BASICfuck is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# BASICfuck. If not, see <https://www.gnu.org/licenses/>.

# TODO Desired additional targets:
# apple2
# apple2enh
# atmos
# creativision - apparntly has keyboard.
# telestrat

# Error on unset variables.
set -u

################################################################################
# Configuration                                                                #
################################################################################

. ./config.sh || exit 1

# Loads configuration for the given build target.
# $1 - build target.
# Sets:
# - basicfuck_memory_size - the amount of memory, in bytes, to give for
#   BASICfuck memory.
# - binary_file_extension - the file extension to use for the compiled program.
# - emulator - the emulator command to use. Append the program file to this
#   command.
load_config_for_target() {
    if [ c64 = "$1" ]; then
        basicfuck_memory_size=$C64_CELL_MEMORY_SIZE
        binary_file_extension=$C64_BINARY_FILE_EXTENSION
        emulator=$C64_EMULATOR
    elif [ c128 = "$1" ]; then
        basicfuck_memory_size=$C128_CELL_MEMORY_SIZE
        binary_file_extension=$C128_BINARY_FILE_EXTENSION
        emulator=$C128_EMULATOR
    elif [ plus4 = "$1" ]; then
        basicfuck_memory_size=$PLUS4_CELL_MEMORY_SIZE
        binary_file_extension=$PLUS4_BINARY_FILE_EXTENSION
        emulator=$PLUS4_EMULATOR
    elif [ pet = "$1" ]; then
        basicfuck_memory_size=$PET_CELL_MEMORY_SIZE
        binary_file_extension=$PET_BINARY_FILE_EXTENSION
        emulator=$PET_EMULATOR
    elif [ cx16 = "$1" ]; then
        basicfuck_memory_size=$CX16_CELL_MEMORY_SIZE
        binary_file_extension=$CX16_BINARY_FILE_EXTENSION
        emulator=$CX16_EMULATOR
    elif [ atari = "$1" ]; then
        basicfuck_memory_size=$ATARI_CELL_MEMORY_SIZE
        binary_file_extension=$ATARI_BINARY_FILE_EXTENSION
        emulator=$ATARI_EMULATOR
    elif [ atarixl = "$1" ]; then
        basicfuck_memory_size=$ATARIXL_CELL_MEMORY_SIZE
        binary_file_extension=$ATARIXL_BINARY_FILE_EXTENSION
        emulator=$ATARIXL_EMULATOR
    else
        echo "ERROR: No build configuration for target '$1'" 1>&2
        exit 1
    fi
}

################################################################################
# Command Line Interface                                                       #
################################################################################

targets='c64 c128 pet plus4 cx16 atari atarixl'
repl_source=baf-repl.c

if [ 0 -eq $# ]; then
    echo "Usages:
  $0 SUBCOMMAND [OPTIONS...]

Build script for BASICfuck.

See 'config.sh' for configuration options.

Subcommands:
  targets
    Lists available build targets.

  build <target>
    Build for the specified target.
    Set the EXTRA_CFLAGS environment variable to add options to cl65.
    Set the CFLAGS environment variable to override the default options to cl65.

  run <target>
    Run the configured emulator for the specfied target.
"
    exit
fi

if [ targets = "$1" ]; then
    echo 'all (for build subcommand only)'
    for target in $targets; do
        echo "$target"
    done
    exit
fi

if [ build = "$1" ]; then
    if [ 2 -gt $# ]; then
        echo 'ERROR: build subcommand expects a target as an argument' 1>&2
        echo "Try '$0' for more information"                           1>&2
        exit 1
    fi

    if [ all = "$2" ]; then
        build_targets=$targets
    else
        build_targets=$2
    fi

    # Remaining arguments are to be passed to cl65.
    shift 2

    CC=cl65
    CFLAGS=${CFLAGS:-'-Osir -Cl -Wc -W,struct-param'}

    # Automatically format if astyle is installed.
    set -x
    if type astyle > /dev/null 2>&1; then
        astyle -n --style=attach "$repl_source" || exit 1
    fi
    set +x

    for target in $build_targets; do
        echo "INFO: Building for target '$target'..."

        load_config_for_target "$target"
        # shellcheck disable=SC2089 # We want \" treated literally.
        ALL_CFLAGS="$CFLAGS -t $target -D BASICFUCK_MEMORY_SIZE=${basicfuck_memory_size}U -D HISTORY_STACK_SIZE=${HISTORY_STACK_SIZE}U"
        out_directory=out/$target
        repl_out="$out_directory/${repl_source%.c}.${binary_file_extension}"

        set -x
        mkdir -p "$out_directory"
        # shellcheck disable=SC2086,SC2090 # We want word splitting.
        $CC $ALL_CFLAGS "$@" -o "$repl_out" "$repl_source" || exit 1
        set +x
    done

    exit
fi

if [ "run" = "$1" ]; then
    if [ 2 -gt $# ]; then
        echo 'ERROR: run subcommand expects a target as an argument' 1>&2
        echo "Try '$0' for more information"                         1>&2
        exit 1
    fi

    echo "INFO: Emulating for target '$2'..."

    load_config_for_target "$2"
    out_directory=out/$2
    repl_out="$out_directory/${repl_source%.c}.${binary_file_extension}"

    set -x
    $emulator "$repl_out" || exit 1
    set +x

    exit
fi

echo "ERROR: Unknown subcommand '$1'" 1>&2
echo "Try '$0' for more information"  1>&2
exit 1
