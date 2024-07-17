/*
 * zlib license
 *
 * Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
 *
 * This software is provided ‘as-is’, without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */

/*
 * BASICfuck Read-Evaluate-Print Loop.
 *
 * Preprocessor parameters:
 *  - BASICFUCK_MEMORY_SIZE - The number of BASICfuck cells (bytes) to allocate.
 *  - TOOLKIT_VERSION - The version string for Brainblast-Toolkit.
 *  - HISTORY_STACK_SIZE - The size, in bytes, of the history stack.
 */

#include <conio.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>

#include "keyboard.h"
#define SCREEN_IMPLEMENTATION
#define SCREEN_BUFFER_SIZE 6
#include "screen.h"
#define TEXT_BUFFER_IMPLEMENTATION
#include "text-buffer.h"
#define BASICFUCK_IMPLEMENTATION
#include "basicfuck.h"



// Gets name of the key controls for the user.
#if defined(__CBM__)
#define STOP_KEY_EQUIVALENT  "STOP"
#define HOME_KEY_EQUIVALENT  "HOME"
#define CLEAR_KEY_EQUIVALENT "CLR"
#if defined(__PET__)
#define F1_KEY_EQUIVALENT    "\x5F (left arrow character)"
#define F2_KEY_EQUIVALENT    "\x5E (up arrow character)"
#else // __PET__
#define F1_KEY_EQUIVALENT    "F1"
#define F2_KEY_EQUIVALENT    "F2"
#endif
#elif defined(__ATARI__) // __CBM__
#define HOME_KEY_EQUIVALENT  "DELLINE (SHIFT+BACKSPACE)"
#define STOP_KEY_EQUIVALENT  "ESC"
#define CLEAR_KEY_EQUIVALENT "CLEAR"
#define F1_KEY_EQUIVALENT    "F1 (ATARI+1)"
#define F2_KEY_EQUIVALENT    "F2 (ATARI+2)"
#else // __ATARI__
#error build target not supported
#endif

/**
 * Runs the help menu, telling the user about the REPL and it's functions.
 */
static void help_menu() {
    clrscr();
    (void)puts("REPL Commands (must be at start of line):\n"
               "\n"
               "! - Exits REPL.\n"
               "? - Displays this help menu.\n"
               "# - Displays bytecode of last program.\n"
               "\n"
               "REPL Controls (Keypress):\n"
               "\n"
               STOP_KEY_EQUIVALENT " - Cancel input and start new line like C-c.\n"
               HOME_KEY_EQUIVALENT " - Move to start of line.\n"
               CLEAR_KEY_EQUIVALENT " - Clear screen and line.\n"
               F1_KEY_EQUIVALENT " - Previous history item.\n"
               F2_KEY_EQUIVALENT " - Next history item.\n"
               "\n"
               STOP_KEY_EQUIVALENT " - Abort BASICfuck program.\n"
               "\n"
               "Press ANY KEY to CONTINUE");
    (void)s_wrapped_cgetc();

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
    (void)s_wrapped_cgetc();

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
    (void)s_wrapped_cgetc();

    clrscr();
}



// Memory for the compiled bytecode of entered BASICfuck code.
#define PROGRAM_MEMORY_SIZE 256U
static baf_opcode_t program_memory[PROGRAM_MEMORY_SIZE];

/**
 * Displays a readout of the bytecode of the last program to the user. Holding
 * space will slow down the printing.
 *
 * @param program (global) - the program buffer.
 */
static void display_bytecode() {
    uint8_t i = 0;

    // Ideally display 16 bytes at a time, but screen real estate is what it is.
    uint8_t bytes_per_line = (s_width - 7) / 3;
    bytes_per_line = bytes_per_line > 16 ? 16 : bytes_per_line;

    while (true) {
        if (i % bytes_per_line == 0) {
            // Slow down while holding space.
            if (kbhit() != 0 && cgetc() == ' ')
                sleep(1);

            // Prints addresses.
            (void)fputs("\n$", stdout);
            s_utoa_fputs(4, i, 16);
            (void)putchar(':');
        }
        // Prints values.
        (void)putchar(' ');
        s_utoa_fputs(2, program_memory[i], 16);

        if (i >= PROGRAM_MEMORY_SIZE - 1)
            break;
        ++i;
    }

    (void)putchar('\n');
}



#define INPUT_BUFFER_SIZE 256U

int main(void) {
    static baf_cell_t BASICfuck_memory[BASICFUCK_MEMORY_SIZE];

    static uint8_t input_buffer[INPUT_BUFFER_SIZE];
    static uint8_t history_stack[HISTORY_STACK_SIZE];
    // Initalizes the history stack.
    tb_history_stack       = history_stack;
    tb_history_stack_size  = HISTORY_STACK_SIZE;
    tb_history_stack_index = 0;

    // Initializes global screen size variables in screen.h.
    screensize(&s_width, &s_height);
    // Initializes the opcode table in basicfuck.h.
    baf_initialize_instruction_opcode_table();

    // Initalizes the compiler.
    baf_compiler_read_buffer       = input_buffer;
    baf_compiler_write_buffer      = program_memory;
    baf_compiler_write_buffer_size = PROGRAM_MEMORY_SIZE;

    // Initializes the interpreter.
    baf_interpreter_program_memory = program_memory;
    baf_interpreter_bfmem          = BASICfuck_memory;
    baf_interpreter_bfmem_size     = BASICFUCK_MEMORY_SIZE;
    baf_interpreter_bfmem_index    = 0;
    baf_interpreter_cmem_pointer   = 0;


    clrscr();
    (void)puts("Brainblast-Toolkit BASICfuck REPL " TOOLKIT_VERSION "\n");
    s_utoa_fputs(0, BASICFUCK_MEMORY_SIZE, 10);
    (void)puts(" CELLS FREE\n"
               "\n"
               "Enter '?' for HELP\n"
               "Enter '!' to EXIT\n");

    while (true) {
        // Read.
        (void)fputs("YOUR WILL? ", stdout);
        tb_edit_buffer(input_buffer, INPUT_BUFFER_SIZE - 1);

        switch (input_buffer[0]) {
        case '\0':
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
        switch (baf_compile()) {
        case BAF_COMPILE_OUT_OF_MEMORY:
            (void)puts("?OUT OF MEMORY");
            continue;
        case BAF_COMPILE_UNTERMINATED_LOOP:
            (void)puts("?UNTERMINATED LOOP");
            continue;
        case BAF_COMPILE_SUCCESS: break;
        default: assert(0 && "Unreachable");
        }

        baf_interpret();

        // Prints cell value.
        s_utoa_fputs(3, baf_interpreter_bfmem[baf_interpreter_bfmem_index], 10);
        (void)fputs(" (Cell ", stdout);
        s_utoa_fputs(5, baf_interpreter_bfmem_index, 10);
        (void)fputs(", Memory $", stdout);
        s_utoa_fputs(4, (uint16_t)baf_interpreter_cmem_pointer, 16);
        (void)puts(")");
    }
 lexit_repl:


    return 0;
}
