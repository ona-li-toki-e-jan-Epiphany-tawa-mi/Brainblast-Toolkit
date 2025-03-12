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

# The size, in bytes, of the stack used to recall previous user inputs.
export HISTORY_STACK_SIZE=1028

# The number of bytes to allocate for BASICfuck cell memory. Make this 30,000
# cells maxiumum. If someone wants more they can always change it.
export C64_CELL_MEMORY_SIZE=30000
export C128_CELL_MEMORY_SIZE=27750
export PLUS4_CELL_MEMORY_SIZE=30000
export PET_CELL_MEMORY_SIZE=17000
export CX16_CELL_MEMORY_SIZE=25250
export ATARI_CELL_MEMORY_SIZE=25500
export ATARIXL_CELL_MEMORY_SIZE=27000

# Which file extension to use for generated binaries.
export C64_BINARY_FILE_EXTENSION=prg
export C128_BINARY_FILE_EXTENSION=prg
export PLUS4_BINARY_FILE_EXTENSION=prg
export PET_BINARY_FILE_EXTENSION=prg
export CX16_BINARY_FILE_EXTENSION=prg
export ATARI_BINARY_FILE_EXTENSION=com
export ATARIXL_BINARY_FILE_EXTENSION=com

# Emulator commands. The binary to run will be appended to the end of the
# command.
export C64_EMULATOR=x64
export C128_EMULATOR=x128
export PLUS4_EMULATOR=xplus4
export PET_EMULATOR=xpet
export CX16_EMULATOR='x16emu -rom /usr/share/x16-rom/rom.bin -prg'
export ATARI_EMULATOR='atari800 -run'
export ATARIXL_EMULATOR='atari800 -xl -run'
