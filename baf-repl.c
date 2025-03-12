/*
 * This file is part of Brainblast-Toolkit.
 *
 * Copyright (c) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi
 *
 * Brainblast-Toolkit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * Brainblast-Toolkit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Brainblast-Toolkit. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * BASICfuck Read-Evaluate-Print Loop.
 *
 * Preprocessor parameters:
 *  - BASICFUCK_MEMORY_SIZE - The number of BASICfuck cells (bytes) to allocate.
 *  - TOOLKIT_VERSION - The version string for Brainblast-Toolkit.
 *  - HISTORY_STACK_SIZE - The size, in bytes, of the history stack.
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
#  if defined( __C16__) || defined( __C64__) || defined(__C128__) || defined(__VIC20__) || defined(__CBM510__) || defined(__CBM610__) || defined(__CX16__)
#    define KEYBOARD_F1        CH_F1
#    define KEYBOARD_F1_STRING "F1"
#    define KEYBOARD_F2        CH_F2
#    define KEYBOARD_F2_STRING "F2"
#  elif defined(__PET__) // __C16__ || __C64__ || __C128__ || __VIC20__ || __CBM510__ || __CBM610__ || __CX16__
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
// Screen                                                                     //
////////////////////////////////////////////////////////////////////////////////

// The width and height of the screen. Must be initalized at some point with
// screensize(), or some other method, else they will be set to 0.
static uint8_t s_width  = 0;
static uint8_t s_height = 0;

// On some machines, cgetc() doesn't block like expected, and instead returns
// immediately. This version adds in a check to ensure the blocking behavior.
static uint8_t s_wrapped_cgetc() {
    uint8_t character = 0;
    do {
        character = cgetc();
    } while ('\0' == character);

    return character;
}

// Runs s_wrapped_cgetc() with a blinking cursor.
// To set a blinking cursor (easily,) you need to use the cursor() function from
// conio.h, but it seems to error with "Illegal function call" or something when
// used in a complex function, so I have it pulled out into this separate one.
static uint8_t s_blinking_cgetc() {
    uint8_t character = 0;

    cursor(true);
    character = s_wrapped_cgetc();
    cursor(false);

    return character;
}

#define SCREEN_BUFFER_SIZE 6
// Uses utoa() to convert the value to a string and prints it with leading zeros
// (no newline.)
// NOTE: that the buffer used for this function has the size of
// SCREEN_BUFFER_SIZE, and does not check for overflows; be careful!
// digit_count - the number of digits to print. If the resulting number has less
// than this number of digits it will be prepended with zeros. A value of 0
// disables this and simply prints the number.
// radix - the base to use to generate the number string.
static void s_utoa_fputs(const size_t digit_count, const uint16_t value, const uint8_t radix) {
    static uint8_t string_buffer[SCREEN_BUFFER_SIZE];
    size_t         leading_zeros = 0;

    utoa(value, string_buffer, radix);

    if (0 != digit_count) {
        leading_zeros = digit_count - strlen(string_buffer);
        for (; leading_zeros > 0; --leading_zeros)
            fputs("0", stdout);
    }

    fputs(string_buffer, stdout);
};

// Returns whether the given character is a screen control character.
static bool s_is_control_character(const uint8_t character) {
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

////////////////////////////////////////////////////////////////////////////////
// Text Buffers                                                               //
////////////////////////////////////////////////////////////////////////////////

// Global variables for passing parameters between functions.
// The buffer currently being edited.
static uint8_t* tb_buffer;
// The location of the user's cursor inside the buffer.
static uint8_t tb_cursor;
// How much of the buffer is taken up by the text typed by the user.
static uint8_t tb_input_size;

// History stack state.
static uint8_t* tb_history_stack;
static uint16_t tb_history_stack_size;
static uint16_t tb_history_stack_index;

// Increments the history stack index and loops it around if it goes out of
// bounds.
// tb_history_stack_index (global) - current index into the history stack.
// tb_history_stack_size (global) - the size of the history stack.
static void tb_increment_stack_index() {
    ++(tb_history_stack_index);
    if (tb_history_stack_index >= tb_history_stack_size)
        tb_history_stack_index = 0;
}

// Decrements the history stack index and loops it around if it goes out of
// bounds.
// tb_history_stack_index (global) - current index into the history stack.
// tb_history_stack_size (global) - the size of the history stack.
static void tb_decrement_stack_index() {
    if (0 == tb_history_stack_index) {
        tb_history_stack_index = tb_history_stack_size - 1;
    } else {
        --(tb_history_stack_index);
    }
}

// Saves the given null-terminated text buffer to the history stack for later
// recollection.
// tb_current_buffer (global) - the null-terminated buffer to save.
// tb_history_stack (global) - pointer to the history stack.
// tb_history_stack_index (global) - current index into the history stack.
static void tb_save_buffer() {
    uint8_t character;
    uint8_t buffer_index = 0;

    if (NULL == tb_buffer[buffer_index])
        return;

    do {
        character = tb_buffer[buffer_index];
        tb_history_stack[tb_history_stack_index] = character;

        tb_increment_stack_index();
        ++buffer_index;
    } while (NULL != character);
}

// Recalls the previous input if foward_recall is false, else recalls the next
// input from the history buffer.
// tb_current_buffer (global) - the null-terminated buffer to write to.
// tb_buffer_cursor (global) - the user's cursor inside the buffer.
// tb_input_size (global) - the amount of space used inside the buffer.
// tb_forward_recall - which direction to move in in the history buffer.
// tb_history_stack (global) - pointer to the history stack.
// tb_history_stack_index (global) - current index into the history stack.
static void tb_recall_buffer(const bool forward_recall) {
    uint8_t  character;
    uint16_t final_history_index = tb_history_stack_index;

    // Moves forwards or backwards to the next block in the history stack.
    if (forward_recall) {
        while (NULL != tb_history_stack[tb_history_stack_index])
            tb_increment_stack_index();
        tb_increment_stack_index();

    } else {
        tb_decrement_stack_index();
        do {
            tb_decrement_stack_index();
        } while (NULL != tb_history_stack[tb_history_stack_index]);
        tb_increment_stack_index();
    }

    // If a NULL is found at the next block, that means that it's at the end of
    // the history buffer.
    if (NULL == tb_history_stack[tb_history_stack_index]) {
        tb_history_stack_index = final_history_index;
        return;
    }
    // Preservers the index of this block as we will stay here in case of
    // succesive movements through the history.
    final_history_index = tb_history_stack_index;

    // Navigates visual cursor to the end of the buffer.
    for (; tb_cursor < tb_input_size; ++tb_cursor)
        putchar(KEYBOARD_RIGHT);
    // Clears visual buffer.
    while (tb_cursor > 0) {
        putchar(KEYBOARD_BACKSPACE);
        --tb_cursor;
    }

    // Reads from history buffer into buffer.
    while (true) {
        character            = tb_history_stack[tb_history_stack_index];
        tb_buffer[tb_cursor] = character;
        putchar(character);

        if (NULL == character)
            break;

        tb_increment_stack_index();
        ++tb_cursor;
    }
    tb_input_size = tb_cursor;

    // Restores history stack index to the start of the current block so that
    // moving forwards and backwords works properly.
    tb_history_stack_index = final_history_index;
}

// Creates an editable text buffer, starting from the current position on the
// screen, and stores what the user typed into the given buffer with a
// null-terminator.
// The cursor on the screen will be moved to the line after the filled portion
// of the text buffer once done.
// buffer - the buffer to store the typed characters into.
// buffer_max_index - the maxiumum addressable index of the buffer.
// tb_history_stack (global) - the stack to store previous user inputs in.
// tb_history_stack_size (global) - the size of the history stack.
// tb_history_stack_index (global) - a pointer to the current index into the
// stack history.
static void tb_edit_buffer(uint8_t *const buffer, uint8_t buffer_max_index) {
    uint8_t new_cursor;
    uint8_t key;

    // Ensures the last byte is reserved for a null-terminator.
    buffer_max_index -= 1;

    // Loads parameters into global variables for this and other functions.
    tb_buffer     = buffer;
    tb_cursor     = 0;
    tb_input_size = 0;

    while (true) {
        key = s_blinking_cgetc();

        switch (key) {
        // Finalizes the buffer and exits from this function.
        case KEYBOARD_ENTER:
            tb_buffer[tb_input_size] = NULL;               // write out null-terminator.
            for (; tb_cursor < tb_input_size; ++tb_cursor) // navigate to then end of buffer if neccesary.
                putchar(KEYBOARD_RIGHT);
            putchar('\n');

            goto lquit_editing_buffer;

        // "Clears" the input buffer and exits from this function.
        case KEYBOARD_STOP:
            tb_buffer[0] = NULL;                  // write null terminator to start of buffer, "clearing" it.
            putchar('\n');

            goto lquit_editing_buffer;

        // Clears the screen and input buffer.
        case KEYBOARD_CLEAR:
            tb_cursor     = 0;
            tb_input_size = 0;
            clrscr();
            break;

        // Deletes characters from the buffer.
        case KEYBOARD_BACKSPACE:
            if (tb_cursor == 0)
                break;

            putchar(KEYBOARD_BACKSPACE);    // display backspace.
            // Shifts characters in buffer to the left, overwiting the deleted
            // character.
            memmove(tb_buffer+tb_cursor - 1, tb_buffer+tb_cursor, tb_input_size - tb_cursor);
            --tb_input_size;
            --tb_cursor;

            break;

        // Handles arrow keys, moving through the buffer.
        case KEYBOARD_LEFT:
            if (tb_cursor > 0) {
                --tb_cursor;
                putchar(KEYBOARD_LEFT);
            }
            break;

        case KEYBOARD_RIGHT:
            if (tb_cursor < tb_input_size) {
                ++tb_cursor;
                putchar(KEYBOARD_RIGHT);
            }
            break;

        case KEYBOARD_UP:
            // Navigates to the next line up, or to the start of the buffer, if
            // there is no line there.
            new_cursor = tb_cursor > s_width ? tb_cursor - s_width : 0;
            for (; tb_cursor > new_cursor; --tb_cursor)
                putchar(KEYBOARD_LEFT);

            break;

        case KEYBOARD_DOWN:
            // Navigates to the next line down, or to the end of the filled
            // buffer, if there is no line there.
            new_cursor = (tb_input_size - tb_cursor) > s_width ? tb_cursor + s_width : tb_input_size;
            for (; tb_cursor < new_cursor; ++tb_cursor)
                putchar(KEYBOARD_RIGHT);

            break;

        // Handles HOME, moving to the start of the buffer.
        case KEYBOARD_HOME:
            for (; tb_cursor > 0; --tb_cursor)
                putchar(KEYBOARD_LEFT);
            break;

        // Handles INST, inserting characters into the buffer.
        case KEYBOARD_INSERT:
            if (tb_input_size > buffer_max_index || tb_cursor == tb_input_size)
                break;

            putchar(KEYBOARD_INSERT);       // display insertion.
            // Shifts characters in buffer to the right, making space for the
            // new one.
            memmove(tb_buffer+tb_cursor + 1, tb_buffer+tb_cursor, tb_input_size - tb_cursor);
            tb_input_size++;
            tb_buffer[tb_cursor] = ' ';

            break;

        // Handles function keys, navigating through the history buffer.
        case KEYBOARD_F1:
            tb_recall_buffer(false);
            break;

        case KEYBOARD_F2:
            tb_recall_buffer(true);
            break;

        // Handles typing characters.
        default:
            if (s_is_control_character(key))      // filter out unhandled control characters.
                break;
            if (tb_cursor > buffer_max_index)
                break;

            if (tb_cursor == tb_input_size)      // increase buffer size if adding characters to the end.
                ++tb_input_size;
            tb_buffer[tb_cursor] = key;
            ++tb_cursor;
            putchar(key);
        }
    }
lquit_editing_buffer:

    tb_save_buffer();
    return;
}

////////////////////////////////////////////////////////////////////////////////
// BASICfuck                                                                  //
////////////////////////////////////////////////////////////////////////////////

typedef uint8_t baf_cell_t;

typedef uint8_t baf_opcode_t;
// Ends the current BASICfuck program.
#define BAF_OPCODE_HALT 0x00
// Increments the current cell.
// argument1 - the amount to increment by.
#define BAF_OPCODE_INCREMENT 0x01
// Decrements the current cell.
// argument1 - the amount to decrement by.
#define BAF_OPCODE_DECREMENT 0x02
// Moves the the cell pointer to the left.
// argument1 - the number of times to move to the left.
#define BAF_OPCODE_BFMEM_LEFT 0x03
// Moves the the cell pointer to the right.
// argument1 - the number of times to move to the right.
#define BAF_OPCODE_BFMEM_RIGHT 0x04
// Prints the value in the current cell as PETSCII character.
#define BAF_OPCODE_PRINT 0x05
// Awaits a value from the keyboard and stores it in the current cell.
#define BAF_OPCODE_INPUT 0x06
// Jumps to the given address if the current cell is 0.
// argument1,2 - the address in program memory to jump to.
#define BAF_OPCODE_JEQ 0x07
// Jumps to the given address if the current cell is not 0.
// argument1,2 - the address in program memory to jump to.
#define BAF_OPCODE_JNE 0x08
// Reads the value at the computer memory pointer into the current cell.
#define BAF_OPCODE_CMEM_READ 0x09
// Writes the value in the current cell to the location at the computer memory
// pointer.
#define BAF_OPCODE_CMEM_WRITE 0x0A
// Moves the computer memory pointer to the left.
// argument1 - the number of times to move to the left.
#define BAF_OPCODE_CMEM_LEFT 0x0B
// Moves the computer memory pointer to the right.
// argument1 - the number of times to move to the right.
#define BAF_OPCODE_CMEM_RIGHT 0x0C
// Runs the subroutine at the computer memory pointer with the current and next
// two cells as the values for the X, Y, and Z registers.
#define BAF_OPCODE_EXECUTE 0x0D

typedef uint8_t BAFCompileResult;
#define BAF_COMPILE_SUCCESS           0
#define BAF_COMPILE_OUT_OF_MEMORY     1
#define BAF_COMPILE_UNTERMINATED_LOOP 2

typedef struct {
    // Buffer to read BASICfuck code from.
    const uint8_t* read_buffer;
    // Buffer to write bytecode to.
    baf_opcode_t* write_buffer;
    uint16_t      write_buffer_size;
} BAFCompiler;

typedef struct {
    const baf_opcode_t* program_buffer;
    baf_cell_t*         basicfuck_cell_pointer;
    baf_cell_t*         basicfuck_cell_start_pointer;
    baf_cell_t*         basicfuck_cell_end_pointer;
    uint8_t*            computer_memory_pointer;
} BAFInterpreter;

// A table mapping from opcodes to their size (opcode + arguments) in bytes.
// Index value must be valid opcode.
static const uint8_t baf_opcode_size_table[] = {
    1, // BAF_OPCODE_HALT.
    2, // BAF_OPCODE_INCREMENT.
    2, // BAF_OPCODE_DECREMENT.
    2, // BAF_OPCODE_BFMEM_LEFT.
    2, // BAF_OPCODE_BFMEM_RIGHT.
    1, // BAF_OPCODE_PRINT.
    1, // BAF_OPCODE_INPUT.
    3, // BAF_OPCODE_JEQ.
    3, // BAF_OPCODE_JNE.
    1, // BAF_OPCODE_CMEM_READ.
    1, // BAF_OPCODE_CMEM_WRITE.
    2, // BAF_OPCODE_CMEM_LEFT.
    2, // BAF_OPCODE_CMEM_RIGHT.
    1  // BAF_OPCODE_EXECUTE.
};

// A table mapping from instruction characters to their corresponding opcodes.
// Index value must not exceed 255.
// Must call baf_initialize_instruction_opcode_table() once prior to use.
// If the given instruction does not have an opcode, 0xFF will be returned.
static baf_opcode_t baf_instruction_opcode_table[256];

// A one-time-call function used to initialize instruction_opcode_table[].
static void baf_initialize_instruction_opcode_table() {
    uint8_t i = 0;
    for (; i < 255; i++)
        baf_instruction_opcode_table[i] = 0xFF;

    baf_instruction_opcode_table[NULL] = BAF_OPCODE_HALT;
    baf_instruction_opcode_table['+']  = BAF_OPCODE_INCREMENT;
    baf_instruction_opcode_table['-']  = BAF_OPCODE_DECREMENT;
    baf_instruction_opcode_table['<']  = BAF_OPCODE_BFMEM_LEFT;
    baf_instruction_opcode_table['>']  = BAF_OPCODE_BFMEM_RIGHT;
    baf_instruction_opcode_table['.']  = BAF_OPCODE_PRINT;
    baf_instruction_opcode_table[',']  = BAF_OPCODE_INPUT;
    baf_instruction_opcode_table['[']  = BAF_OPCODE_JEQ;
    baf_instruction_opcode_table[']']  = BAF_OPCODE_JNE;
    baf_instruction_opcode_table['@']  = BAF_OPCODE_CMEM_READ;
    baf_instruction_opcode_table['*']  = BAF_OPCODE_CMEM_WRITE;
    baf_instruction_opcode_table['(']  = BAF_OPCODE_CMEM_LEFT;
    baf_instruction_opcode_table[')']  = BAF_OPCODE_CMEM_RIGHT;
    baf_instruction_opcode_table['%']  = BAF_OPCODE_EXECUTE;
}

// Compiler state.
static const uint8_t* baf_compiler_read_pointer      = NULL; // Pointer to the current position in the read buffer.
static baf_opcode_t*  baf_compiler_write_pointer     = NULL; // Pointer to the current position in the write buffer.
static baf_opcode_t*  baf_compiler_write_end_pointer = NULL; // Pointer to one after the end of the write buffer.

// Performs the first pass of BASICfuck compilation, converting the text program
// to opcodes.
// baf_compiler_read_pointer (global) - set to the start of the read buffer.
// Gets clobbered.
// baf_compiler_write_pointer (global) - set to the start of the write buffer.
// Gets clobbered.
// baf_compiler_write_end_pointer (global).
// true if succeeded, false if ran out of memory.
static bool baf_compile_first_pass() {
    uint8_t      instruction = 0;
    baf_opcode_t opcode      = 0;

    // Used by counted instructions.
    uint16_t instruction_count = 0;
    uint8_t  other_instruction = 0;
    uint8_t  chunk_count       = 0;

    static const void *const jump_table[] = {
        &&lfinish_bytecode_compilation,           // BAF_OPCODE_HALT.
        &&lcompile_counted_instruction,           // BAF_OPCODE_INCREMENT.
        &&lcompile_counted_instruction,           // BAF_OPCODE_DECREMENT.
        &&lcompile_counted_instruction,           // BAF_OPCODE_BFMEM_LEFT.
        &&lcompile_counted_instruction,           // BAF_OPCODE_BFMEM_RIGHT.
        &&lcompile_instruction_no_arugments,      // BAF_OPCODE_PRINT.
        &&lcompile_instruction_no_arugments,      // BAF_OPCODE_INPUT.
        &&lcompile_jump_instruction,              // BAF_OPCODE_JEQ.
        &&lcompile_jump_instruction,              // BAF_OPCODE_JNE.
        &&lcompile_instruction_no_arugments,      // BAF_OPCODE_CMEM_READ.
        &&lcompile_instruction_no_arugments,      // BAF_OPCODE_CMEM_WRITE.
        &&lcompile_counted_instruction,           // BAF_OPCODE_CMEM_LEFT.
        &&lcompile_counted_instruction,           // BAF_OPCODE_CMEM_RIGHT.
        &&lcompile_instruction_no_arugments       // BAF_OPCODE_EXECUTE.
    };

    while (true) {
        instruction = *baf_compiler_read_pointer;
        opcode      = baf_instruction_opcode_table[instruction];

        // Ignores non-instructions.
        if (opcode == 0xFF) {
            ++baf_compiler_read_pointer;
            continue;
        }

        goto *jump_table[opcode];

        // End of program.
lfinish_bytecode_compilation:
        *baf_compiler_write_pointer = BAF_OPCODE_HALT;
        break;

        // Takes no arguments.
lcompile_instruction_no_arugments:
        if (baf_compiler_write_pointer >= baf_compiler_write_end_pointer)
            return false;

        *(baf_compiler_write_pointer++) = opcode;
        ++baf_compiler_read_pointer;

        continue;

        // Takes a 16-bit address relative to program memory as a parameter,
        // which will be handled by the second pass.
lcompile_jump_instruction:
        if (baf_compiler_write_pointer + 2 >= baf_compiler_write_end_pointer)
            return false;

        *(baf_compiler_write_pointer++) = opcode;
        *(baf_compiler_write_pointer++) = 0xFF;
        *(baf_compiler_write_pointer++) = 0xFF;
        ++baf_compiler_read_pointer;

        continue;

        // Takes an 8-bit count of how many times to preform the operation.
lcompile_counted_instruction:
        instruction_count = 0;

        // Count number of consecutive instructions.
        while (true) {
            other_instruction = *baf_compiler_read_pointer;

            if (other_instruction != instruction)
                break;

            ++instruction_count;
            ++baf_compiler_read_pointer;
        }

        // Each instruction opcode can only take an 8-bit value, so this chops up
        // the full count into separate 8-bit chunks.
        while (instruction_count > 0) {
            if (baf_compiler_write_pointer + 1 >= baf_compiler_write_end_pointer)
                return false;

            chunk_count = instruction_count > 255 ? 255 : (uint8_t)instruction_count;

            *(baf_compiler_write_pointer++) = opcode;
            *(baf_compiler_write_pointer++) = chunk_count;

            instruction_count -= (uint16_t)chunk_count;
        }

        continue;
    }

    return true;
}

// Performs the second pass of BASICfuck compilation, calculating the addresses
// for jump instructions.
// baf_compiler_write_pointer (global) - set to the start of the write buffer.
// Gets clobbered.
// Returns true if succeeded, false if there is an unterminated loop.
static bool baf_compile_second_pass() {
    baf_opcode_t* write_start_pointer = baf_compiler_write_pointer;
    baf_opcode_t  opcode = 0;
    uint16_t      loop_depth = 0;

    baf_opcode_t* seek_pointer  = NULL;
    baf_opcode_t  seeked_opcode = 0;

    while (BAF_OPCODE_HALT != (opcode = *baf_compiler_write_pointer)) {
        switch (opcode) {
        case BAF_OPCODE_JEQ:
            seek_pointer = baf_compiler_write_pointer + baf_opcode_size_table[BAF_OPCODE_JEQ];
            loop_depth   = 1;

            // Finds and links with accomanying JNE instruction.
            while (BAF_OPCODE_HALT != (seeked_opcode = *seek_pointer)) {
                switch (seeked_opcode) {
                case BAF_OPCODE_JEQ:
                    ++loop_depth;
                    break;
                case BAF_OPCODE_JNE:
                    --loop_depth;
                    break;
                }

                if (loop_depth == 0) {
                    // Sets JEQ instruction to jump to accomanying JNE.
                    *(baf_opcode_t**)(baf_compiler_write_pointer + 1) = seek_pointer;
                    // And vice-versa.
                    *(baf_opcode_t**)(seek_pointer + 1) = baf_compiler_write_pointer;

                    break;
                }

                seek_pointer += baf_opcode_size_table[seeked_opcode];
            }

            if (loop_depth != 0)
                return false;

            break;

        case BAF_OPCODE_JNE:
            // Address should have been set by some preceeding JEQ instruction.
            if (0xFFFF == *(uint16_t*)(baf_compiler_write_pointer + 1))
                return false;

            break;
        }

        baf_compiler_write_pointer += baf_opcode_size_table[opcode];
    }


    return true;
}

// Bytecode compiles BASICfuck code.
// Returns BAF_COMPILE_SUCCESS on success, BAF_COMPILE_OUT_OF_MEMORY if the
// program exceeded the size of the program memory,
// BAF_COMPILE_UNTERMINATED_LOOP if the program has an unterminated loop.
static BAFCompileResult baf_compile(const BAFCompiler *const compiler) {
    baf_compiler_read_pointer      = compiler->read_buffer;
    baf_compiler_write_pointer     = compiler->write_buffer;
    // The last location is reserved for end of program.
    baf_compiler_write_end_pointer = compiler->write_buffer + compiler->write_buffer_size - 1;
    if (!baf_compile_first_pass())
        return BAF_COMPILE_OUT_OF_MEMORY;

    baf_compiler_write_pointer = compiler->write_buffer;
    if (!baf_compile_second_pass())
        return BAF_COMPILE_UNTERMINATED_LOOP;

    return BAF_COMPILE_SUCCESS;
}

// Interpreter state.
static const baf_opcode_t* baf_interpreter_program_pointer     = NULL;
static baf_cell_t*         baf_interpreter_bfmem_pointer       = NULL;
static baf_cell_t*         baf_interpreter_bfmem_start_pointer = NULL;
static baf_cell_t*         baf_interpreter_bfmem_end_pointer   = NULL;
static uint8_t*            baf_interpreter_cmem_pointer        = NULL;

// Global variables for exchaning values with inline assembler.
static uint8_t baf_interpreter_register_a = 0;
static uint8_t baf_interpreter_register_x = 0;
static uint8_t baf_interpreter_register_y = 0;

// Runs the execute part of the BASICfuck execute instruction.
// baf_interpreter_register_a (global) - the value to place in the A register.
// baf_interpreter_register_x (global) - the value to place in the X register.
// baf_interpreter_register_y (global) - the value to place in the Y register.
// baf_interpreter_cmem_pointer (global) - the address to execute as a
// subroutine.
static void baf_execute() {
    // Overwrites address of subroutine to call in next assembly block with the
    // computer memory pointer's value.
    __asm__ volatile ("lda     %v",   baf_interpreter_cmem_pointer);
    __asm__ volatile ("sta     %g+1", ljump_instruction);
    __asm__ volatile ("lda     %v+1", baf_interpreter_cmem_pointer);
    __asm__ volatile ("sta     %g+2", ljump_instruction);
    // Executes subroutine.
    __asm__ volatile ("lda     %v",   baf_interpreter_register_a);
    __asm__ volatile ("ldx     %v",   baf_interpreter_register_x);
    __asm__ volatile ("ldy     %v",   baf_interpreter_register_y);
ljump_instruction:
    __asm__ volatile ("jsr     %w",   NULL);
    // Retrieves resuting values.
    __asm__ volatile ("sta     %v",   baf_interpreter_register_a);
    __asm__ volatile ("stx     %v",   baf_interpreter_register_x);
    __asm__ volatile ("sty     %v",   baf_interpreter_register_y);

    return;
    // If we don't include a jmp instruction, cc65, annoyingly, strips the label
    // from the resulting assembly.
    __asm__ volatile ("jmp     %g", ljump_instruction);
}

// Runs the interpreter with the given bytecode-compiled BASICfuck program,
// leaving the given starting state off wherever it the program finished at.
// interpreter - interpreter state.
static void baf_interpret(BAFInterpreter *const interpreter) {
    baf_opcode_t opcode   = 0;
    uint8_t      argument = 0;

    static const void *const jump_table[] = {
        &&lopcode_halt,        // BAF_OPCODE_HALT.
        &&lopcode_increment,   // BAF_OPCODE_INCREMENT.
        &&lopcode_decrement,   // BAF_OPCODE_DECREMENT.
        &&lopcode_bfmem_left,  // BAF_OPCODE_BFMEM_LEFT.
        &&lopcode_bfmem_right, // BAF_OPCODE_BFMEM_RIGHT.
        &&lopcode_print,       // BAF_OPCODE_PRINT.
        &&lopcode_input,       // BAF_OPCODE_INPUT.
        &&lopcode_jeq,         // BAF_OPCODE_JEQ.
        &&lopcode_jne,         // BAF_OPCODE_JNE.
        &&lopcode_cmem_read,   // BAF_OPCODE_CMEM_READ.
        &&lopcode_cmem_write,  // BAF_OPCODE_CMEM_WRITE.
        &&lopcode_cmem_left,   // BAF_OPCODE_CMEM_LEFT.
        &&lopcode_cmem_right,  // BAF_OPCODE_CMEM_RIGHT.
        &&lopcode_execute      // BAF_OPCODE_EXECUTE.
    };

    // Loads interpeter settings into globals for faster access.
    baf_interpreter_program_pointer     = interpreter->program_buffer;
    baf_interpreter_bfmem_pointer       = interpreter->basicfuck_cell_pointer;
    baf_interpreter_bfmem_start_pointer = interpreter->basicfuck_cell_start_pointer;
    baf_interpreter_bfmem_end_pointer   = interpreter->basicfuck_cell_end_pointer;
    baf_interpreter_cmem_pointer        = interpreter->computer_memory_pointer;


    while (true) {
        if (0 != kbhit() && KEYBOARD_STOP == cgetc()) {
            puts("?ABORT");
            break;
        }


        opcode   = *baf_interpreter_program_pointer;
        argument = baf_interpreter_program_pointer[1];
        assert(opcode < sizeof(jump_table)/sizeof(jump_table[0]) && "Unknown opcode");
        goto *jump_table[opcode];

lopcode_halt:
        break;

lopcode_increment:
        *baf_interpreter_bfmem_pointer += argument;
        goto lfinish_interpreter_cycle;

lopcode_decrement:
        *baf_interpreter_bfmem_pointer -= argument;
        goto lfinish_interpreter_cycle;

lopcode_bfmem_left:
        if (baf_interpreter_bfmem_pointer > baf_interpreter_bfmem_start_pointer + argument) {
            baf_interpreter_bfmem_pointer -= argument;
        } else {
            baf_interpreter_bfmem_pointer = baf_interpreter_bfmem_start_pointer;
        }
        goto lfinish_interpreter_cycle;

lopcode_bfmem_right:
        if (baf_interpreter_bfmem_pointer + argument < baf_interpreter_bfmem_end_pointer)
            baf_interpreter_bfmem_pointer += argument;
        goto lfinish_interpreter_cycle;

lopcode_print:
        putchar(*baf_interpreter_bfmem_pointer);
        goto lfinish_interpreter_cycle;

lopcode_input:
        argument = s_wrapped_cgetc();
        if (KEYBOARD_STOP == argument) {
            puts("?ABORT");
            break;
        };
        *baf_interpreter_bfmem_pointer = argument;
        goto lfinish_interpreter_cycle;

lopcode_jeq:
        if (0 == *baf_interpreter_bfmem_pointer) {
            baf_interpreter_program_pointer =
                *(baf_opcode_t**)(baf_interpreter_program_pointer + 1);
        }
        goto lfinish_interpreter_cycle;

lopcode_jne:
        if (0 != *baf_interpreter_bfmem_pointer) {
            baf_interpreter_program_pointer =
                *(baf_opcode_t**)(baf_interpreter_program_pointer + 1);
        }
        goto lfinish_interpreter_cycle;

lopcode_cmem_read:
        *baf_interpreter_bfmem_pointer = *baf_interpreter_cmem_pointer;
        goto lfinish_interpreter_cycle;

lopcode_cmem_write:
        *baf_interpreter_cmem_pointer = *baf_interpreter_bfmem_pointer;
        goto lfinish_interpreter_cycle;

lopcode_cmem_left:
        if ((uint16_t)baf_interpreter_cmem_pointer > argument) {
            baf_interpreter_cmem_pointer -= argument;
        } else {
            baf_interpreter_cmem_pointer = 0;
        }
        goto lfinish_interpreter_cycle;

lopcode_cmem_right:
        if (UINT16_MAX - (uint16_t)baf_interpreter_cmem_pointer > argument) {
            baf_interpreter_cmem_pointer += argument;
        } else {
            baf_interpreter_cmem_pointer = (uint8_t*)UINT16_MAX;
        }
        goto lfinish_interpreter_cycle;

lopcode_execute:
        baf_interpreter_register_a = *baf_interpreter_bfmem_pointer;
        baf_interpreter_register_x = baf_interpreter_bfmem_pointer[1];
        baf_interpreter_register_y = baf_interpreter_bfmem_pointer[2];
        baf_execute();
        *baf_interpreter_bfmem_pointer   = baf_interpreter_register_a;
        baf_interpreter_bfmem_pointer[1] = baf_interpreter_register_x;
        baf_interpreter_bfmem_pointer[2] = baf_interpreter_register_y;
        goto lfinish_interpreter_cycle;


lfinish_interpreter_cycle:
        // Jumped to after an opcode has been executed.
        baf_interpreter_program_pointer += baf_opcode_size_table[opcode];
    }

    // Writes back mutable interpreter state.
    interpreter->basicfuck_cell_pointer       = baf_interpreter_bfmem_pointer;
    interpreter->basicfuck_cell_start_pointer = baf_interpreter_bfmem_start_pointer;
    interpreter->basicfuck_cell_end_pointer   = baf_interpreter_bfmem_end_pointer;
    interpreter->computer_memory_pointer      = baf_interpreter_cmem_pointer;
}

////////////////////////////////////////////////////////////////////////////////
// REPL                                                                       //
////////////////////////////////////////////////////////////////////////////////

static void help_menu() {
    clrscr();
    puts(
        "REPL Commands (must be at start of line):\n"
        "\n"
        "! - Exits REPL.\n"
        "? - Displays this help menu.\n"
        "# - Displays bytecode of last program.\n"
        "\n"
        "REPL Controls (Keypress):\n"
        "\n"
        KEYBOARD_STOP_STRING " - Cancel input and start new line like C-c.\n"
        KEYBOARD_HOME_STRING " - Move to start of line.\n"
        KEYBOARD_CLEAR_STRING " - Clear screen and line.\n"
        KEYBOARD_F1_STRING " - Previous history item.\n"
        KEYBOARD_F2_STRING " - Next history item.\n"
        "\n"
        KEYBOARD_STOP_STRING " - Abort BASICfuck program.\n"
        "\n"
        "Press ANY KEY to CONTINUE"
    );
    s_wrapped_cgetc();

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
    s_wrapped_cgetc();

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
    s_wrapped_cgetc();

    clrscr();
}

// Memory for the compiled bytecode of entered BASICfuck code.
#define PROGRAM_MEMORY_SIZE 256
static baf_opcode_t program_memory[PROGRAM_MEMORY_SIZE];

// Displays a readout of the bytecode of the last program to the user.
// Holding space will slow down the printing.
// program_memory (global) - the program buffer.
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
            fputs("\n$", stdout);
            s_utoa_fputs(4, (uint16_t)program_memory + i, 16);
            putchar(':');
        }
        // Prints values.
        putchar(' ');
        s_utoa_fputs(2, program_memory[i], 16);

        if (i >= PROGRAM_MEMORY_SIZE - 1)
            break;
        ++i;
    }

    putchar('\n');
}

#define INPUT_BUFFER_SIZE 256

int main(void) {
    static baf_cell_t BASICfuck_memory[BASICFUCK_MEMORY_SIZE];
    BAFCompiler       compiler;
    BAFInterpreter    interpreter;

    static uint8_t input_buffer[INPUT_BUFFER_SIZE];
    static uint8_t history_stack[HISTORY_STACK_SIZE];

    compiler.read_buffer       = input_buffer;
    compiler.write_buffer      = program_memory;
    compiler.write_buffer_size = PROGRAM_MEMORY_SIZE;

    interpreter.program_buffer               = program_memory;
    interpreter.basicfuck_cell_pointer       = BASICfuck_memory;
    interpreter.basicfuck_cell_start_pointer = BASICfuck_memory;
    interpreter.basicfuck_cell_end_pointer   = BASICfuck_memory + BASICFUCK_MEMORY_SIZE;
    interpreter.computer_memory_pointer      = 0;

    // Initalizes the history stack.
    tb_history_stack       = history_stack;
    tb_history_stack_size  = HISTORY_STACK_SIZE;
    tb_history_stack_index = 0;

    // Initializes global screen size variables in screen.h.
    screensize(&s_width, &s_height);
    // Initializes the opcode table in basicfuck.h.
    baf_initialize_instruction_opcode_table();


    clrscr();
    puts("Brainblast-Toolkit BASICfuck REPL " TOOLKIT_VERSION "\n");
    s_utoa_fputs(0, BASICFUCK_MEMORY_SIZE, 10);
    puts(
        " CELLS FREE\n"
        "\n"
        "Enter '?' for HELP\n"
        "Enter '!' to EXIT\n"
    );

    while (true) {
        // Read.
        fputs("YOUR WILL? ", stdout);
        tb_edit_buffer(input_buffer, INPUT_BUFFER_SIZE - 1);

        switch (input_buffer[0]) {
        case '\0':
            // empty input.
            continue;

        case '!':
            puts("SO BE IT.");
            goto lexit_repl;

        case '?':
            help_menu();
            continue;

        case '#':
            display_bytecode();
            continue;
        }

        // Evaluate.
        switch (baf_compile(&compiler)) {
        case BAF_COMPILE_OUT_OF_MEMORY:
            puts("?OUT OF MEMORY");
            continue;
        case BAF_COMPILE_UNTERMINATED_LOOP:
            puts("?UNTERMINATED LOOP");
            continue;
        case BAF_COMPILE_SUCCESS:
            break;
        default:
            assert(0 && "unreachable");
        }

        baf_interpret(&interpreter);

        // Print.
        s_utoa_fputs(3, *interpreter.basicfuck_cell_pointer, 10);
        fputs(" (Cell ", stdout);
        s_utoa_fputs(
            5,
            (uint16_t)(interpreter.basicfuck_cell_pointer
                       - interpreter.basicfuck_cell_start_pointer)
            , 10
        );
        fputs(", Memory $", stdout);
        s_utoa_fputs(4, (uint16_t)interpreter.computer_memory_pointer, 16);
        puts(")");
    }
lexit_repl:

    return 0;
}
