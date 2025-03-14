/*
 * This file is part of BASICfuck.
 *
 * Copyright (c) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi
 *
 * BASICfuck is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BASICfuck is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BASICfuck. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * BASICfuck Read-Evaluate-Print Loop.
 *
 * Preprocessor parameters:
 * - BASICFUCK_MEMORY_SIZE - The number of BASICfuck cells (bytes) to allocate.
 * - HISTORY_STACK_SIZE - The size, in bytes, of the history stack.
 */

#include <assert.h>
#include <conio.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////
// Utilities                                                                   //
////////////////////////////////////////////////////////////////////////////////

// The width and height of the screen. Must be initalized at some point with
// screensize(), or some other method, else they will be set to 0.
static uint8_t width  = 0;
static uint8_t height = 0;

// On the Commander X16, cgetc() doesn't block like expected, and instead
// returns immediately. This version adds in a check to ensure the blocking
// behavior.
static uint8_t wrappedCgetc(void) {
#ifdef __CX16__
    uint8_t character = 0;
    do {
        character = cgetc();
    } while ('\0' == character);
    return character;
#else // __CX16__
    return cgetc();
#endif
}

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

////////////////////////////////////////////////////////////////////////////////
// Keyboard                                                                   //
////////////////////////////////////////////////////////////////////////////////

#if defined(__CBM__)
#  include <cbm.h>
#  define KEYBOARD_UP          CH_CURS_UP
#  define KEYBOARD_DOWN        CH_CURS_DOWN
#  define KEYBOARD_LEFT        CH_CURS_LEFT
#  define KEYBOARD_RIGHT       CH_CURS_RIGHT
#  define KEYBOARD_BACKSPACE   CH_DEL
#  define KEYBOARD_INSERT      CH_INS
#  define KEYBOARD_ENTER       CH_ENTER
#  define KEYBOARD_STOP        CH_STOP
#  define KEYBOARD_STOP_STRING "STOP"
#  define KEYBOARD_HOME        CH_HOME
#  define KEYBOARD_HOME_STRING "HOME"
// Not defined in cbm.h, but the same for all CBM computers.
#  define KEYBOARD_CLEAR        0x93
#  define KEYBOARD_CLEAR_STRING "CLR"
#  if defined( __C16__) || defined( __C64__) || defined(__C128__) || defined(__CX16__)
#    define KEYBOARD_F1        CH_F1
#    define KEYBOARD_F1_STRING "F1"
#    define KEYBOARD_F2        CH_F2
#    define KEYBOARD_F2_STRING "F2"
#  elif defined(__PET__) // __C16__ || __C64__ || __C128__ || __CX16__
#    define KEYBOARD_F1        0x5F
#    define KEYBOARD_F1_STRING "\x5F (left arrow character)"
#    define KEYBOARD_F2        0x5E
#    define KEYBOARD_F2_STRING "\x5E (up arrow character)"
#  else // __PET__
#    error build target not supported
#  endif

#elif defined(__ATARI__) // __CBM__
#  include <atari.h>
#  define KEYBOARD_UP           CH_CURS_UP
#  define KEYBOARD_DOWN         CH_CURS_DOWN
#  define KEYBOARD_LEFT         CH_CURS_LEFT
#  define KEYBOARD_RIGHT        CH_CURS_RIGHT
#  define KEYBOARD_BACKSPACE    CH_DEL
#  define KEYBOARD_INSERT       KEY_INSERT
#  define KEYBOARD_ENTER        CH_ENTER
#  define KEYBOARD_STOP         CH_ESC
#  define KEYBOARD_STOP_STRING  "ESC"
#  define KEYBOARD_HOME         CH_DELLINE
#  define KEYBOARD_HOME_STRING  "DELLINE (SHIFT+BACKSPACE)"
#  define KEYBOARD_CLEAR        CH_CLR
#  define KEYBOARD_CLEAR_STRING "CLEAR"
#  define KEYBOARD_F1           CH_F1
#  define KEYBOARD_F1_STRING    "F1 (ATARI+1)"
#  define KEYBOARD_F2           CH_F2
#  define KEYBOARD_F2_STRING    "F2 (ATARI+2)"

#else // __ATARI__
#  error build target not supported
#endif // #else

////////////////////////////////////////////////////////////////////////////////
// Text Buffers                                                               //
////////////////////////////////////////////////////////////////////////////////

// Runs cgetc() with a blinking cursor.
// To set a blinking cursor (easily,) you need to use the cursor() function from
// conio.h, but it seems to error with "Illegal function call" or something when
// used in a complex function, so I have it pulled out into this separate one.
static uint8_t blinkingCgetc(void) {
    uint8_t character = 0;

    cursor(true);
    character = wrappedCgetc();
    cursor(false);

    return character;
}

// Returns whether the given character is a screen control character.
static bool isControlCharacter(const uint8_t character) {
#if defined(__CBM__)
    // PETSCII character set.
    return (character & 0x7F) < 0x20;
#elif defined(__ATARI__)
    // ATASCII character set.
    return (character >= 0x1B && character <= 0x1F) ||
           (character >= 0x7D && character <= 0x7F) ||
           (character >= 0x8B && character <= 0x8F) ||
           character >= 0xFD;
#else
#  error build target not supported
#endif
}

// History stack state.
static uint8_t  history_stack[HISTORY_STACK_SIZE] = {0};
static uint16_t history_stack_index               = 0;

// Increments the history stack index and loops it around if it goes out of
// bounds.
// history_stack_index (global) - current index into the history stack.
static void incrementHistoryStackIndex(void) {
    ++history_stack_index;
    if (history_stack_index >= HISTORY_STACK_SIZE) {
        history_stack_index = 0;
    }
}

// Decrements the history stack index and loops it around if it goes out of
// bounds.
static void decrementHistoryStackIndex(void) {
    if (0 == history_stack_index) {
        history_stack_index = HISTORY_STACK_SIZE - 1;
    } else {
        --history_stack_index;
    }
}

// Edit buffer state.
// The buffer currently being edited.
#define EDIT_BUFFER_SIZE 255 // No more than 255.
static uint8_t edit_buffer[EDIT_BUFFER_SIZE] = {0};
// The location of the user's cursor inside the buffer.
static uint8_t edit_buffer_cursor = 0;
// How much of the buffer is taken up by the text typed by the user.
static uint8_t edit_buffer_input_size = 0;

// Saves the edit buffer to the history stack for later recollection.
static void saveEditBuffer(void) {
    uint8_t character    = 0;
    uint8_t buffer_index = 0;

    if (NULL == edit_buffer[buffer_index]) {
        return;
    }

    do {
        character                          = edit_buffer[buffer_index];
        history_stack[history_stack_index] = character;

        incrementHistoryStackIndex();
        ++buffer_index;
    } while (NULL != character);
}

// Recalls, into the edit buffer, the previous input if foward_recall is false,
// else recalls the next input from the history stack.
static void recallEditBuffer(const bool forward_recall) {
    uint8_t  character;
    uint16_t final_history_index = history_stack_index;

    // Moves forwards or backwards to the next block in the history stack.
    if (forward_recall) {
        while (NULL != history_stack[history_stack_index])
            incrementHistoryStackIndex();
        incrementHistoryStackIndex();

    } else {
        decrementHistoryStackIndex();
        do {
            decrementHistoryStackIndex();
        } while (NULL != history_stack[history_stack_index]);
        incrementHistoryStackIndex();
    }

    // If a NULL is found at the next block, that means that it's at the end of
    // the history buffer.
    if (NULL == history_stack[history_stack_index]) {
        history_stack_index = final_history_index;
        return;
    }
    // Preservers the index of this block as we will stay here in case of
    // succesive movements through the history.
    final_history_index = history_stack_index;

    // Navigates visual cursor to the end of the buffer.
    for (; edit_buffer_cursor < edit_buffer_input_size; ++edit_buffer_cursor)
        putchar(KEYBOARD_RIGHT);
    // Clears visual buffer.
    while (edit_buffer_cursor > 0) {
        putchar(KEYBOARD_BACKSPACE);
        --edit_buffer_cursor;
    }

    // Reads from history buffer into buffer.
    while (true) {
        character                       = history_stack[history_stack_index];
        edit_buffer[edit_buffer_cursor] = character;
        putchar(character);

        if (NULL == character)
            break;

        incrementHistoryStackIndex();
        ++edit_buffer_cursor;
    }
    edit_buffer_input_size = edit_buffer_cursor;

    // Restores history stack index to the start of the current block so that
    // moving forwards and backwords works properly.
    history_stack_index = final_history_index;
}

// Creates an editable text buffer, starting from the current position on the
// screen, and stores what the user typed into the edit buffer with a
// null-terminator.
// The cursor on the screen will be moved to the line after the filled portion
// of the text buffer once done.
static void editEditBuffer(void) {
    uint8_t new_cursor = 0;
    uint8_t key        = 0;

    // Reset edit buffer state.
    edit_buffer_cursor     = 0;
    edit_buffer_input_size = 0;

    while (true) {
        key = blinkingCgetc();

        switch (key) {
        // Finalizes the buffer and exits from this function.
        case KEYBOARD_ENTER: {
            // Writes out null-terminator.
            edit_buffer[edit_buffer_input_size] = NULL;
            // Navigates to then end of buffer if neccesary.
            while (edit_buffer_cursor < edit_buffer_input_size) {
                putchar(KEYBOARD_RIGHT);
                ++edit_buffer_cursor;
            }
            putchar('\n');
            goto lquit_editing_buffer;
        }

        // "Clears" the input buffer and exits from this function.
        case KEYBOARD_STOP: {
            edit_buffer[0] = NULL;
            putchar('\n');
            goto lquit_editing_buffer;
        }

        // Clears the screen and input buffer and exits from this function.
        case KEYBOARD_CLEAR: {
            edit_buffer[0] = NULL;
            clrscr();
            goto lquit_editing_buffer;
        }

        // Deletes characters from the buffer.
        case KEYBOARD_BACKSPACE: {
            if (edit_buffer_cursor == 0) break;

            putchar(KEYBOARD_BACKSPACE);
            // Shifts characters in buffer to the left, overwiting the deleted
            // character.
            memmove(
                edit_buffer + edit_buffer_cursor - 1,
                edit_buffer + edit_buffer_cursor,
                edit_buffer_input_size - edit_buffer_cursor
            );
            --edit_buffer_input_size;
            --edit_buffer_cursor;

            break;
        }

        // Handles arrow keys, moving through the buffer.
        case KEYBOARD_LEFT: {
            if (edit_buffer_cursor > 0) {
                --edit_buffer_cursor;
                putchar(KEYBOARD_LEFT);
            }
            break;
        }

        case KEYBOARD_RIGHT: {
            if (edit_buffer_cursor < edit_buffer_input_size) {
                ++edit_buffer_cursor;
                putchar(KEYBOARD_RIGHT);
            }
            break;
        }

        case KEYBOARD_UP: {
            // Navigates to the next line up, or to the start of the buffer, if
            // there is no line there.
            new_cursor = edit_buffer_cursor > width
                         ? edit_buffer_cursor - width : 0;
            for (; edit_buffer_cursor > new_cursor; --edit_buffer_cursor) {
                putchar(KEYBOARD_LEFT);
            }
            break;
        }

        case KEYBOARD_DOWN: {
            // Navigates to the next line down, or to the end of the filled
            // buffer, if there is no line there.
            new_cursor = edit_buffer_input_size - edit_buffer_cursor > width
                         ? edit_buffer_cursor + width : edit_buffer_input_size;
            for (; edit_buffer_cursor < new_cursor; ++edit_buffer_cursor) {
                putchar(KEYBOARD_RIGHT);
            }
            break;
        }

        // Handles HOME, moving to the start of the buffer.
        case KEYBOARD_HOME: {
            for (; edit_buffer_cursor > 0; --edit_buffer_cursor) {
                putchar(KEYBOARD_LEFT);
            }
            break;
        }

        // Handles INST, inserting characters into the buffer.
        case KEYBOARD_INSERT: {
            if (edit_buffer_input_size >= EDIT_BUFFER_SIZE
            || edit_buffer_cursor == edit_buffer_input_size) {
                break;
            }

            putchar(KEYBOARD_INSERT);
            // Shifts characters in buffer to the right, making space for the
            // new one.
            memmove(
                edit_buffer + edit_buffer_cursor + 1,
                edit_buffer + edit_buffer_cursor,
                edit_buffer_input_size - edit_buffer_cursor
            );
            edit_buffer_input_size++;
            edit_buffer[edit_buffer_cursor] = ' ';

            break;
        }

        // Handles function keys, navigating through the history buffer.
        case KEYBOARD_F1: {
            recallEditBuffer(false);
            break;
        }

        case KEYBOARD_F2: {
            recallEditBuffer(true);
            break;
        }

        // Handles typing characters.
        default: {
            if (isControlCharacter(key))                break;
            if (edit_buffer_cursor >= EDIT_BUFFER_SIZE) break;

            // Increases buffer size if adding characters to the end.
            if (edit_buffer_cursor == edit_buffer_input_size)
                ++edit_buffer_input_size;
            edit_buffer[edit_buffer_cursor] = key;
            ++edit_buffer_cursor;
            putchar(key);
        }
        }
    }
lquit_editing_buffer:

    saveEditBuffer();
    return;
}

////////////////////////////////////////////////////////////////////////////////
// BASICfuck                                                                  //
////////////////////////////////////////////////////////////////////////////////

typedef uint8_t opcode_t;
// Ends the current BASICfuck program.
#define OPCODE_HALT 0x00
// Increments the current cell.
// argument1 - the amount to increment by.
#define OPCODE_INCREMENT 0x01
// Decrements the current cell.
// argument1 - the amount to decrement by.
#define OPCODE_DECREMENT 0x02
// Moves the the cell pointer to the left.
// argument1 - the number of times to move to the left.
#define OPCODE_BFMEM_LEFT 0x03
// Moves the the cell pointer to the right.
// argument1 - the number of times to move to the right.
#define OPCODE_BFMEM_RIGHT 0x04
// Prints the value in the current cell as PETSCII character.
#define OPCODE_PRINT 0x05
// Awaits a value from the keyboard and stores it in the current cell.
#define OPCODE_INPUT 0x06
// Jumps to the given address if the current cell is 0.
// argument1,2 - the address in program memory to jump to.
#define OPCODE_JEQ 0x07
// Jumps to the given address if the current cell is not 0.
// argument1,2 - the address in program memory to jump to.
#define OPCODE_JNE 0x08
// Reads the value at the computer memory pointer into the current cell.
#define OPCODE_CMEM_READ 0x09
// Writes the value in the current cell to the location at the computer memory
// pointer.
#define OPCODE_CMEM_WRITE 0x0A
// Moves the computer memory pointer to the left.
// argument1 - the number of times to move to the left.
#define OPCODE_CMEM_LEFT 0x0B
// Moves the computer memory pointer to the right.
// argument1 - the number of times to move to the right.
#define OPCODE_CMEM_RIGHT 0x0C
// Runs the subroutine at the computer memory pointer with the current and next
// two cells as the values for the X, Y, and Z registers.
#define OPCODE_EXECUTE 0x0D

// A table mapping from opcodes to their size (opcode + arguments) in bytes.
// Index value must be valid opcode.
static const uint8_t opcode_size_table[] = {
    1, // OPCODE_HALT.
    2, // OPCODE_INCREMENT.
    2, // OPCODE_DECREMENT.
    2, // OPCODE_BFMEM_LEFT.
    2, // OPCODE_BFMEM_RIGHT.
    1, // OPCODE_PRINT.
    1, // OPCODE_INPUT.
    3, // OPCODE_JEQ.
    3, // OPCODE_JNE.
    1, // OPCODE_CMEM_READ.
    1, // OPCODE_CMEM_WRITE.
    2, // OPCODE_CMEM_LEFT.
    2, // OPCODE_CMEM_RIGHT.
    1  // OPCODE_EXECUTE.
};

// A table mapping from instruction characters to their corresponding opcodes.
// Index value must not exceed 255.
// Must call baf_initialize_instruction_opcode_table() once prior to use.
// If the given instruction does not have an opcode, 0xFF will be returned.
static opcode_t instruction_opcode_table[256];

// A one-time-call function used to initialize instruction_opcode_table[].
// TODO: see if array can be intialized at compile time.
static void initializeInstructionOpcodeTable(void) {
    uint8_t i = 0;
    for (; i < 255; ++i) instruction_opcode_table[i] = 0xFF;
    instruction_opcode_table[255] = 0xFF;

    instruction_opcode_table[NULL] = OPCODE_HALT;
    instruction_opcode_table['+']  = OPCODE_INCREMENT;
    instruction_opcode_table['-']  = OPCODE_DECREMENT;
    instruction_opcode_table['<']  = OPCODE_BFMEM_LEFT;
    instruction_opcode_table['>']  = OPCODE_BFMEM_RIGHT;
    instruction_opcode_table['.']  = OPCODE_PRINT;
    instruction_opcode_table[',']  = OPCODE_INPUT;
    instruction_opcode_table['[']  = OPCODE_JEQ;
    instruction_opcode_table[']']  = OPCODE_JNE;
    instruction_opcode_table['@']  = OPCODE_CMEM_READ;
    instruction_opcode_table['*']  = OPCODE_CMEM_WRITE;
    instruction_opcode_table['(']  = OPCODE_CMEM_LEFT;
    instruction_opcode_table[')']  = OPCODE_CMEM_RIGHT;
    instruction_opcode_table['%']  = OPCODE_EXECUTE;
}

// Memory for the compiled bytecode of entered BASICfuck code.
#define PROGRAM_MEMORY_SIZE 256
static opcode_t program_memory[PROGRAM_MEMORY_SIZE] = {0};

// Compiler state.
// Pointer to the current position in the read buffer.
static const uint8_t* compiler_read_pointer = NULL;
// Pointer to the current position in the write buffer.
static opcode_t* compiler_write_pointer = NULL;
// Pointer to the end of the write buffer.
static opcode_t* compiler_write_pointer_end = NULL;

// Performs the first pass of BASICfuck compilation, converting the text program
// to opcodes.
// true if succeeded, false if ran out of memory.
static bool compileFirstPass(void) {
    uint8_t  instruction = 0;
    opcode_t opcode      = 0;

    // Used by counted instructions.
    uint16_t instruction_count = 0;
    uint8_t  other_instruction = 0;
    uint8_t  chunk_count       = 0;

    static const void *const jump_table[] = {
        &&lfinish_bytecode_compilation,      // OPCODE_HALT.
        &&lcompile_counted_instruction,      // OPCODE_INCREMENT.
        &&lcompile_counted_instruction,      // OPCODE_DECREMENT.
        &&lcompile_counted_instruction,      // OPCODE_BFMEM_LEFT.
        &&lcompile_counted_instruction,      // OPCODE_BFMEM_RIGHT.
        &&lcompile_instruction_no_arugments, // OPCODE_PRINT.
        &&lcompile_instruction_no_arugments, // OPCODE_INPUT.
        &&lcompile_jump_instruction,         // OPCODE_JEQ.
        &&lcompile_jump_instruction,         // OPCODE_JNE.
        &&lcompile_instruction_no_arugments, // OPCODE_CMEM_READ.
        &&lcompile_instruction_no_arugments, // OPCODE_CMEM_WRITE.
        &&lcompile_counted_instruction,      // OPCODE_CMEM_LEFT.
        &&lcompile_counted_instruction,      // OPCODE_CMEM_RIGHT.
        &&lcompile_instruction_no_arugments  // OPCODE_EXECUTE.
    };

    // Initialize compiler.
    compiler_read_pointer = edit_buffer;
    compiler_write_pointer = program_memory;
    compiler_write_pointer_end = PROGRAM_MEMORY_SIZE - 1 + program_memory;

    while (true) {
        instruction = *compiler_read_pointer;
        opcode      = instruction_opcode_table[instruction];

        // Ignores non-instructions.
        if (opcode == 0xFF) {
            ++compiler_read_pointer;
            continue;
        }

        assert(opcode < ARRAY_SIZE(jump_table) && "unreachable");
        goto *jump_table[opcode];

        // End of program.
lfinish_bytecode_compilation: {
            *compiler_write_pointer = OPCODE_HALT;
            break;
        }

        // Takes no arguments.
lcompile_instruction_no_arugments: {
            if (compiler_write_pointer >= compiler_write_pointer_end) {
                return false;
            }

            *(compiler_write_pointer++) = opcode;
            ++compiler_read_pointer;

            continue;
        }

        // Takes a 16-bit address relative to program memory as a parameter,
        // which will be handled by the second pass.
lcompile_jump_instruction: {
            if (compiler_write_pointer + 2 >= compiler_write_pointer_end) {
                return false;
            }

            *(compiler_write_pointer++) = opcode;
            *(compiler_write_pointer++) = 0xFF;
            *(compiler_write_pointer++) = 0xFF;
            ++compiler_read_pointer;

            continue;
        }

        // Takes an 8-bit count of how many times to preform the operation.
lcompile_counted_instruction: {
            instruction_count = 0;

            // Count number of consecutive instructions.
            while (true) {
                other_instruction = *compiler_read_pointer;

                if (other_instruction != instruction) {
                    break;
                }

                ++instruction_count;
                ++compiler_read_pointer;
            }

            // Each instruction opcode can only take an 8-bit value, so this chops up
            // the full count into separate 8-bit chunks.
            while (instruction_count > 0) {
                if (compiler_write_pointer + 1 >= compiler_write_pointer_end) {
                    return false;
                }

                chunk_count = instruction_count > 255 ? 255
                              : (uint8_t)instruction_count;

                *(compiler_write_pointer++) = opcode;
                *(compiler_write_pointer++) = chunk_count;

                instruction_count -= (uint16_t)chunk_count;
            }

            continue;
        }
    }

    return true;
}

// Performs the second pass of BASICfuck compilation, calculating the addresses
// for jump instructions.
// Returns true if succeeded, false if there is an unterminated loop.
static bool compileSecondPass(void) {
    opcode_t* write_start_pointer = compiler_write_pointer;
    opcode_t  opcode              = 0;
    uint16_t  loop_depth          = 0;

    opcode_t* seek_pointer  = NULL;
    opcode_t  seeked_opcode = 0;

    // Initialize compiler.
    compiler_write_pointer = program_memory;

    while (OPCODE_HALT != (opcode = *compiler_write_pointer)) {
        switch (opcode) {
        case OPCODE_JEQ:
            seek_pointer = compiler_write_pointer + opcode_size_table[OPCODE_JEQ];
            loop_depth   = 1;

            // Finds and links with accomanying JNE instruction.
            while (OPCODE_HALT != (seeked_opcode = *seek_pointer)) {
                switch (seeked_opcode) {
                case OPCODE_JEQ: {
                    ++loop_depth;
                    break;
                }
                case OPCODE_JNE: {
                    --loop_depth;
                    break;
                }
                }

                if (loop_depth == 0) {
                    // Sets JEQ instruction to jump to accomanying JNE.
                    *(opcode_t**)(compiler_write_pointer + 1) = seek_pointer;
                    // And vice-versa.
                    *(opcode_t**)(seek_pointer + 1) = compiler_write_pointer;

                    break;
                }

                seek_pointer += opcode_size_table[seeked_opcode];
            }

            if (loop_depth != 0) return false;

            break;

        case OPCODE_JNE:
            // Address should have been set by some preceeding JEQ instruction.
            if (0xFFFF == *(uint16_t*)(compiler_write_pointer + 1)) {
                return false;
            }

            break;
        }

        compiler_write_pointer += opcode_size_table[opcode];
    }

    return true;
}

typedef uint8_t cell_t;

static cell_t basicfuck_memory[BASICFUCK_MEMORY_SIZE] = {0};
// Pointer to one after the end of the memory.
static const cell_t *const basicfuck_memory_end = basicfuck_memory +
    BASICFUCK_MEMORY_SIZE;

// Interpreter state.
static const opcode_t* interpreter_program_pointer = NULL;
static cell_t* interpreter_bfmem_pointer = basicfuck_memory;
static uint8_t* interpreter_cmem_pointer = NULL;

// Global variables for exchaning values with inline assembler.
static uint8_t interpreter_register_a = 0;
static uint8_t interpreter_register_x = 0;
static uint8_t interpreter_register_y = 0;

// Runs the execute part of the BASICfuck execute instruction.
// interpreter_register_a (global) - the value to place in the A register.
// interpreter_register_x (global) - the value to place in the X register.
// interpreter_register_y (global) - the value to place in the Y register.
// interpreter_cmem_pointer (global) - the address to execute as a subroutine.
static void basicfuckExecute(void) {
    // Overwrites address of subroutine to call in next assembly block with the
    // computer memory pointer's value.
    __asm__ volatile ("lda %v",   interpreter_cmem_pointer);
    __asm__ volatile ("sta %g+1", ljump_instruction);
    __asm__ volatile ("lda %v+1", interpreter_cmem_pointer);
    __asm__ volatile ("sta %g+2", ljump_instruction);
    // Executes subroutine.
    __asm__ volatile ("lda %v", interpreter_register_a);
    __asm__ volatile ("ldx %v", interpreter_register_x);
    __asm__ volatile ("ldy %v", interpreter_register_y);
ljump_instruction:
    __asm__ volatile ("jsr %w", NULL);
    // Retrieves resuting values.
    __asm__ volatile ("sta %v", interpreter_register_a);
    __asm__ volatile ("stx %v", interpreter_register_x);
    __asm__ volatile ("sty %v", interpreter_register_y);

    return;
    // If we don't include a jmp instruction, cc65, annoyingly, strips the label
    // from the resulting assembly.
    __asm__ volatile ("jmp %g", ljump_instruction);
}

// Runs the interpreter with the given bytecode-compiled BASICfuck program.
// interpreter_bfmem_pointer (global) - the current BASICfuck memory pointer.
// interpreter_cmem_pointer (global) - the current computer memory pointer.
static void interpret(void) {
    opcode_t opcode   = 0;
    uint8_t  argument = 0;

    static const void *const jump_table[] = {
        &&lopcode_halt,        // OPCODE_HALT.
        &&lopcode_increment,   // OPCODE_INCREMENT.
        &&lopcode_decrement,   // OPCODE_DECREMENT.
        &&lopcode_bfmem_left,  // OPCODE_BFMEM_LEFT.
        &&lopcode_bfmem_right, // OPCODE_BFMEM_RIGHT.
        &&lopcode_print,       // OPCODE_PRINT.
        &&lopcode_input,       // OPCODE_INPUT.
        &&lopcode_jeq,         // OPCODE_JEQ.
        &&lopcode_jne,         // OPCODE_JNE.
        &&lopcode_cmem_read,   // OPCODE_CMEM_READ.
        &&lopcode_cmem_write,  // OPCODE_CMEM_WRITE.
        &&lopcode_cmem_left,   // OPCODE_CMEM_LEFT.
        &&lopcode_cmem_right,  // OPCODE_CMEM_RIGHT.
        &&lopcode_execute      // OPCODE_EXECUTE.
    };

    // Initialize interpreter.
    interpreter_program_pointer = program_memory;

    while (true) {
        if (0 != kbhit() && KEYBOARD_STOP == cgetc()) {
            puts("?ABORT");
            break;
        }

        opcode   = *interpreter_program_pointer;
        argument = interpreter_program_pointer[1];
        assert(opcode < ARRAY_SIZE(jump_table) && "unreachable");
        goto *jump_table[opcode];

lopcode_halt: {
            break;
        }

lopcode_increment: {
            *interpreter_bfmem_pointer += argument;
            goto lfinish_interpreter_cycle;
        }

lopcode_decrement: {
            *interpreter_bfmem_pointer -= argument;
            goto lfinish_interpreter_cycle;
        }

lopcode_bfmem_left: {
            if (interpreter_bfmem_pointer > basicfuck_memory + argument) {
                interpreter_bfmem_pointer -= argument;
            } else {
                interpreter_bfmem_pointer = basicfuck_memory;
            }
            goto lfinish_interpreter_cycle;
        }

lopcode_bfmem_right: {
            if (interpreter_bfmem_pointer + argument < basicfuck_memory_end) {
                interpreter_bfmem_pointer += argument;
            }
            goto lfinish_interpreter_cycle;
        }

lopcode_print: {
            putchar(*interpreter_bfmem_pointer);
            goto lfinish_interpreter_cycle;
        }

lopcode_input: {
            argument = wrappedCgetc();
            if (KEYBOARD_STOP == argument) {
                puts("?ABORT");
                break;
            };
            *interpreter_bfmem_pointer = argument;
            goto lfinish_interpreter_cycle;
        }

lopcode_jeq: {
            if (0 == *interpreter_bfmem_pointer) {
                interpreter_program_pointer =
                    *(opcode_t**)(interpreter_program_pointer + 1);
            }
            goto lfinish_interpreter_cycle;
        }

lopcode_jne: {
            if (0 != *interpreter_bfmem_pointer) {
                interpreter_program_pointer =
                    *(opcode_t**)(interpreter_program_pointer + 1);
            }
            goto lfinish_interpreter_cycle;
        }

lopcode_cmem_read: {
            *interpreter_bfmem_pointer = *interpreter_cmem_pointer;
            goto lfinish_interpreter_cycle;
        }

lopcode_cmem_write: {
            *interpreter_cmem_pointer = *interpreter_bfmem_pointer;
            goto lfinish_interpreter_cycle;
        }

lopcode_cmem_left: {
            if ((uint16_t)interpreter_cmem_pointer > argument) {
                interpreter_cmem_pointer -= argument;
            } else {
                interpreter_cmem_pointer = 0;
            }
            goto lfinish_interpreter_cycle;
        }

lopcode_cmem_right: {
            if (UINT16_MAX - (uint16_t)interpreter_cmem_pointer > argument) {
                interpreter_cmem_pointer += argument;
            } else {
                interpreter_cmem_pointer = (uint8_t*)UINT16_MAX;
            }
            goto lfinish_interpreter_cycle;
        }

lopcode_execute: {
            interpreter_register_a = *interpreter_bfmem_pointer;
            interpreter_register_x = interpreter_bfmem_pointer[1];
            interpreter_register_y = interpreter_bfmem_pointer[2];
            basicfuckExecute();
            *interpreter_bfmem_pointer   = interpreter_register_a;
            interpreter_bfmem_pointer[1] = interpreter_register_x;
            interpreter_bfmem_pointer[2] = interpreter_register_y;
            goto lfinish_interpreter_cycle;
        }

lfinish_interpreter_cycle: {
            // Jumped to after an opcode has been executed.
            interpreter_program_pointer += opcode_size_table[opcode];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// REPL                                                                       //
////////////////////////////////////////////////////////////////////////////////

static void helpMenu(void) {
    clrscr();
    puts(
        "REPL Commands (must be at start of line):\n"
        "\n"
        "! - Exits REPL.\n"
        "? - Displays this help menu.\n"
        "L - Displays license.\n"
        "# - Displays bytecode of last program.\n"
        "\n"
        "REPL Controls (Keypress):\n"
        "\n"
        KEYBOARD_STOP_STRING
        " - Cancel input and start new line like C-c.\n"
        KEYBOARD_HOME_STRING
        " - Move to start of line.\n"
        KEYBOARD_CLEAR_STRING
        " - Clear screen and line.\n"
        KEYBOARD_F1_STRING
        " - Previous history item.\n"
        KEYBOARD_F2_STRING
        " - Next history item.\n"
        "\n"
        KEYBOARD_STOP_STRING
        " - Abort BASICfuck program.\n"
        "\n"
        "Press ANY KEY to CONTINUE"
    );
    wrappedCgetc();

    clrscr();
    puts(
        "BASICfuck Instructions (Part 1):\n"
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
        "Press ANY KEY to CONTINUE"
    );
    wrappedCgetc();

    clrscr();
    puts(
        "BASICfuck Instructions (Part 2):\n"
        "\n"
        ") - Move to next location in computer memory.\n"
        "( - Move to previous location in computer memory.\n"
        "@ - Read value from computer memory into cell.\n"
        "* - Write value from cell into computer memory\n"
        "% - Execute location in computer memory as subroutine. The values of "
        "the current and next two cells will be used for the A, X, and Y "
        "registers. Resulting register values will be stored back into the "
        "respective cells.\n"
        "\n"
        "Press ANY KEY to CONTINUE"
    );
    wrappedCgetc();

    clrscr();
}

static void licenseMenu(void) {
    clrscr();
    puts(
        "Copyright (c) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi\n"
        "\n"
        "BASICfuck is free software: you can redistribute it and/or modify it "
        "under the terms of the GNU General Public License as published by the "
        "Free Software Foundation, either version 3 of the License, or (at "
        "your option) any later version.\n"
        "\n"
        "BASICfuck is distributed in the hope that it will be useful, but "
        "WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
        "General Public License for more details.\n"
        "\n"
        "Press ANY KEY to CONTINUE"
    );
    wrappedCgetc();

    clrscr();
    puts(
        "You should have received a copy of the GNU General Public License "
        "along with BASICfuck. If not, see <https://www.gnu.org/licenses/>.\n"
        "\n"
        "Source (paltepuk):\n"
        "Clearnet - https://paltepuk.xyz/cgit/BASICfuck.git/about/\n"
        "I2P - http://oytjumugnwsf4g72vemtamo72vfvgmp4lfsf6wmggcvba3qmcsta.b32.i2p/cgit/BASICfuck.git/about/\n"
        "Tor - http://4blcq4arxhbkc77tfrtmy4pptf55gjbhlj32rbfyskl672v2plsmjcyd.onion/cgit/BASICfuck.git/about/\n"
        "\n"
        "Source (GitHub):\n"
        "Clearnet - https://github.com/ona-li-toki-e-jan-Epiphany-tawa-mi/BASICfuck/\n"
        "\n"
        "Press ANY KEY to CONTINUE"
    );
    wrappedCgetc();

    clrscr();
}

#define SCREEN_BUFFER_SIZE 10
// Uses utoa() to convert the value to a string and prints it with leading zeros
// (no newline.)
// NOTE: the buffer used for this function has the size of SCREEN_BUFFER_SIZE,
// and does not check for overflows; be careful!
static void utoaFputs(
    const size_t digit_count,
    const uint16_t value,
    const uint8_t radix
) {
    static uint8_t string_buffer[SCREEN_BUFFER_SIZE] = {0};
    size_t         leading_zeros = 0;

    utoa(value, string_buffer, radix);

    if (0 != digit_count) {
        leading_zeros = digit_count - strlen(string_buffer);
        for (; leading_zeros > 0; --leading_zeros) fputs("0", stdout);
    }

    fputs(string_buffer, stdout);
};

// Displays a readout of the bytecode of the last program to the user.
// Holding space will slow down the printing.
// program_memory (global) - the program buffer.
static void displayBytecode(void) {
    uint8_t i = 0;

    // Ideally display 16 bytes at a time, but screen real estate is what it is.
    uint8_t bytes_per_line = (width - 7) / 3;
    bytes_per_line = bytes_per_line > 16 ? 16 : bytes_per_line;

    while (true) {
        if (i % bytes_per_line == 0) {
            // Slow down while holding space.
            if (kbhit() != 0 && cgetc() == ' ')
                sleep(1);

            // Prints addresses.
            fputs("\n$", stdout);
            utoaFputs(4, (uint16_t)program_memory + i, 16);
            putchar(':');
        }
        // Prints values.
        putchar(' ');
        utoaFputs(2, program_memory[i], 16);

        if (i >= PROGRAM_MEMORY_SIZE - 1) break;
        ++i;
    }

    putchar('\n');
}

int main(void) {
    // Initializes global screen size variables in screen.h.
    screensize(&width, &height);
    // Initializes the opcode table in basicfuck.h.
    initializeInstructionOpcodeTable();

    clrscr();
    puts("BASICfuck REPL 0.2.0\n");
    utoaFputs(0, BASICFUCK_MEMORY_SIZE, 10);
    puts(
        " CELLS FREE\n"
        "\n"
        "Enter '?' for HELP\n"
        "Enter '!' to EXIT\n"
    );

    while (true) {
        // Read.
        fputs("YOUR WILL? ", stdout);
        editEditBuffer();

        switch (edit_buffer[0]) {
        case '\0': {
            // empty input.
            continue;
        }
        case '!': {
            puts("SO BE IT.");
            goto lexit_repl;
        }
        case '?': {
            helpMenu();
            continue;
        }
        case 'L': {
            licenseMenu();
            continue;
        }
        case '#': {
            displayBytecode();
            continue;
        }
        default: {
            break;
        }
        }

        // Evaluate.
        if (!compileFirstPass()) {
            puts("?OUT OF MEMORY");
            continue;
        }
        if (!compileSecondPass()) {
            puts("?UNTERMINATED LOOP");
            continue;
        }
        interpret();

        // Print.
        utoaFputs(3, *interpreter_bfmem_pointer, 10);
        fputs(" (Cell ", stdout);
        utoaFputs(
            5,
            (uint16_t)(interpreter_bfmem_pointer - basicfuck_memory)
            , 10
        );
        fputs(", Memory $", stdout);
        utoaFputs(4, (uint16_t)interpreter_cmem_pointer, 16);
        puts(")");
    }
lexit_repl:

    return 0;
}
