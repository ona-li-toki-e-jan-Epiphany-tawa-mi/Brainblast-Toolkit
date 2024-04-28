################################################################################
# This file is part of Brainblast-Toolkit.                                     #
#                                                                              #
# Brainblast-Toolkit is free software: you can redistribute it and/or modify   #
# it under the terms of the GNU General Public License as published by the     #
# Free Software Foundation, either version 3 of the License, or (at your       #
# option) any later version.                                                   #
#                                                                              #
# Brainblast-Toolkit is distributed in the hope that it will be useful, but    #
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY   #
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for  #
# more details.                                                                #
#                                                                              #
# You should have received a copy of the GNU General Public License along with #
# Brainblast-Toolkit. If not, see <https://www.gnu.org/licenses/>.             #
################################################################################x

# Impossible targets:
# gamate      - no keyboard.
# geos-apple  - this is not an OS application.
# geos-cbm    - this is not an OS application.
# lynx        - no keyboard.
# nes         - no keyboard.
# pce         - no keyboard.
# supervision - no keyboard.

# TODO Desired additional targets:
# apple2
# apple2enh
# atari (atarixl if not enough memory)
# atmos
# creativision - apparntly has keyboard.
# cx16
# telestrat

# TODO Currently working on:
# c16    - memory issue.
# vic20  - code size issue.
# cbm510 - doesn't display anything.
# cbm610 - doesn't always load .prg (why?,) blinking cursor appears in wrong place.

# Supported targets:
# c64
# c128
# pet
# plus4

# Target platform to compile for.
TARGET ?= c64

# Size of stack used to recall previous inputs.
HISTORY_STACK_SIZE ?= 2048U

# TODO fine tune how many cells each version gets.
ifeq (${TARGET}, c16)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else ifeq (${TARGET}, plus4)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else ifeq (${TARGET}, c64)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else ifeq (${TARGET}, c128)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else ifeq (${TARGET}, vic20)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else ifeq (${TARGET}, pet)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else ifeq (${TARGET}, cbm510)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else ifeq (${TARGET}, cbm610)
	BASICFUCK_MEMORY_SIZE ?= 15000U
else
	BASICFUCK_MEMORY_SIZE ?= 15000U
endif



CC65_DIRECTORY := /usr/share/cc65
CL65_ARGUMENTS := --include-dir ${CC65_DIRECTORY}/include --asm-include-dir ${CC65_DIRECTORY}/asminc --lib-path ${CC65_DIRECTORY}/lib --cfg-path ${CC65_DIRECTORY}/cfg --target ${TARGET} -D BASICFUCK_MEMORY_SIZE=${BASICFUCK_MEMORY_SIZE} -D HISTORY_STACK_SIZE=${HISTORY_STACK_SIZE} -Osir --static-locals

SOURCES := bytecode_compiler.c opcodes.c text_buffer.c screen.c

REPL     := repl.c
PROGRAMS := ${REPL}


.PHONY: all
all: ${PROGRAMS:.c=.prg}

%.prg: %.o ${SOURCES:.c=.o}
	cl65 ${CL65_ARGUMENTS} -o $@ $^

ifneq (${MAKECMDGOALS},clean)
-include ${PROGRAMS:.c=.d} ${SOURCES:.c=.d}
endif

%.o: %.c
	cl65 ${CL65_ARGUMENTS} --create-dep ${<:.c=.d} -c -o $@ $<



# Generates assembly files of C code for analysis.
.PHONY: assembly
assembly: ${PROGRAMS:.c=.o.s} ${SOURCES:.c=.o.s}

%.o.s: %.c
	cl65 ${CL65_ARGUMENTS} -S -o $@ $<



# Runs the REPL inside an emulator for the given target.
.PHONY: runREPL
runREPL: ${REPL:.c=.prg}
ifeq (${TARGET}, c16)
	xplus4 -model c16 -silent $< # VICE.
else ifeq (${TARGET}, plus4)
	xplus4 -silent $<            # VICE.
else ifeq (${TARGET}, c64)
	x64 -silent $<               # VICE.
else ifeq (${TARGET}, c128)
	x128 -silent $<              # VICE.
else ifeq (${TARGET}, vic20)
	xvic -silent $<              # VICE.
else ifeq (${TARGET}, pet)
	xpet -silent $<              # VICE.
else ifeq (${TARGET}, cbm510)
	xcbm5x0 -silent $<           # VICE.
else ifeq (${TARGET}, cbm610)
	xcbm2 -silent $<             # VICE.
else
	${error no emulator configured for REPL of build target ${TARGET}}
endif



.PHONY: clean
clean:
	-rm *.prg *.o *.o.s *.d
