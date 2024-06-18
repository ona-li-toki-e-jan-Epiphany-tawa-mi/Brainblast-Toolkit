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



typedef uint8_t baf_cell_t;

typedef uint8_t baf_opcode_t;
#define BAF_ADC_ABSOLUTE   0x6D
#define BAF_ADC_IMMEDIATE  0x69
#define BAF_BCC            0x90
#define BAF_BCS            0xB0
#define BAF_BNE            0xD0
#define BAF_BVC            0x50
#define BAF_BVC            0x50
#define BAF_CLC            0x18
#define BAF_DEC_ABSOLUTE   0xCE
#define BAF_DEX            0xCA
#define BAF_INC_ABSOLUTE   0xEE
#define BAF_JSR            0x20
#define BAF_LDA_ABSOLUTE   0xAD
#define BAF_LDA_IMMEDIATE  0xA9
#define BAF_LDA_INDIRECT_Y 0xB1
#define BAF_LDX_ABSOLUTE   0xAE
#define BAF_LDX_IMMEDIATE  0xA2
#define BAF_LDY_IMMEDIATE  0xA0
#define BAF_RTS            0x60
#define BAF_SBC_IMMEDIATE  0xE9
#define BAF_SEC            0x38
#define BAF_STA_ABSOLUTE   0x8D
#define BAF_STA_INDIRECT_Y 0x91
#define BAF_STA_ZEROPAGE   0x85
#define BAF_STX_ZEROPAGE   0x86

// BASICfuck state.
extern baf_cell_t* baf_bfmem;
extern uint8_t*    baf_cmem_pointer;

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
baf_cell_t* baf_bfmem;
uint8_t*    baf_cmem_pointer;

// Compiler state.
const uint8_t* baf_read_buffer;
baf_opcode_t*  baf_write_buffer;
uint16_t       baf_write_buffer_size;

// The first pointer register allocated by cc65. Used for indirect
// addressing. Zero page address.
static uint8_t pointer1;

// TODO add bounds checking.
// TODO add abiltiy to abort program.
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
    uint8_t instruction = 0;
    // Used to count the number of times an instruction occurs.
    uint8_t operand = 0;
    // Used to figure out addressing for self-modifying code.
    baf_opcode_t* address = NULL;

    // To make the code smaller and faster, rather than accessing the buffer via
    // index (i.e. buffer[index],) we just directly move a pointer instead.
    const uint8_t* read_address  = baf_read_buffer;
    baf_opcode_t*  write_address = baf_write_buffer;

    // We can't access assembly variables directly from C, so instead we use a
    // little inline assembly to copy it to a C variable.
    __asm__ ("lda    #<(ptr1)"          );
    __asm__ ("sta    %v      ", pointer1);

    /**
     * Pushes data to the write buffer.
     * @param type - the type of the data.
     * @param value - the data.
     */
#define BAF_PUSH(type, value)                   \
    {                                           \
        *((type*)write_address) = (value);      \
        write_address += sizeof(type);          \
    }

    /**
     * Pushes a function pointer to the write buffer.
     * @param return_type - the function's return type.
     * @param argument_types - the function's argument types.
     * @param pointer - the function pointer/address.
     */
#define BAF_PUSH_FUNCTION(return_type, argument_types, pointer)         \
    {                                                                   \
        *((return_type(**)(argument_types))write_address) = (pointer);  \
        write_address += sizeof(return_type(*)(argument_types));        \
    }

    /**
     * Computes the operand of counted instructions.
     */
#define BAF_COMPUTE_OPERAND                     \
    {                                           \
        operand = 1;                            \
        while (instruction == *read_address) {  \
            ++operand;                          \
            ++read_address;                     \
        }                                       \
    }

    while (true) {
        instruction = *(read_address++);

        switch (instruction) {
            // The null-terminator marks the end of the program.
        case '\0': {
            // return;

            //    rts
            BAF_PUSH(baf_opcode_t, BAF_RTS);
        } goto lend_first_pass;

            // Increments/decrements the current cell.
        case '+':
        case '-': {
            BAF_COMPUTE_OPERAND;
            // To perform decrements we add the two's complement of the operand
            // to subtract, which is effectively subtraction.
            if ('-' == instruction) operand = -operand;

            // *baf_bfmem += operand;

            //    lda    baf_bfmem
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, &baf_bfmem);
            //    sta    pointer1
            BAF_PUSH(baf_opcode_t, BAF_STA_ZEROPAGE);
            BAF_PUSH(uint8_t,      pointer1);
            //    lda    baf_bfmem+1
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, (baf_cell_t**)(1+(uint16_t)&baf_bfmem));
            //    sta    pointer1+1
            BAF_PUSH(baf_opcode_t, BAF_STA_ZEROPAGE);
            BAF_PUSH(uint8_t,      1+pointer1);
            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t, 0);
            //    lda    (pointer1),y
            BAF_PUSH(baf_opcode_t, BAF_LDA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      pointer1);
            //    clc
            BAF_PUSH(baf_opcode_t, BAF_CLC);
            //    adc    #operand
            BAF_PUSH(baf_opcode_t, BAF_ADC_IMMEDIATE);
            BAF_PUSH(uint8_t, operand);
            //    sta    (pointer1),y
            BAF_PUSH(baf_opcode_t, BAF_STA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      pointer1);
        } continue;

            // Moves the cell memory pointer to the right.
        case '<': {
            BAF_COMPUTE_OPERAND;

            // baf_bfmem -= operand;

            //    lda    baf_bfmem
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, &baf_bfmem);
            //    sec
            BAF_PUSH(baf_opcode_t, BAF_SEC);
            //    sbc    #operand
            BAF_PUSH(baf_opcode_t, BAF_SBC_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    sta    baf_bfmem
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, &baf_bfmem);
            //    bcs    lno_borrow
            BAF_PUSH(baf_opcode_t, BAF_BCS);
            BAF_PUSH(uint8_t,      3);
            //    dec    baf_bfmem+1
            BAF_PUSH(baf_opcode_t, BAF_DEC_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, (baf_cell_t**)(1 + (uint16_t)&baf_bfmem));
            // lno_borrow:
        } continue;

            // Moves the cell memory pointer to the right.
        case '>': {
            BAF_COMPUTE_OPERAND;

            // baf_bfmem += operand;

            //    lda    #operand
            BAF_PUSH(baf_opcode_t, BAF_LDA_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    clc
            BAF_PUSH(baf_opcode_t, BAF_CLC);
            //    adc    baf_bfmem
            BAF_PUSH(baf_opcode_t, BAF_ADC_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, &baf_bfmem);
            //    sta    baf_bfmem
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, &baf_bfmem);
            //    bcc    lno_carry
            BAF_PUSH(baf_opcode_t, BAF_BCC);
            BAF_PUSH(uint8_t,      3);
            //    inc    baf_bfmem+1
            BAF_PUSH(baf_opcode_t, BAF_INC_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, (baf_cell_t**)(1 + (uint16_t)&baf_bfmem));
            // lno_carry:
        } continue;

            // TODO make accept operand.
            // Prints the value of the current cell as a character.
        case '.': {
            // (void)putchar(*baf_bfmem);

            //    lda    baf_bfmem
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, &baf_bfmem);
            //    sta    pointer1
            BAF_PUSH(baf_opcode_t, BAF_STA_ZEROPAGE);
            BAF_PUSH(uint8_t,      pointer1);
            //    lda    baf_bfmem+1
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, (baf_cell_t**)(1+(uint16_t)&baf_bfmem));
            //    sta    pointer1+1
            BAF_PUSH(baf_opcode_t, BAF_STA_ZEROPAGE);
            BAF_PUSH(uint8_t,      1+pointer1);
            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t, 0);
            //    lda    (pointer1),y
            BAF_PUSH(baf_opcode_t, BAF_LDA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      pointer1);
            //    ldx    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDX_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            //    jsr    _putchar
            BAF_PUSH(baf_opcode_t, BAF_JSR);
            BAF_PUSH_FUNCTION(int, int, &putchar);
        } continue;

            // Accepts a character from the keyboard and stores its value in the
            // current cell.
        case ',': {
            // *baf_bfmem = s_wrapped_cgetc();

            //    jsr    _s_wrapped_cgetc
            BAF_PUSH(baf_opcode_t, BAF_JSR);
            BAF_PUSH_FUNCTION(uint8_t, void, &s_wrapped_cgetc);
            //    ldx    baf_bfmem
            BAF_PUSH(baf_opcode_t, BAF_LDX_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, &baf_bfmem);
            //    stx    pointer1
            BAF_PUSH(baf_opcode_t, BAF_STX_ZEROPAGE);
            BAF_PUSH(uint8_t,      pointer1);
            //    ldx    baf_bfmem+1
            BAF_PUSH(baf_opcode_t, BAF_LDX_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, (baf_cell_t**)(1+(uint16_t)&baf_bfmem));
            //    stx    pointer1+1
            BAF_PUSH(baf_opcode_t, BAF_STX_ZEROPAGE);
            BAF_PUSH(uint8_t,      1+pointer1);
            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t, 0);
            //    sta    (pointer1),y
            BAF_PUSH(baf_opcode_t, BAF_STA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      pointer1);
        } continue;

        case '[':
        case ']':
        case '@':
        case '*': assert(false && "TODO");

            // Moves the computer memory pointer to the left.
        case '(': {
            BAF_COMPUTE_OPERAND;

            // baf_cmem_pointer -= operand;

            //    lda    baf_cmem_pointer
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(uint8_t**,    &baf_cmem_pointer);
            //    sec
            BAF_PUSH(baf_opcode_t, BAF_SEC);
            //    sbc    #operand
            BAF_PUSH(baf_opcode_t, BAF_SBC_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    sta    baf_cmem_pointer
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(uint8_t**,    &baf_cmem_pointer);
            //    bcs    lno_borrow
            BAF_PUSH(baf_opcode_t, BAF_BCS);
            BAF_PUSH(uint8_t,      3);
            //    dec    baf_cmem_pointer+1
            BAF_PUSH(baf_opcode_t, BAF_DEC_ABSOLUTE);
            BAF_PUSH(uint8_t**,    (uint8_t**)(1 + (uint16_t)&baf_cmem_pointer));
            // lno_borrow:
        } continue;

            // Moves the computer memory pointer to the right.
        case ')': {
            BAF_COMPUTE_OPERAND;

            // baf_cmem_pointer += operand;

            //    lda    #operand
            BAF_PUSH(baf_opcode_t, BAF_LDA_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    clc
            BAF_PUSH(baf_opcode_t, BAF_CLC);
            //    adc    baf_cmem_pointer
            BAF_PUSH(baf_opcode_t, BAF_ADC_ABSOLUTE);
            BAF_PUSH(uint8_t**,    &baf_cmem_pointer);
            //    sta    baf_cmem_pointer
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(uint8_t**,    &baf_cmem_pointer);
            //    bcc    lno_carry
            BAF_PUSH(baf_opcode_t, BAF_BCC);
            BAF_PUSH(uint8_t,      3);
            //    inc    baf_cmem_pointer+1
            BAF_PUSH(baf_opcode_t, BAF_INC_ABSOLUTE);
            BAF_PUSH(uint8_t**,    (uint8_t**)(1 + (uint16_t)&baf_cmem_pointer));
            // lno_carry:
        } continue;

        case '%': assert(false && "TODO");

            // Ignores non-instructions.
        default: continue;
        }
    }
 lend_first_pass:

    return true;

    // We undefine these macros since they only make sense within the context of
    // this function.
#undef BAF_PUSH
#undef BAF_COMPUTE_OPERAND
#undef BAF_PUSH_FUNCTION
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

/*     while (true) { */
/*         if (kbhit() != 0 && cgetc() == KEYBOARD_STOP) { */
/*             puts("?ABORT"); */
/*             break; */
/*         } */

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

/*     lopcode_execute: */
/*         baf_interpreter_register_a = baf_interpreter_bfmem[baf_interpreter_bfmem_index]; */
/*         baf_interpreter_register_x = baf_interpreter_bfmem[baf_interpreter_bfmem_index+1]; */
/*         baf_interpreter_register_y = baf_interpreter_bfmem[baf_interpreter_bfmem_index+2]; */
/*         baf_execute(); */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index]   = baf_interpreter_register_a; */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index+1] = baf_interpreter_register_x; */
/*         baf_interpreter_bfmem[baf_interpreter_bfmem_index+2] = baf_interpreter_register_y; */
/*         goto lfinish_interpreter_cycle; */
/* } */

#endif // BASICFUCK_IMPLEMENTATION

#endif // _BASICFUCK_H
