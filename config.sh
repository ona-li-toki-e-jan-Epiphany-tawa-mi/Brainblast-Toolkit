#!/bin/sh

# zlib license
#
# Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# This software is provided ‘as-is’, without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software. If you use this software
# in a product, an acknowledgment in the product documentation would be
# appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source
# distribution.



# Metadata.
export TOOLKIT_VERSION=0.1.0

# The size, in bytes, of the stack used to recall previous user inputs.
export HISTORY_STACK_SIZE=1028

# The number of bytes to allocate for BASICfuck cell memory. Make this 30,000
# cells maxiumum. If someone wants more they can always change it.
export C64_CELL_MEMORY_SIZE=30000
export C128_CELL_MEMORY_SIZE=28000
export PLUS4_CELL_MEMORY_SIZE=30000
export PET_CELL_MEMORY_SIZE=17500
export CX16_CELL_MEMORY_SIZE=25000
export ATARI_CELL_MEMORY_SIZE=25900
export ATARIXL_CELL_MEMORY_SIZE=28300

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
export C64_EMULATOR='x64 -silent'
export C128_EMULATOR='x128 -silent'
export PLUS4_EMULATOR='xplus4 -silent'
export PET_EMULATOR='xpet -silent'
export CX16_EMULATOR='x16emu -rom /usr/share/x16-rom/rom.bin -prg'
export ATARI_EMULATOR='atari800 > /dev/null -run'
export ATARIXL_EMULATOR='atari800 -xl > /dev/null -run'
