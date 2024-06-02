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

# See 'config.mk' for extra build configuration options.
#
# Available commands:
# - make/make all
#     Builds binaries for given TARGET.
# - make assembly
#     Builds assembly for given TARGET for analysis.
# - make runREPL
#     Builds REPL binary for given TARGET and runs it in an emulator.
# - make clean
#     Deletes built files.
#
# Important make parameters (VARIABLE=VALUE on command line):
# - TARGET
#     Target platform to build for. Availible targets:
#     - c64 (Commodore 64, emulator: VICE)
#     - c128 (Commodore 128, emulator: VICE)
#     - pet (Commodore PET, emulator: VICE)
#     - plus4 (Commodore Plus/4, emulator: VICE)
#     - cx16 (Commander X16, emulator: x16-emulator)
#     - atari (All 8-bit Atari computers, emulator: atari800)
#     - atarixl (8-bit Atari computers, XL or newer, except for the 600XL,
#       emulator: atari800)



TARGET ?= c64

# Imports build configuration.
include config.mk
ifeq (c64,$(TARGET))
BASICFUCK_MEMORY_SIZE := $(C64_CELL_MEMORY_SIZE)
BINARY_FILE_EXTENSION := $(C64_BINARY_FILE_EXTENSION)
EMULATOR              := $(C64_EMULATOR)
else ifeq (c128,$(TARGET))
BASICFUCK_MEMORY_SIZE := $(C128_CELL_MEMORY_SIZE)
BINARY_FILE_EXTENSION := $(C128_BINARY_FILE_EXTENSION)
EMULATOR              := $(C128_EMULATOR)
else ifeq (plus4,$(TARGET))
BASICFUCK_MEMORY_SIZE := $(PLUS4_CELL_MEMORY_SIZE)
BINARY_FILE_EXTENSION := $(PLUS4_BINARY_FILE_EXTENSION)
EMULATOR              := $(PLUS4_EMULATOR)
else ifeq (pet,$(TARGET))
BASICFUCK_MEMORY_SIZE := $(PET_CELL_MEMORY_SIZE)
BINARY_FILE_EXTENSION := $(PET_BINARY_FILE_EXTENSION)
EMULATOR              := $(PET_EMULATOR)
else ifeq (cx16,$(TARGET))
BASICFUCK_MEMORY_SIZE := $(CX16_CELL_MEMORY_SIZE)
BINARY_FILE_EXTENSION := $(CX16_BINARY_FILE_EXTENSION)
EMULATOR              := $(CX16_EMULATOR)
else ifeq (atari,$(TARGET))
BASICFUCK_MEMORY_SIZE := $(ATARI_CELL_MEMORY_SIZE)
BINARY_FILE_EXTENSION := $(ATARI_BINARY_FILE_EXTENSION)
EMULATOR              := $(ATARI_EMULATOR)
else ifeq (atarixl,$(TARGET))
BASICFUCK_MEMORY_SIZE := $(ATARIXL_CELL_MEMORY_SIZE)
BINARY_FILE_EXTENSION := $(ATARIXL_BINARY_FILE_EXTENSION)
EMULATOR              := $(ATARIXL_EMULATOR)
else
$(error no build configuration for target $(TARGET))
endif



CC65DIR     := /usr/share/cc65
CC          := cl65
CFLAGS      ?= -Osir --static-locals -Wc -W,error
ALL_CFLAGS  := $(CFLAGS) --target $(TARGET) --include-dir $(CC65DIR)/include --asm-include-dir $(CC65DIR)/asminc -D BASICFUCK_MEMORY_SIZE=$(BASICFUCK_MEMORY_SIZE)U -D HISTORY_STACK_SIZE=$(HISTORY_STACK_SIZE)U -D TOOLKIT_VERSION=\"$(TOOLKIT_VERSION)\"
LD          := cl65
LDFLAGS     := --target $(TARGET) --cfg-path $(CC65DIR)/cfg --lib-path $(CC65DIR)/lib

srcdir       := src
REPL_SOURCES := $(addprefix $(srcdir)/,repl.c bytecode_compiler.c opcodes.c)
vpath %.c $(dir $(REPL_SOURCES))
REPL_OBJECTS := $(notdir $(REPL_SOURCES:.c=.o))

outdir      ?= out
REPL_BINARY := $(outdir)/$(TARGET)-repl.$(BINARY_FILE_EXTENSION)

.PHONY: all
all: $(REPL_BINARY)

$(REPL_BINARY): $(REPL_OBJECTS)
	mkdir -p $(outdir)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(ALL_CFLAGS) --create-dep $(@:.o=.d) -c -o $@ $<
-include $(REPL_OBJECTS:.o=.d)



.PHONY: assembly
assembly: $(REPL_OBJECTS:.o=.s)

%.s: %.c
	$(CC) $(ALL_CFLAGS) -S -o $@ $<



.PHONY: runREPL
runREPL: $(REPL_BINARY)
	$(EMULATOR) $<



.PHONY: clean
clean:
	-rm -r $(outdir) *.o *.s *.d
