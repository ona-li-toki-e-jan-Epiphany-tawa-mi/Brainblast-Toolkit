/*
 * This file is part of Brainblast-Toolkit.
 *
 * Brainblast-Toolkit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Brainblast-Toolkit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Brainblast-Toolkit. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * BASICfuck Read-Evaluate-Print Loop.
 *
 * Requires BASICFUCK_MEMORY_SIZE to be defined to be the number of BASICfuck
 * cells to allocate for the REPL to use.
 */

#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "text_buffer.h"
#include "bytecode_compiler.h"
#include "opcodes.h"
#include "keyboard.h"
#include "screen.h"



// Memory for the compiled bytecode of entered BASICfuck code.
#define PROGRAM_MEMORY_SIZE 256U
Opcode program_memory[PROGRAM_MEMORY_SIZE];

// Interpreter state.
uchar  BASICfuck_memory[BASICFUCK_MEMORY_SIZE];   // BASICfuck cell memory.
uint   BASICfuck_memory_index  = 0;               // the current index into cell memory.
uchar* computer_memory_pointer = 0;               // the current index into raw computer memory.



// Global variables for exchaning values with inline assembler.
uchar register_a, register_x, register_y;

/**
 * Runs the BASICfuck execute instruction.
 */
void execute() {
    register_a   = BASICfuck_memory[BASICfuck_memory_index];
    register_x   = BASICfuck_memory[BASICfuck_memory_index+1];
    register_y   = BASICfuck_memory[BASICfuck_memory_index+2];

    // Overwrites address of subroutine to call in next assembly block with the
    // computer memory pointer's value.
    __asm__ volatile ("lda     %v",   computer_memory_pointer);
    __asm__ volatile ("sta     %g+1", ljump_instruction);
    __asm__ volatile ("lda     %v+1", computer_memory_pointer);
    __asm__ volatile ("sta     %g+2", ljump_instruction);
    // Executes subroutine.
    __asm__ volatile ("lda     %v",   register_a);
    __asm__ volatile ("ldx     %v",   register_x);
    __asm__ volatile ("ldy     %v",   register_y);
 ljump_instruction:
    __asm__ volatile ("jsr     %w",   NULL);
    // Retrieves resuting values.
    __asm__ volatile ("sta     %v",   register_a);
    __asm__ volatile ("stx     %v",   register_x);
    __asm__ volatile ("sty     %v",   register_y);

    BASICfuck_memory[BASICfuck_memory_index]   = register_a;
    BASICfuck_memory[BASICfuck_memory_index+1] = register_x;
    BASICfuck_memory[BASICfuck_memory_index+2] = register_y;

    return;
    // If we don't include a jmp instruction, cc65, annoyingly, strips it from
    // the resulting assembly.
    __asm__ volatile ("jmp     %g", ljump_instruction);
}

/**
 * Runs the interpreter with the given bytecode-compiled BASICfuck program,
 * leaving the given starting state off wherever it the program finished at.
 *
 * @param program_memory (global) - the BASICfuck program. Must be no larger
 *        than 256 bytes.
 * @param BASICfuck_memory (global) - the starting address of BASICfuck memory.
 * @param BASICfuck_memory_index (global) - the current index into BASICfuck
 *        memory.
 * @param computer_memory_pointer (global) - the current index into raw computer
 *        memory.
 */
void run_interpreter() {
    Opcode opcode;
    uchar  argument;

    uchar program_index = 0;

    static const void *const jump_table[] = {
        &&lopcode_end_program,                    // BASICFUCK_END_PROGRAM.
        &&lopcode_increment,                      // BASICFUCK_INCREMENT.
        &&lopcode_decrement,                      // BAISCFUCK_DECREMENT.
        &&lopcode_bfmemory_left,                  // BASICFUCK_BFMEMORY_LEFT.
        &&lopcode_bfmemory_right,                 // BASICFUCK_BFMEMORY_RIGHT.
        &&lopcode_print,                          // BASICFUCK_PRINT.
        &&lopcode_input,                          // BASICFUCK_INPUT.
        &&lopcode_jeq,                            // BASICFUCK_JEQ.
        &&lopcode_jne,                            // BASICFUCK_JNE.
        &&lopcode_cmemory_read,                   // BASICFUCK_CMEMORY_READ.
        &&lopcode_cmemory_write,                  // BASICFUCK_CMEMORY_WRITE.
        &&lopcode_cmemory_left,                   // BASICFUCK_CMEMORY_LEFT.
        &&lopcode_cmemory_right,                  // BASICFUCK_CMEMORY_RIGHT.
        &&lopcode_execute                         // BASICFUCK_EXECUTE.
    };


    while (true) {
        if (kbhit() != 0 && cgetc() == KEYBOARD_STOP) {
            puts("?ABORT");
            break;
        }


        opcode   = program_memory[program_index];
        argument = program_memory[program_index+1];
        assert(opcode < sizeof(jump_table)/sizeof(jump_table[0]));
        goto *jump_table[opcode];

    lopcode_end_program:
        break;

    lopcode_increment:
        BASICfuck_memory[BASICfuck_memory_index] += argument;
        goto lfinish_interpreter_cycle;

    lopcode_decrement:
        BASICfuck_memory[BASICfuck_memory_index] -= argument;
        goto lfinish_interpreter_cycle;

    lopcode_bfmemory_left:
        if (BASICfuck_memory_index > argument) {
            BASICfuck_memory_index -= argument;
        } else {
            BASICfuck_memory_index = 0;
        }
        goto lfinish_interpreter_cycle;

    lopcode_bfmemory_right:
        if (BASICfuck_memory_index + argument < BASICFUCK_MEMORY_SIZE)
            BASICfuck_memory_index += argument;
        goto lfinish_interpreter_cycle;

    lopcode_print:
        (void)putchar(BASICfuck_memory[BASICfuck_memory_index]);
        goto lfinish_interpreter_cycle;

    lopcode_input:
        BASICfuck_memory[BASICfuck_memory_index] = cgetc();
        goto lfinish_interpreter_cycle;

    lopcode_jeq:
        if (BASICfuck_memory[BASICfuck_memory_index] == 0) {
            // Since the program can only be 256 bytes long, we can ignore
            // the high byte of the address.
            program_index = argument;
        }
        goto lfinish_interpreter_cycle;

    lopcode_jne:
        if (BASICfuck_memory[BASICfuck_memory_index] != 0) {
            // Since the program can only be 256 bytes long, we can ignore
            // the high byte of the address.
            program_index = argument;
        }
        goto lfinish_interpreter_cycle;

    lopcode_cmemory_read:
        BASICfuck_memory[BASICfuck_memory_index] = *computer_memory_pointer;
        goto lfinish_interpreter_cycle;

    lopcode_cmemory_write:
        *computer_memory_pointer = BASICfuck_memory[BASICfuck_memory_index];
        goto lfinish_interpreter_cycle;

    lopcode_cmemory_left:
        if ((uint)computer_memory_pointer > argument) {
            computer_memory_pointer -= argument;
        } else {
            computer_memory_pointer = 0;
        }
        goto lfinish_interpreter_cycle;

    lopcode_cmemory_right:
        if (UINT_MAX - (uint)computer_memory_pointer > argument) {
            computer_memory_pointer += argument;
        } else {
            computer_memory_pointer = (uchar*)UINT_MAX;
        }
        goto lfinish_interpreter_cycle;

    lopcode_execute:
        execute();
        goto lfinish_interpreter_cycle;


    lfinish_interpreter_cycle:
        // Jumped to after an opcode has been executed.
        program_index += opcode_size_table[opcode];
    }
}



#ifdef __PET__
#define F1_KEY_EQUIVALENT "\x5F (left arrow character)"
#define F2_KEY_EQUIVALENT "\x5E (up arrow character)"
#else // __PET__
#define F1_KEY_EQUIVALENT "F1"
#define F2_KEY_EQUIVALENT "F2"
#endif

/**
 * Runs the help menu, telling the user about the REPL and it's functions.
 */
void help_menu(void) {
    clrscr();
    (void)puts("REPL Commands (must be at start of line):\n"
               "\n"
               "! - Exits REPL.\n"
               "? - Displays this help menu.\n"
               "# - Displays bytecode of last program.\n"
               "\n"
               "REPL Controls (Keypress):\n"
               "\n"
               "STOP - Cancel input and start new line.\n"
               "       Like C-c.\n"
               "HOME - Move to start of line.\n"
               "CLR - Clear screen and line.\n"
               F1_KEY_EQUIVALENT " - Previous history item.\n"
               F2_KEY_EQUIVALENT " - Next history item.\n"
               "\n"
               F1_KEY_EQUIVALENT " - Abort BASICfuck program.\n"
               "\n"
               "Press ANY KEY to CONTINUE");
    (void)cgetc();

    clrscr();
    (void)puts("BASICfuck Instructions (Part 1):\n"
               "\n"
               "+ - Increment cell.\n"
               "- - Decrement cell.\n"
               "> - Move to next cell.\n"
               "< - Move to previous cell.\n"
               ". - Display value in cell as character.\n"
               ", - Store value of key from keyboard in cell.\n"
               "[ - Jump to corresponding ']' if value of cell is 0.\n"
               "] - Jump to corresponding '[' if value of cell is not 0.\n"
               "\n"
               "Press ANY KEY to CONTINUE");
    (void)cgetc();

    clrscr();
    (void)puts("BASICfuck Instructions (Part 2):\n"
               "\n"
               ") - Move to next location in computer memory.\n"
               "( - Move to previous location in computer memory.\n"
               "@ - Read value from computer memory into cell.\n"
               "* - Write value from cell into computer memory\n"
               "% - Execute location in computer memory as subroutine. The values of the current and next two cells will be used for the A, X, and Y registers. Resulting register values will be stored back into the respective cells.\n"
               "\n"
               "Press ANY KEY to CONTINUE");
    (void)cgetc();

    clrscr();
}

/**
 * Displays a readout of the bytecode of the last program to the user. Holding
 * space will slow down the printing.
 *
 * @param program (global) - the program buffer.
 */
void display_bytecode() {
    uchar i = 0;

    // Ideally display 16 bytes at a time, but screen real estate is what it is.
    uchar bytes_per_line = (screen_width - 7) / 3;
    bytes_per_line = bytes_per_line > 16 ? 16 : bytes_per_line;

    while (true) {
        if (i % bytes_per_line == 0) {
            // Slow down while holding space.
            if (kbhit() != 0 && cgetc() == ' ')
                sleep(1);

            // Prints addresses.
            (void)fputs("\n$", stdout);
            cputhex16((uint)i);
            (void)putchar(':');
        }
        // Prints values.
        (void)putchar(' ');
        cputhex8(program_memory[i]);

        if (i >= PROGRAM_MEMORY_SIZE - 1)
            break;
        ++i;
    }

    (void)putchar('\n');
}



#define INPUT_BUFFER_SIZE 256U

int main(void) {
    uchar input_buffer[INPUT_BUFFER_SIZE];
    // Used to avoid calling the bloated printf. 6 chars because 5 digit number
    // + null-terminator.
    uchar number_to_string_buffer[6];
    // The number of zeros to put before the cell number.
    size_t leading_zero_count;

    // Initializes global screen size variables in screen.h.
    screensize(&screen_width, &screen_height);
    // Initializes the opcode table in opcodes.h.
    initialize_instruction_opcode_table();


    clrscr();
    (void)puts("Brainblast-Toolkit BASICfuck REPL 0.1.0\n");
    (void)fputs( utoa(BASICFUCK_MEMORY_SIZE, number_to_string_buffer, 10)
               , stdout);
    (void)puts(" CELLS FREE\n"
               "\n"
               "Enter '?' for HELP\n"
               "Enter '!' to EXIT\n");

    while (true) {
        // Run.
        (void)fputs("YOUR WILL? ", stdout);
        edit_buffer(input_buffer, INPUT_BUFFER_SIZE - 1);

        switch (input_buffer[0]) {
        case NULL:
            continue;                             // empty input.

        case '!':
            (void)puts("SO BE IT.");
            goto lexit_repl;

        case '?':
            help_menu();
            continue;

        case '#':
            display_bytecode();
            continue;
        }

        // Evaluate.
        switch (bytecode_compile(input_buffer, program_memory, PROGRAM_MEMORY_SIZE)) {
        case BCCOMPILE_OUT_OF_MEMORY:
            (void)puts("?OUT OF MEMORY");
            continue;
        case BCCOMPILE_UNTERMINATED_LOOP:
            (void)puts("?UNTERMINATED LOOP");
            continue;
        }

        run_interpreter();

        // Print.
        (void)fputs( utoa(BASICfuck_memory[BASICfuck_memory_index], number_to_string_buffer, 10)
                   , stdout);
        (void)fputs(" (Cell ", stdout);
        leading_zero_count = 5 - strlen(utoa(BASICfuck_memory_index, number_to_string_buffer, 10));
        for (; leading_zero_count > 0; --leading_zero_count)
            (void)fputs("0", stdout);
        (void)fputs(number_to_string_buffer, stdout);
        (void)fputs(", Memory $", stdout);
        cputhex16((uint)computer_memory_pointer);
        (void)puts(")");
    }
 lexit_repl:


    return 0;
}
