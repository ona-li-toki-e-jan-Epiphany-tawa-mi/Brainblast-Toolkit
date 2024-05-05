# BASICfuck

A brainfuck REPL for the Commodore 64 written in BASIC.

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
- `;` - Stores the value of a key from the keyboard into the current cell (equivalent of `,`.)
- `[` - Jumps to corresponding ']' if value of the current cell cell is 0.
- `]` - Jumps to corresponding '[' if value of the current cell cell is not 0.
- `)` - Moves to the next location in computer memory.
- `(` - Moves to the previous location in computer memory.
- `@` - Reads a value from computer memory into the current cell.
- `*` - Writes the value from the current cell into computer memory.

## REPL controls

Pressing the F1 key while a BASICfuck program is running will abort it,
returning you to the input line.

#### REPL commands

To execute these commands, put them at the start of the input line and press
ENTER/RETURN:

- `!` - exits the REPL.
- `?` - displays the help menu.

## How to build

At the time of development, I used CBM prg Studio (https://www.ajordison.co.uk).
to create the .prg file. I don't remember how the process went, but it shouldn't
be too hard to figure out.

There's no need to build it though, as the .prg is included in this directory,
`basicfuck.prg`.

## Example programs

#### Hello World using '@'

```brainfuck
@>)@-[<+>--]<--.---.+++++++..+++.                      Prints HELLO
>(@[-<->]<.>)@[-<+>]<.--------.+++.------.--------.    Prints a space and WORLD
```

This utilizes the fact that the first two memory addresses of the C64 typically
hold 47 and 55 by default. Keep in mind that these values may change; this might
not always work.

#### Cycling screen border colors

```brainfuck
-[->++++++++++[-)))))))))))))))))))))]<]--[--((]+++[-(((((](    Moves the C64 memory pointer to $D020 the location of the border's color Takes forever
+[>*+<]                                                         "Rapidly" switches border color
```

#### Cat program / screen editor

```brainfuck
+[;.]
```

#### Reverse cat

```brainfuck
+[;.[->+>+<<]>>[-<<+>>]<-------------]<<[.[-]<]
```

Type what you want to be reversed, and then press enter. Your input will be
displayed as you type it.

#### "Random" maze thing

Clone of `10 PRINT CHR$ (205.5 + RND (1)); : GOTO 10`.

```brainfuck
->+++++[-<---------->]<     Initializes the current cell to 205 the code for the backslash
[>@)[<+>>]<<.>[<->>]<<<]    Prints out maze Prints a backslash if the selected C64 memory cell is 0 or a forward slash if it is not
```

This program relies on reading the values stored in the C64's memory to generate
"random" numbers. If the selected value in memory is 0, a backslash is printed;
if it is not 0, then a forward slash will be printed.

The data is random enough *some* of the time. You will find large areas with
only one type of slash.

## Links

Demonstration:<br>
https://odysee.com/BASICfuck-Demonstration---a-Brainfuck-REPL-for-the-Commodore-64:e0e5115474fe6334db53f93302e35adb86c45e6a?r=HYroMZaqrVN4gL5oSJ35gcTgt3K56r39
