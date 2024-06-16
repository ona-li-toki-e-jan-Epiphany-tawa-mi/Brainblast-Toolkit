#!/bin/sh

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

# Metadata.
TOOLKIT_VERSION=0.1.0

# The size, in bytes, of the stack used to recall previous user inputs.
HISTORY_STACK_SIZE=1028

# The number of bytes to allocate for BASICfuck cell memory. Make this 30,000
# cells maxiumum. If someone wants more they can always change it.
C64_CELL_MEMORY_SIZE=30000
C128_CELL_MEMORY_SIZE=28000
PLUS4_CELL_MEMORY_SIZE=30000
PET_CELL_MEMORY_SIZE=17500
CX16_CELL_MEMORY_SIZE=25000
ATARI_CELL_MEMORY_SIZE=25900
ATARIXL_CELL_MEMORY_SIZE=28300

# Which file extension to use for generated binaries.
C64_BINARY_FILE_EXTENSION=prg
C128_BINARY_FILE_EXTENSION=prg
PLUS4_BINARY_FILE_EXTENSION=prg
PET_BINARY_FILE_EXTENSION=prg
CX16_BINARY_FILE_EXTENSION=prg
ATARI_BINARY_FILE_EXTENSION=com
ATARIXL_BINARY_FILE_EXTENSION=com

# Emulator commands. The binary to run will be appended to the end of the
# command.
C64_EMULATOR='x64 -silent'
C128_EMULATOR='x128 -silent'
PLUS4_EMULATOR='xplus4 -silent'
PET_EMULATOR='xpet -silent'
CX16_EMULATOR='x16emu -rom /usr/share/x16-rom/rom.bin -prg'
ATARI_EMULATOR='atari800 > /dev/null -run'
ATARIXL_EMULATOR='atari800 -xl > /dev/null -run'
