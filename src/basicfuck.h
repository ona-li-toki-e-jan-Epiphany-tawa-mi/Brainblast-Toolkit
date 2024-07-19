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
 * BASICfuck bytecode compiler and related utilities.
 *
 * Preprocessor parameters:
 * - BASICFUCK_IMPLEMENTATION - Define in *one* source file to instantiate the
 *                              implementation.
 * - BASICFUCK_DISABLE_COMPILER - If defined, strips out compiler code.
 * - BASICFUCK_DISABLE_INTERPRETER - If defined, strips out interpreter code.
 */

#ifndef _BASICFUCK_H
#define _BASICFUCK_H

#include <stdint.h>



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


#ifndef BASICFUCK_DISABLE_COMPILER
typedef uint8_t BAFCompileResult;
#define BAF_COMPILE_SUCCESS           0
#define BAF_COMPILE_OUT_OF_MEMORY     1
#define BAF_COMPILE_UNTERMINATED_LOOP 2

// Compiler state.
typedef struct {
    // Buffer to read BASICfuck code from.
    const uint8_t* read_buffer;
    // Buffer to write bytecode to.
    baf_opcode_t* write_buffer;
    uint16_t      write_buffer_size;
} BAFCompiler;

/**
 * Bytecode compiles BASICfuck code.
 *
 * @param compiler - compiler settings.
 * @return BAF_COMPILE_SUCCESS on success,
 *         BAF_COMPILE_OUT_OF_MEMORY if the program exceeded the size of the
 *         program memory,
 *         BAF_COMPILE_UNTERMINATED_LOOP if the program has an unterminated
 *         loop.
 */
BAFCompileResult baf_compile(const BAFCompiler* compiler);
#endif // BASICFUCK_DISABLE_COMPILER


#ifndef BASICFUCK_DISABLE_INTERPRETER
// Intepreter state.
extern const baf_opcode_t* baf_interpreter_program_memory;
extern baf_cell_t*         baf_interpreter_bfmem;
extern uint16_t            baf_interpreter_bfmem_size;
extern uint16_t            baf_interpreter_bfmem_index;
extern uint8_t*            baf_interpreter_cmem_pointer;

/**
 * Runs the interpreter with the given bytecode-compiled BASICfuck program,
 * leaving the given starting state off wherever it the program finished at.
 *
 * @param baf_interpreter_program_memory (global) - the BASICfuck program.
 * @param baf_interpreter_bfmem (global) - the BASICfuck memory buffer.
 * @param baf_interpreter_bfmem_size (global) - the size of the memory buffer.
 * @param baf_interpreter_bfmem_index (global) - the current index in BASICfuck
 *        memory.
 * @param baf_interpreter_cmem_pointer (global) - the current pointer into RAM.
 */
void baf_interpret();
#endif // BASICFUCK_DISABLE_INTERPRETER



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

#ifndef BASICFUCK_DISABLE_COMPILER
// Compiler state.
static const uint8_t* baf_compiler_read_pointer      = NULL; // Pointer to the current position in the read buffer.
static baf_opcode_t*  baf_compiler_write_pointer     = NULL; // Pointer to the current position in the write buffer.
static baf_opcode_t*  baf_compiler_write_end_pointer = NULL; // Pointer to one after the end of the write buffer.

/**
 * Performs the first pass of BASICfuck compilation, converting the text program
 * to opcodes.
 *
 * @param baf_compiler_read_pointer (global) - set to the start of the read
 *        buffer. Gets clobbered.
 * @param baf_compiler_write_pointer (global) - set to the start of the write
 *        buffer. Gets clobbered.
 * @param baf_compiler_write_end_pointer (global).
 * @return true if succeeded, false if ran out of memory.
 */
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

/**
 * Performs the second pass of BASICfuck compilation, calculating the addresses
 * for jump instructions.
 *
 * @param baf_compiler_write_pointer (global) - set to the start of the write
 *        buffer. Gets clobbered.
 * @return true if succeeded, false if there is an unterminated loop.
 */
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
                    *(uint16_t*)(baf_compiler_write_pointer + 1) = seek_pointer - write_start_pointer;
                    // And vice-versa.
                    *(uint16_t*)(seek_pointer + 1) = baf_compiler_write_pointer - write_start_pointer;

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

BAFCompileResult baf_compile(const BAFCompiler* compiler) {
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
#endif // BASICFUCK_DISABLE_COMPILER

#ifndef BASICFUCK_DISABLE_INTERPRETER
// Interpreter state.
const uint8_t* baf_interpreter_program_memory;
baf_cell_t*    baf_interpreter_bfmem;
uint16_t       baf_interpreter_bfmem_size;
uint16_t       baf_interpreter_bfmem_index;
uint8_t*       baf_interpreter_cmem_pointer;

// Global variables for exchaning values with inline assembler.
static uint8_t  baf_interpreter_register_a
             ,  baf_interpreter_register_x
             ,  baf_interpreter_register_y;

/**
 * Runs the execute part of the BASICfuck execute instruction.
 *
 * @param baf_interpreter_register_a (global) - the value to place in the A
 *                                              register.
 * @param baf_interpreter_register_x (global) - the value to place in the X
 *                                              register.
 * @param baf_interpreter_register_y (global) - the value to place in the Y
 *                                              register.
 * @param baf_interpreter_cmem_pointer (global) - the address to execute as a
 *                                                subroutine.
 */
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

void baf_interpret() {
    baf_opcode_t opcode;
    uint8_t      argument;

    uint16_t program_index = 0;

    static const void *const jump_table[] = {
        &&lopcode_halt,                           // BAF_OPCODE_HALT.
        &&lopcode_increment,                      // BAF_OPCODE_INCREMENT.
        &&lopcode_decrement,                      // BAF_OPCODE_DECREMENT.
        &&lopcode_bfmem_left,                     // BAF_OPCODE_BFMEM_LEFT.
        &&lopcode_bfmem_right,                    // BAF_OPCODE_BFMEM_RIGHT.
        &&lopcode_print,                          // BAF_OPCODE_PRINT.
        &&lopcode_input,                          // BAF_OPCODE_INPUT.
        &&lopcode_jeq,                            // BAF_OPCODE_JEQ.
        &&lopcode_jne,                            // BAF_OPCODE_JNE.
        &&lopcode_cmem_read,                      // BAF_OPCODE_CMEM_READ.
        &&lopcode_cmem_write,                     // BAF_OPCODE_CMEM_WRITE.
        &&lopcode_cmem_left,                      // BAF_OPCODE_CMEM_LEFT.
        &&lopcode_cmem_right,                     // BAF_OPCODE_CMEM_RIGHT.
        &&lopcode_execute                         // BAF_OPCODE_EXECUTE.
    };


    while (true) {
        if (kbhit() != 0 && cgetc() == KEYBOARD_STOP) {
            puts("?ABORT");
            break;
        }


        opcode   = baf_interpreter_program_memory[program_index];
        argument = baf_interpreter_program_memory[program_index+1];
        assert(opcode < sizeof(jump_table)/sizeof(jump_table[0]) && "Unknown opcode");
        goto *jump_table[opcode];

    lopcode_halt:
        break;

    lopcode_increment:
        baf_interpreter_bfmem[baf_interpreter_bfmem_index] += argument;
        goto lfinish_interpreter_cycle;

    lopcode_decrement:
        baf_interpreter_bfmem[baf_interpreter_bfmem_index] -= argument;
        goto lfinish_interpreter_cycle;

    lopcode_bfmem_left:
        if (baf_interpreter_bfmem_index > argument) {
            baf_interpreter_bfmem_index -= argument;
        } else {
            baf_interpreter_bfmem_index = 0;
        }
        goto lfinish_interpreter_cycle;

    lopcode_bfmem_right:
        if (baf_interpreter_bfmem_index + argument < baf_interpreter_bfmem_size)
            baf_interpreter_bfmem_index += argument;
        goto lfinish_interpreter_cycle;

    lopcode_print:
        (void)putchar(baf_interpreter_bfmem[baf_interpreter_bfmem_index]);
        goto lfinish_interpreter_cycle;

    lopcode_input:
        argument = s_wrapped_cgetc();
        if (KEYBOARD_STOP == argument) {
            puts("?ABORT");
            break;
        };
        baf_interpreter_bfmem[baf_interpreter_bfmem_index] = argument;
        goto lfinish_interpreter_cycle;

    lopcode_jeq:
        if (baf_interpreter_bfmem[baf_interpreter_bfmem_index] == 0) {
            program_index = (uint16_t)argument
                          + ((uint16_t)baf_interpreter_program_memory[program_index+2] << 8);
        }
        goto lfinish_interpreter_cycle;

    lopcode_jne:
        if (baf_interpreter_bfmem[baf_interpreter_bfmem_index] != 0) {
            program_index = (uint16_t)argument
                          + ((uint16_t)baf_interpreter_program_memory[program_index+2] << 8);
        }
        goto lfinish_interpreter_cycle;

    lopcode_cmem_read:
        baf_interpreter_bfmem[baf_interpreter_bfmem_index] = *baf_interpreter_cmem_pointer;
        goto lfinish_interpreter_cycle;

    lopcode_cmem_write:
        *baf_interpreter_cmem_pointer = baf_interpreter_bfmem[baf_interpreter_bfmem_index];
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
        baf_interpreter_register_a = baf_interpreter_bfmem[baf_interpreter_bfmem_index];
        baf_interpreter_register_x = baf_interpreter_bfmem[baf_interpreter_bfmem_index+1];
        baf_interpreter_register_y = baf_interpreter_bfmem[baf_interpreter_bfmem_index+2];
        baf_execute();
        baf_interpreter_bfmem[baf_interpreter_bfmem_index]   = baf_interpreter_register_a;
        baf_interpreter_bfmem[baf_interpreter_bfmem_index+1] = baf_interpreter_register_x;
        baf_interpreter_bfmem[baf_interpreter_bfmem_index+2] = baf_interpreter_register_y;
        goto lfinish_interpreter_cycle;


    lfinish_interpreter_cycle:
        // Jumped to after an opcode has been executed.
        program_index += baf_opcode_size_table[opcode];
    }
}
#endif // BASICFUCK_DISABLE_INTERPRETER

#endif // BASICFUCK_IMPLEMENTATION

#endif // _BASICFUCK_H
