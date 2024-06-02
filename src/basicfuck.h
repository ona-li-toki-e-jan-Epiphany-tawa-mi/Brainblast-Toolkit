/*
 * This file is part of Brainblast-Toolkit.
 *
 * Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
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
 * BASICfuck bytecode compiler and related utilities.
 *
 * Preprocessor parameters:
 *  - BASICFUCK_IMPLEMENTATION - Define in *one* source file to instantiate the
 *                               implementation.
 */

#ifndef _BASICFUCK_H
#define _BASICFUCK_H

#include <stdint.h>



typedef uint8_t baf_opcode_t;
// Ends the current BASICfuck program.
#define BAF_OPCODE_HALT 0x00U
// Increments the current cell.
// argument1 - the amount to increment by.
#define BAF_OPCODE_INCREMENT 0x01U
// Decrements the current cell.
// argument1 - the amount to decrement by.
#define BAF_OPCODE_DECREMENT 0x02
// Moves the the cell pointer to the left.
// argument1 - the number of times to move to the left.
#define BAF_OPCODE_BFMEM_LEFT 0x03U
// Moves the the cell pointer to the right.
// argument1 - the number of times to move to the right.
#define BAF_OPCODE_BFMEM_RIGHT 0x04U
// Prints the value in the current cell as PETSCII character.
#define BAF_OPCODE_PRINT 0x05U
// Awaits a value from the keyboard and stores it in the current cell.
#define BAF_OPCODE_INPUT 0x06U
// Jumps to the given address if the current cell is 0.
// argument1,2 - the address in program memory to jump to.
#define BAF_OPCODE_JEQ 0x07U
// Jumps to the given address if the current cell is not 0.
// argument1,2 - the address in program memory to jump to.
#define BAF_OPCODE_JNE 0x08U
// Reads the value at the computer memory pointer into the current cell.
#define BAF_OPCODE_CMEM_READ 0x09U
// Writes the value in the current cell to the location at the computer memory
// pointer.
#define BAF_OPCODE_CMEM_WRITE 0x0AU
// Moves the computer memory pointer to the left.
// argument1 - the number of times to move to the left.
#define BAF_OPCODE_CMEM_LEFT 0x0BU
// Moves the computer memory pointer to the right.
// argument1 - the number of times to move to the right.
#define BAF_OPCODE_CMEM_RIGHT 0x0CU
// Runs the subroutine at the computer memory pointer with the current and next
// two cells as the values for the X, Y, and Z registers.
#define BAF_OPCODE_EXECUTE 0x0DU

/**
 * A table mapping from opcodes to their size (opcode + arguments) in bytes.
 *
 * Index value must be valid opcode.
 */
extern const uint8_t baf_opcode_size_table[];

/**
 * A table mapping from instruction characters to their corresponding opcodes.
 *
 * Index value must not exceed 255.
 * Must call baf_initialize_instruction_opcode_table() once prior to use.
 * If the given instruction does not have an opcode, 0xFF will be returned.
 */
extern baf_opcode_t baf_instruction_opcode_table[];

/**
 * A one-time-call function used to initialize instruction_opcode_table[].
 */
void baf_initialize_instruction_opcode_table();

typedef uint8_t BAFCompileResult;
#define BAF_COMPILE_SUCCESS           0U
#define BAF_COMPILE_OUT_OF_MEMORY     1U
#define BAF_COMPILE_UNTERMINATED_LOOP 2U

/**
 * Bytecode compiles BASICfuck code.
 *
 * @param read_buffer - the null-terminated program text buffer to compile.
 * @param write_buffer - the buffer to write the compiled program to.
 * @param write_buffer_size - the size of the program memory buffer.
 * @return BAF_COMPILE_SUCCESS on success,
 *         BAF_COMPILE_OUT_OF_MEMORY if the program exceeded the size of the
 *         program memory,
 *         BAF_COMPILE_UNTERMINATED_LOOP if the program has an unterminated
 *         loop.
 */
BAFCompileResult baf_compile(const uint8_t *const read_buffer, uint8_t *const write_buffer, const uint16_t write_buffer_size);



#ifdef BASICFUCK_IMPLEMENTATION

#include <stddef.h>
#include <stdbool.h>

const uint8_t baf_opcode_size_table[] = {
    1,                                            // BAF_OPCODE_HALT.
    2,                                            // BAF_OPCODE_INCREMENT.
    2,                                            // BAF_OPCODE_DECREMENT.
    2,                                            // BAF_OPCODE_BFMEM_LEFT.
    2,                                            // BAF_OPCODE_BFMEM_RIGHT.
    1,                                            // BAF_OPCODE_PRINT.
    1,                                            // BAF_OPCODE_INPUT.
    3,                                            // BAF_OPCODE_JEQ.
    3,                                            // BAF_OPCODE_JNE.
    1,                                            // BAF_OPCODE_CMEM_READ.
    1,                                            // BAF_OPCODE_CMEM_WRITE.
    2,                                            // BAF_OPCODE_CMEM_LEFT.
    2,                                            // BAF_OPCODE_CMEM_RIGHT.
    1                                             // BAF_OPCODE_EXECUTE.
};

baf_opcode_t baf_instruction_opcode_table[256];

void baf_initialize_instruction_opcode_table() {
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

// Global variables for passing parameters between compiler functions.
static const uint8_t* baf_compiler_read_buffer;       // the null-terminated buffer to read the program to compile from.
static uint16_t       baf_compiler_read_index;        // an index into the read buffer.
static uint8_t*       baf_compiler_write_buffer;      // the buffer to write the compiled program to.
static uint16_t       baf_compiler_write_index;       // an index into the write buffer.
static uint16_t       baf_compiler_write_buffer_size; // the size of the write buffer.

/**
 * Performs the first pass of BASICfuck compilation, converting the text program
 * to opcodes.
 *
 * @param compiler_read_buffer (global) - the read buffer.
 * @param compiler_read_index (global) - the current index into the read buffer.
 * @param compiler_write_buffer (global) - the write buffer.
 * @param compiler_write_index (global) - the current index into the write buffer.
 * @param compiler_write_buffer_size (global) - the size of the write buffer.
 * @return true if succeeded, false if ran out of memory.
 */
static bool baf_compile_first_pass() {
    uint8_t      instruction;
    baf_opcode_t opcode;

    // Used by counted instructions.
    uint16_t instruction_count;
    uint8_t  other_instruction;
    uint8_t  chunk_count;

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
        instruction = baf_compiler_read_buffer[baf_compiler_read_index];
        opcode      = baf_instruction_opcode_table[instruction];

        // Ignores non-instructions.
        if (opcode == 0xFF) {
            baf_compiler_read_index++;
            continue;
        }

        goto *jump_table[opcode];


        // End of program.
    lfinish_bytecode_compilation:
        baf_compiler_write_buffer[baf_compiler_write_index] = BAF_OPCODE_HALT;
        break;

        // Takes no arguments.
    lcompile_instruction_no_arugments:
        if (baf_compiler_write_index >= baf_compiler_write_buffer_size)
            return false;

        baf_compiler_write_buffer[baf_compiler_write_index] = opcode;
        ++baf_compiler_read_index;
        ++baf_compiler_write_index;

        continue;

        // Takes a 16-bit address relative to program memory as a parameter,
        // which will be handled by the second pass.
    lcompile_jump_instruction:
        if (baf_compiler_write_index + 2 >= baf_compiler_write_buffer_size)
            return false;

        baf_compiler_write_buffer[baf_compiler_write_index]   = opcode;
        baf_compiler_write_buffer[baf_compiler_write_index+1] = 0xFF;
        baf_compiler_write_buffer[baf_compiler_write_index+2] = 0xFF;
        ++baf_compiler_read_index;
        baf_compiler_write_index += 3;

        continue;

        // Takes an 8-bit count of how many times to preform the operation.
    lcompile_counted_instruction:
        instruction_count = 0;

        // Count number of consecutive instructions.
        while (true) {
            other_instruction = baf_compiler_read_buffer[baf_compiler_read_index];

            if (other_instruction != instruction)
                break;

            ++instruction_count;
            ++baf_compiler_read_index;
        }

        // Each instruction opcode can only take an 8-bit value, so this chops up
        // the full count into separate 8-bit chunks.
        while (instruction_count > 0) {
            if (baf_compiler_write_index + 1 >= baf_compiler_write_buffer_size)
                return false;

            chunk_count = instruction_count > 255 ? 255 : (uint8_t)instruction_count;

            baf_compiler_write_buffer[baf_compiler_write_index]   = opcode;
            baf_compiler_write_buffer[baf_compiler_write_index+1] = chunk_count;
            baf_compiler_write_index += 2;

            instruction_count -= (uint16_t)chunk_count;
        }

        continue;
    }


    return true;
}

/**
 * Performs the second pass of BASICfuck compilation, calculating the addresses
 * for jump instructions.
 *
 * @param write_buffer (global) - the write buffer.
 * @param compiler_state - the current state of the compiler.
 * @return true if succeeded, false if there is an unterminated loop.
 */
static bool baf_compile_second_pass() {
    uint16_t     loop_depth;
    uint16_t     seek_index;
    baf_opcode_t opcode, seeked_opcode;

    baf_compiler_write_index = 0;


    while ((opcode = baf_compiler_write_buffer[baf_compiler_write_index]) != BAF_OPCODE_HALT) {
        switch (opcode) {
        case BAF_OPCODE_JEQ:
            seek_index = baf_compiler_write_index + baf_opcode_size_table[BAF_OPCODE_JEQ];
            loop_depth = 1;

            // Finds and links with accomanying JNE instruction.
            while ((seeked_opcode = baf_compiler_write_buffer[seek_index]) != BAF_OPCODE_HALT) {
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
                    *(uint16_t*)(baf_compiler_write_buffer + baf_compiler_write_index+1) = seek_index;
                    // And vice-versa.
                    *(uint16_t*)(baf_compiler_write_buffer + seek_index+1) = baf_compiler_write_index;

                    break;
                }

                seek_index += baf_opcode_size_table[seeked_opcode];
            }

            if (loop_depth != 0)
                return false;

            break;

        case BAF_OPCODE_JNE:
            // Address should have been set by some preceeding JEQ instruction.
            if (*(uint16_t*)(baf_compiler_write_buffer + baf_compiler_write_index+1) == 0xFFFF)
                return false;

            break;
        }

        baf_compiler_write_index += baf_opcode_size_table[opcode];
    }


    return true;
}

BAFCompileResult baf_compile(const uint8_t *const read_buffer, uint8_t *const write_buffer, const uint16_t write_buffer_size) {
    baf_compiler_read_buffer       = read_buffer;
    baf_compiler_read_index        = 0;
    baf_compiler_write_buffer      = write_buffer;
    baf_compiler_write_index       = 0;
    // The last location is reserved for end of program.
    baf_compiler_write_buffer_size = write_buffer_size - 1;

    if (!baf_compile_first_pass())
        return BAF_COMPILE_OUT_OF_MEMORY;

    if (!baf_compile_second_pass())
        return BAF_COMPILE_UNTERMINATED_LOOP;

    return BAF_COMPILE_SUCCESS;
}

#endif // BASICFUCK_IMPLEMENTATION

#endif // _BASICFUCK_H
