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
 * - BASICFUCK_IMPLEMENTATION - Define in *one* source file to instantiate the
 *                              implementation.
 */

#ifndef _BASICFUCK_H
#define _BASICFUCK_H

#include <stdint.h>



typedef uint8_t baf_opcode_t;
#define BAF_RTS           0x60
#define BAF_INC_ABSOLUTE  0xEE
#define BAF_JSR           0x20
#define BAF_LDA_ABSOLUTE  0xAD
#define BAF_LDA_IMMEDIATE 0xA9
#define BAF_STA_ABSOLUTE  0x8D
#define BAF_LDX_IMMEDIATE 0xA2
#define BAF_DEX           0xCA
#define BAF_ADC_ABSOLUTE  0x6D
#define BAF_BNE           0xD0
#define BAF_DEC_ABSOLUTE  0xCE
#define BAF_BVC           0x50
#define BAF_BVC           0x50
#define BAF_BCS           0xB0
#define BAF_CLC           0x18
#define BAF_BCC           0x90
#define BAF_SBC_ABSOLUTE  0xED
#define BAF_SEC           0x38

// BASICfuck state.
extern uint8_t* baf_bfmem;
extern uint8_t* baf_cmem_pointer;

// Compiler state.
extern const uint8_t* baf_read_buffer;
extern baf_opcode_t*  baf_write_buffer;
extern uint16_t       baf_write_buffer_size;

typedef uint8_t BAFCompileResult;
#define BAF_COMPILE_SUCCESS           0U
#define BAF_COMPILE_OUT_OF_MEMORY     1U
#define BAF_COMPILE_UNTERMINATED_LOOP 2U

/**
 * Bytecode compiles BASICfuck code.
 *
 * @param baf_read_buffer (global) - the null-terminated program text
 *                                            buffer to compile.
 * @param baf_write_buffer (global) - the buffer to write the compiled program
 *                                    to.
 * @param baf_write_buffer_size (global) - the size of the program memory
 *                                         buffer.
 * @return BAF_COMPILE_SUCCESS on success,
 *         BAF_COMPILE_OUT_OF_MEMORY if the program exceeded the size of the
 *         program memory,
 *         BAF_COMPILE_UNTERMINATED_LOOP if the program has an unterminated
 *         loop.
 */
BAFCompileResult baf_compile();



#ifdef BASICFUCK_IMPLEMENTATION

#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

// BASICfuck state.
uint8_t* baf_bfmem;
uint8_t* baf_cmem_pointer;

// Compiler state.
const uint8_t* baf_read_buffer;
baf_opcode_t*  baf_write_buffer;
uint16_t       baf_write_buffer_size;

/**
 * Performs the first pass of BASICfuck compilation, converting the text program
 * to machine code.
 *
 * @param baf_read_buffer (global) - the read buffer.
 * @param baf_write_buffer (global) - the write buffer.
 * @param baf_write_buffer_size (global) - the size of the write buffer.
 * @return true if succeeded, false if ran out of memory.
 */
static bool baf_compile_first_pass() {
    uint8_t instruction;
    // Used to count the number of times an instruction occurs.
    uint8_t operand;
    // Used to figure out addressing for self-modifying code.
    baf_opcode_t* address;

    const uint8_t* read_address  = baf_read_buffer;
    baf_opcode_t*  write_address = baf_write_buffer;

#define BAF_PUSH_TYPE(type, value)              \
    {                                           \
        *((type*)write_address) = (value);      \
        write_address += sizeof(type);          \
    }

    while (true) {
        instruction = *(read_address++);

        switch (instruction) {
            // The null-terminator marks the end of the program.
        case '\0': {
            //if (baf_write_buffer_size >= baf_write_index) return false;

            //    rts
            BAF_PUSH_TYPE(baf_opcode_t, BAF_RTS);
        } goto lend_first_pass;

            // Increments/decrements the current cell.
        case '+':
        case '-': {
            operand = 1;
            while (instruction == *(read_address)) {
                ++operand;
                ++read_address;
            }

            // TODO: make work correctly.
            //    lda baf_bfmem
            BAF_PUSH_TYPE(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH_TYPE(uint8_t*, baf_bfmem);
            //    sta lset_address1+1
            address = 1 + 17 + write_address;
            BAF_PUSH_TYPE(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH_TYPE(baf_opcode_t*, address);
            //    sta lset_address2+1
            address = 1 + 17 + write_address;
            BAF_PUSH_TYPE(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH_TYPE(baf_opcode_t*, address);
            //    lda baf_bfmem+1
            BAF_PUSH_TYPE(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH_TYPE(uint8_t*, 1 + baf_bfmem);
            //    sta lset_address1+2
            address = 2 + 8 + write_address;
            BAF_PUSH_TYPE(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH_TYPE(baf_opcode_t*, address);
            //    sta lset_address2+2
            address = 2 + 8 + write_address;
            BAF_PUSH_TYPE(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH_TYPE(baf_opcode_t*, address);
            //    lda, #(operand|-operand)
            BAF_PUSH_TYPE(baf_opcode_t, BAF_LDA_IMMEDIATE);
            BAF_PUSH_TYPE(baf_opcode_t, '+' == instruction ? operand : -operand);
            // lset_address1:
            //    adc <address>
            BAF_PUSH_TYPE(baf_opcode_t, BAF_ADC_ABSOLUTE);
            BAF_PUSH_TYPE(baf_opcode_t*, NULL);
            // lset_address2:
            //    sta <address>
            BAF_PUSH_TYPE(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH_TYPE(baf_opcode_t*, NULL);
        } continue;

        case '>': {
            operand = 1;
            while (instruction == *(read_address)) {
                ++operand;
                ++read_address;
            }

            //    lda #operand
            BAF_PUSH_TYPE(baf_opcode_t, BAF_LDA_IMMEDIATE);
            BAF_PUSH_TYPE(uint8_t,      operand);
            //    clc
            BAF_PUSH_TYPE(baf_opcode_t, BAF_CLC);
            //    adc baf_bfmem
            BAF_PUSH_TYPE(baf_opcode_t, BAF_ADC_ABSOLUTE);
            BAF_PUSH_TYPE(uint8_t**,    &baf_bfmem);
            //    sta baf_bfmem
            BAF_PUSH_TYPE(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH_TYPE(uint8_t**,    &baf_bfmem);
            //    bcc lno_carry
            BAF_PUSH_TYPE(baf_opcode_t, BAF_BCC);
            BAF_PUSH_TYPE(uint8_t,      3);
            //    inc baf_bfmem+1
            BAF_PUSH_TYPE(baf_opcode_t, BAF_INC_ABSOLUTE);
            BAF_PUSH_TYPE(uint8_t**,    (uint8_t**)(1 + (uint16_t)&baf_bfmem));
            // lno_carry:
        } continue;

        case '<':
        case '.':
        case ',':
        case '[':
        case ']':
        case '@':
        case '*':
        case '(':
        case ')':
        case '%': assert(false && "TODO");

            // Ignores non-instructions.
        default: continue;
        }



    /*     // End of program. */
    /* lfinish_bytecode_compilation: */
    /*     baf_compiler_write_buffer[baf_compiler_write_index] = BAF_OPCODE_HALT; */
    /*     break; */

    /*     // Takes no arguments. */
    /* lcompile_instruction_no_arugments: */
    /*     if (baf_compiler_write_index >= baf_compiler_write_buffer_size) */
    /*         return false; */

    /*     baf_compiler_write_buffer[baf_compiler_write_index] = opcode; */
    /*     ++baf_compiler_read_index; */
    /*     ++baf_compiler_write_index; */

    /*     continue; */

    /*     // Takes a 16-bit address relative to program memory as a parameter, */
    /*     // which will be handled by the second pass. */
    /* lcompile_jump_instruction: */
    /*     if (baf_compiler_write_index + 2 >= baf_compiler_write_buffer_size) */
    /*         return false; */

    /*     baf_compiler_write_buffer[baf_compiler_write_index]   = opcode; */
    /*     baf_compiler_write_buffer[baf_compiler_write_index+1] = 0xFF; */
    /*     baf_compiler_write_buffer[baf_compiler_write_index+2] = 0xFF; */
    /*     ++baf_compiler_read_index; */
    /*     baf_compiler_write_index += 3; */

    /*     continue; */

    /*     // Takes an 8-bit count of how many times to preform the operation. */
    /* lcompile_counted_instruction: */
    /*     instruction_count = 0; */

    /*     // Count number of consecutive instructions. */
    /*     while (true) { */
    /*         other_instruction = baf_compiler_read_buffer[baf_compiler_read_index]; */

    /*         if (other_instruction != instruction) */
    /*             break; */

    /*         ++instruction_count; */
    /*         ++baf_compiler_read_index; */
    /*     } */

    /*     // Each instruction opcode can only take an 8-bit value, so this chops up */
    /*     // the full count into separate 8-bit chunks. */
    /*     while (instruction_count > 0) { */
    /*         if (baf_compiler_write_index + 1 >= baf_compiler_write_buffer_size) */
    /*             return false; */

    /*         chunk_count = instruction_count > 255 ? 255 : (uint8_t)instruction_count; */

    /*         baf_compiler_write_buffer[baf_compiler_write_index]   = opcode; */
    /*         baf_compiler_write_buffer[baf_compiler_write_index+1] = chunk_count; */
    /*         baf_compiler_write_index += 2; */

    /*         instruction_count -= (uint16_t)chunk_count; */
    /*     } */

    /*     continue; */
    }
 lend_first_pass:

    return true;

    // We undefine this macro since they only make sense within the context of
    // this function.
#undef BAF_PUSH_TYPE
}

/**
 * Performs the second pass of BASICfuck compilation, calculating the addresses
 * for jump instructions.
 *
 * @param baf_compiler_write_buffer (global) - the write buffer.
 * @return true if succeeded, false if there is an unterminated loop.
 */
static bool baf_compile_second_pass() {
    return true;
    /* uint16_t     loop_depth; */
    /* uint16_t     seek_index; */
    /* baf_opcode_t opcode, seeked_opcode; */

    /* baf_compiler_write_index = 0; */


    /* while ((opcode = baf_compiler_write_buffer[baf_compiler_write_index]) != BAF_OPCODE_HALT) { */
    /*     switch (opcode) { */
    /*     case BAF_OPCODE_JEQ: */
    /*         seek_index = baf_compiler_write_index + baf_opcode_size_table[BAF_OPCODE_JEQ]; */
    /*         loop_depth = 1; */

    /*         // Finds and links with accomanying JNE instruction. */
    /*         while ((seeked_opcode = baf_compiler_write_buffer[seek_index]) != BAF_OPCODE_HALT) { */
    /*             switch (seeked_opcode) { */
    /*             case BAF_OPCODE_JEQ: */
    /*                 ++loop_depth; */
    /*                 break; */
    /*             case BAF_OPCODE_JNE: */
    /*                 --loop_depth; */
    /*                 break; */
    /*             } */

    /*             if (loop_depth == 0) { */
    /*                 // Sets JEQ instruction to jump to accomanying JNE. */
    /*                 *(uint16_t*)(baf_compiler_write_buffer + baf_compiler_write_index+1) = seek_index; */
    /*                 // And vice-versa. */
    /*                 *(uint16_t*)(baf_compiler_write_buffer + seek_index+1) = baf_compiler_write_index; */

    /*                 break; */
    /*             } */

    /*             seek_index += baf_opcode_size_table[seeked_opcode]; */
    /*         } */

    /*         if (loop_depth != 0) */
    /*             return false; */

    /*         break; */

    /*     case BAF_OPCODE_JNE: */
    /*         // Address should have been set by some preceeding JEQ instruction. */
    /*         if (*(uint16_t*)(baf_compiler_write_buffer + baf_compiler_write_index+1) == 0xFFFF) */
    /*             return false; */

    /*         break; */
    /*     } */

    /*     baf_compiler_write_index += baf_opcode_size_table[opcode]; */
    /* } */


    return true;
}

BAFCompileResult baf_compile() {
    if (!baf_compile_first_pass())
        return BAF_COMPILE_OUT_OF_MEMORY;

    if (!baf_compile_second_pass())
        return BAF_COMPILE_UNTERMINATED_LOOP;

    return BAF_COMPILE_SUCCESS;
}

/* // Interpreter state. */
/* const uint8_t* baf_interpreter_program_memory; */
/* uint8_t*       baf_interpreter_bfmem; */
/* uint16_t       baf_interpreter_bfmem_size; */
/* uint16_t       baf_interpreter_bfmem_index; */
/* uint8_t*       baf_interpreter_cmem_pointer; */

/* // Global variables for exchaning values with inline assembler. */
/* static uint8_t  baf_interpreter_register_a */
/*              ,  baf_interpreter_register_x */
/*              ,  baf_interpreter_register_y; */

/* /\** */
/*  * Runs the execute part of the BASICfuck execute instruction. */
/*  * */
/*  * @param baf_interpreter_register_a (global) - the value to place in the A */
/*  *                                              register. */
/*  * @param baf_interpreter_register_x (global) - the value to place in the X */
/*  *                                              register. */
/*  * @param baf_interpreter_register_y (global) - the value to place in the Y */
/*  *                                              register. */
/*  * @param baf_interpreter_cmem_pointer (global) - the address to execute as a */
/*  *                                                subroutine. */
/*  *\/ */
/* static void baf_execute() { */
/*     // Overwrites address of subroutine to call in next assembly block with the */
/*     // computer memory pointer's value. */
/*     __asm__ volatile ("lda     %v",   baf_interpreter_cmem_pointer); */
/*     __asm__ volatile ("sta     %g+1", ljump_instruction); */
/*     __asm__ volatile ("lda     %v+1", baf_interpreter_cmem_pointer); */
/*     __asm__ volatile ("sta     %g+2", ljump_instruction); */
/*     // Executes subroutine. */
/*     __asm__ volatile ("lda     %v",   baf_interpreter_register_a); */
/*     __asm__ volatile ("ldx     %v",   baf_interpreter_register_x); */
/*     __asm__ volatile ("ldy     %v",   baf_interpreter_register_y); */
/*  ljump_instruction: */
/*     __asm__ volatile ("jsr     %w",   NULL); */
/*     // Retrieves resuting values. */
/*     __asm__ volatile ("sta     %v",   baf_interpreter_register_a); */
/*     __asm__ volatile ("stx     %v",   baf_interpreter_register_x); */
/*     __asm__ volatile ("sty     %v",   baf_interpreter_register_y); */

/*     return; */
/*     // If we don't include a jmp instruction, cc65, annoyingly, strips the label */
/*     // from the resulting assembly. */
/*     __asm__ volatile ("jmp     %g", ljump_instruction); */
/* } */

/* void baf_interpret() { */
/*     baf_opcode_t opcode; */
/*     uint8_t      argument; */

/*     uint16_t program_index = 0; */

/*     static const void *const jump_table[] = { */
/*         &&lopcode_halt,                           // BAF_OPCODE_HALT. */
/*         &&lopcode_increment,                      // BAF_OPCODE_INCREMENT. */
/*         &&lopcode_decrement,                      // BAF_OPCODE_DECREMENT. */
/*         &&lopcode_bfmem_left,                     // BAF_OPCODE_BFMEM_LEFT. */
/*         &&lopcode_bfmem_right,                    // BAF_OPCODE_BFMEM_RIGHT. */
/*         &&lopcode_print,                          // BAF_OPCODE_PRINT. */
/*         &&lopcode_input,                          // BAF_OPCODE_INPUT. */
/*         &&lopcode_jeq,                            // BAF_OPCODE_JEQ. */
/*         &&lopcode_jne,                            // BAF_OPCODE_JNE. */
/*         &&lopcode_cmem_read,                      // BAF_OPCODE_CMEM_READ. */
/*         &&lopcode_cmem_write,                     // BAF_OPCODE_CMEM_WRITE. */
/*         &&lopcode_cmem_left,                      // BAF_OPCODE_CMEM_LEFT. */
/*         &&lopcode_cmem_right,                     // BAF_OPCODE_CMEM_RIGHT. */
/*         &&lopcode_execute                         // BAF_OPCODE_EXECUTE. */
/*     }; */


/*     while (true) { */
/*         if (kbhit() != 0 && cgetc() == KEYBOARD_STOP) { */
/*             puts("?ABORT"); */
/*             break; */
/*         } */


/*         opcode   = baf_interpreter_program_memory[program_index]; */
/*         argument = baf_interpreter_program_memory[program_index+1]; */
/*         assert(opcode < sizeof(jump_table)/sizeof(jump_table[0]) && "Unknown opcode"); */
/*         goto *jump_table[opcode]; */

/*     lopcode_halt: */
/*         break; */

/*     lopcode_increment: */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index] += argument; */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_decrement: */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index] -= argument; */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_bfmem_left: */
/*         if (baf_interpreter_bfmem_index > argument) { */
/*             baf_interpreter_bfmem_index -= argument; */
/*         } else { */
/*             baf_interpreter_bfmem_index = 0; */
/*         } */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_bfmem_right: */
/*         if (baf_interpreter_bfmem_index + argument < baf_interpreter_bfmem_size) */
/*             baf_interpreter_bfmem_index += argument; */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_print: */
/*         (void)putchar(baf_interpreter_bfmem[baf_interpreter_bfmem_index]); */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_input: */
/*         argument = s_wrapped_cgetc(); */
/*         if (KEYBOARD_STOP == argument) { */
/*             puts("?ABORT"); */
/*             break; */
/*         }; */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index] = argument; */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_jeq: */
/*         if (baf_interpreter_bfmem[baf_interpreter_bfmem_index] == 0) { */
/*             program_index = (uint16_t)argument */
/*                           + ((uint16_t)baf_interpreter_program_memory[program_index+2] << 8); */
/*         } */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_jne: */
/*         if (baf_interpreter_bfmem[baf_interpreter_bfmem_index] != 0) { */
/*             program_index = (uint16_t)argument */
/*                           + ((uint16_t)baf_interpreter_program_memory[program_index+2] << 8); */
/*         } */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_cmem_read: */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index] = *baf_interpreter_cmem_pointer; */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_cmem_write: */
/*         *baf_interpreter_cmem_pointer = baf_interpreter_bfmem[baf_interpreter_bfmem_index]; */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_cmem_left: */
/*         if ((uint16_t)baf_interpreter_cmem_pointer > argument) { */
/*             baf_interpreter_cmem_pointer -= argument; */
/*         } else { */
/*             baf_interpreter_cmem_pointer = 0; */
/*         } */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_cmem_right: */
/*         if (UINT16_MAX - (uint16_t)baf_interpreter_cmem_pointer > argument) { */
/*             baf_interpreter_cmem_pointer += argument; */
/*         } else { */
/*             baf_interpreter_cmem_pointer = (uint8_t*)UINT16_MAX; */
/*         } */
/*         goto lfinish_interpreter_cycle; */

/*     lopcode_execute: */
/*         baf_interpreter_register_a = baf_interpreter_bfmem[baf_interpreter_bfmem_index]; */
/*         baf_interpreter_register_x = baf_interpreter_bfmem[baf_interpreter_bfmem_index+1]; */
/*         baf_interpreter_register_y = baf_interpreter_bfmem[baf_interpreter_bfmem_index+2]; */
/*         baf_execute(); */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index]   = baf_interpreter_register_a; */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index+1] = baf_interpreter_register_x; */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index+2] = baf_interpreter_register_y; */
/*         goto lfinish_interpreter_cycle; */


/*     lfinish_interpreter_cycle: */
/*         // Jumped to after an opcode has been executed. */
/*         program_index += baf_opcode_size_table[opcode]; */
/*     } */
/* } */

#endif // BASICFUCK_IMPLEMENTATION

#endif // _BASICFUCK_H
