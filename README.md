# Brainblast-Toolkit

A brainfuck/BASICfuck REPL for 6502 machines.

This project is a rewrite of the original version I wrote in BASIC for the
Commodore 64, which you can find in the `BASIC` directory.

This rewrite includes the following benefits:

- An extra instruction, `%` (execute.)
- WAYYYYY FASTER!
- Bytecode viewer (mainly just for debugging this, but it looks cool.)
- WAYYYYY FASTER! (x2)
- Support for a few 6502 machines, not just the Commodore 64 (see `build.sh` for supported systems.)

## BASICfuck

BASICfuck is a small extension of mine to the brainfuck language that is
reminiscent of BASIC, and is the language this REPL uses. You can find more
information about brainfuck at the following link:

https://wikipedia.org/wiki/Brainfuck

In addition to the normal cell memory of brainfuck, BASICfuck has a pointer into
the computer's raw memory that you can control.

#### Instruction set

- `+` - Increments the current cell.
- `-` - Decrements the current cell.
- `>` - Moves to the next cell.
- `<` - Moves to the previous cell.
- `.` - Displays the value in current cell as a character.
- `,` - Stores the value of a key from the keyboard into the current cell.
- `[` - Jumps to corresponding ']' if value of the current cell cell is 0.
- `]` - Jumps to corresponding '[' if value of the current cell cell is not 0.
- `)` - Moves to the next location in computer memory.
- `(` - Moves to the previous location in computer memory.
- `@` - Reads a value from computer memory into the current cell.
- `*` - Writes the value from the current cell into computer memory.
- `%` - Executes the current location in computer memory as a subroutine. The values of the current and next two cells will be used for the A, X, and Y registers repsectively. Resulting register values will be stored back into the same cells.

## REPL controls

Pressing STOP cancels the current input and starts a new line, similar to C-c.

Pressing HOME moves to the start of the current input.

Pressing CLR clears the screen and the current input.

Pressing F1/F2 cycles backwards/forwards through the input history.

Pressing STOP during while a program is running will abort it.

*These functions may be mapped to different keys depending on the system. The
help menu will display the correct keys if this is the case.*

#### Commands

To execute these commands, put them at the start of the input line and press
ENTER/RETURN:

- `!` - exits the REPL.
- `?` - displays the help menu.
- `#` - outputs hexdump of the bytecode of the previous BASICfuck program. Holding SPACE will slow down the printing.

## How to build

You will need make and the cc65 toolchain (https://cc65.github.io). There is a
`flake.nix` you can use with `nix develop path:.` to get them.

There are configuration options you can change in `config.sh`, mainly for memory
sizes.

To build for paticular systems, you will need to specify the TARGETS variable on
the command line to make. By default it will build for the Commodore 64. Check
`build.sh` for available targets. I.e.:

```
TARGETS=c64 ./build.sh
```

Resulting binaries can be found in the directory with the name of the build
target.

Passing multiple targets in a space-separated list with TARGETS will cause the
build commands to be run for each target one-by-one.

There are also emulator commands set up to be invoked with `build.sh` with
`run`. The emulator needed will depend on the target system. Check `build.sh`
for the required emulators (if you used `nix develop path:.`, they are already
included.) You may need to specify TARGETS as well, like before:

```
TARGETS=c64 ./build.sh run
```

You can also grab precompiled binaries off of my Hydra instance at one of the
following links:
- I2P: http://oytjumugnwsf4g72vemtamo72vfvgmp4lfsf6wmggcvba3qmcsta.b32.i2p/hydra/jobset/brainblast-toolkit/master/latest-eval
- Tor: http://4blcq4arxhbkc77tfrtmy4pptf55gjbhlj32rbfyskl672v2plsmjcyd.onion/hydra/jobset/brainblast-toolkit/master/latest-eval

## Example programs

#### Cycling screen border colors

On the Commodore 64, the color of the border of the screen is stored at $D020. This code uses that, so it might not work on other machines.

```brainfuck
-[->++++++++++[-)))))))))))))))))))))]<]--[--((]+++[-(((((](    Moves the memory pointer to $D020 the location of the border's color
+[>*+<]                                                         Rapidly switches border color
```

#### Cat program / screen editor

```brainfuck
+[,.]
```

#### Reverse cat

```brainfuck
+[,.[->+>+<<]>>[-<<+>>]<-------------]<<[.[-]<]
```

Type what you want to be reversed, and then press enter. Your input will be
displayed as you type it. You'll have to type a little slowly.

#### "Random" maze thing

Note: this requires a machine that uses PETSCII.

cc65 has programs on PETSCII machines use the shifted character set by default
to be more standard. You will have to switch it to the non-shifted set for this
program. On the Commodore 64, and probably on other Commodore machines, you can
do this by pressing SHIFT+Commodore key.

Clone of `10 PRINT CHR$ (205.5 + RND (1)); : GOTO 10`.

```brainfuck
->+++++[-<---------->]<     Initializes the current cell to 205 the code for the backslash
[>@)[<+>>]<<.>[<->>]<<<]    Prints out maze Prints a backslash if the selected memory cell is 0 or a forward slash if it is not
```

This program relies on reading the values stored in the computers's memory to
generate "random" numbers. If the selected value in memory is 0, a backslash is
printed; if it is not 0, then a forward slash will be printed.

The data is random enough *some* of the time. You will find large areas with
only one type of slash.

## Release notes

- Initial release.
