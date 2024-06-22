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
 * BASICfuck JIT compiler.
 *
 * Preprocessor parameters:
 * - BASICFUCK_IMPLEMENTATION - Define in *one* source file to instantiate the
 *                              implementation.
 */

#ifndef _BASICFUCK_H
#define _BASICFUCK_H

#include <stdint.h>



/**
 * BASICfuck cell memory type.
 */
typedef uint8_t baf_cell_t;

/**
 * 6502 instruction opcodes type.
 */
typedef uint8_t baf_opcode_t;
#define BAF_ADC_ABSOLUTE   0x6D
#define BAF_ADC_IMMEDIATE  0x69
#define BAF_BCC            0x90
#define BAF_BCS            0xB0
#define BAF_BEQ            0xF0
#define BAF_BNE            0xD0
#define BAF_BVC            0x50
#define BAF_BVC            0x50
#define BAF_CLC            0x18
#define BAF_CMP_IMMEDIATE  0xC9
#define BAF_DEC_ABSOLUTE   0xCE
#define BAF_DEX            0xCA
#define BAF_INC_ABSOLUTE   0xEE
#define BAF_JSR            0x20
#define BAF_JMP_ABSOLUTE   0x4C
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

/**
 * BASICfuck compiler configuration.
 */
typedef struct {
    // Buffer to read BASICfuck instructions from.
    const uint8_t* read_buffer;
    // Buffer to write the generated 6502 machine code to.
    baf_opcode_t*  write_buffer;
    uint16_t       write_buffer_size;
    // A pointer to the variable to use as the pointer into cell memory in the
    // generated code.
    baf_cell_t**   cell_memory_pointer;
    // A pointer to the variable to use as the pointer into computer memory in
    // the generated code.
    uint8_t**      computer_memory_pointer;
} BAFCompiler;

typedef uint8_t BAFCompileResult;
#define BAF_COMPILE_SUCCESS           0
#define BAF_COMPILE_OUT_OF_MEMORY     1
#define BAF_COMPILE_UNTERMINATED_LOOP 2
/**
 * Compiles BASICfuck code to 6502 machine code.
 * @param compiler.
 * @return BAF_COMPILE_SUCCESS on success,
 *         BAF_COMPILE_OUT_OF_MEMORY if the program exceeded the size of the
 *         program memory,
 *         BAF_COMPILE_UNTERMINATED_LOOP if the program has an unterminated
 *         loop.
 */
BAFCompileResult baf_compile(const BAFCompiler* compiler);



#ifdef BASICFUCK_IMPLEMENTATION

#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include "screen.h"
#include "keyboard.h"

/**
 * Macro to pass two comma separated values as a single argument to a macro.
 */
#define BAF_MACRO_ARG(a, b) a, b

/**
 * Zeropage addresses.
 */
typedef uint8_t baf_zeropage_t;
/**
 * The first pointer register allocated by cc65. Used for indirect
 * addressing.
 */
static baf_zeropage_t baf_pointer1 = NULL;

/**
 * Pointer to the cc65 pushax register that pushes the A (low-byte) and X
 * (high-byte) registers on the software stack.
 */
static baf_opcode_t* baf_pushax = NULL;

/**
 * A pointer to the variable to use as the pointer into cell memory in the
 * the generated code.
 */
static baf_cell_t** baf_cell_memory_pointer = NULL;

/**
 * A pointer to the variable to use as the pointer into computer memory in
 * the generated code.
 */
static uint8_t** baf_computer_memory_pointer = NULL;

/**
 * The address in the read buffer to read instructions from. Must be intialized
 * prior to calls to the compilation leaf-functions, and is clobbered by them.
 */
static const uint8_t* baf_read_address = NULL;

/**
 * Increments a pointer to point at the next byte in memory.
 * @param type - the pointer's type.
 * @param value - the pointer's value.
 */
#define BAF_INCREMENT_POINTER(type, value) (type)(1 + (uint16_t)(value))

/**
 * The address in the write buffer to write compiled programs to. Must be
 * initialized prior to calls to the compilation leaf-functions, and is
 * clobbered by them.
 */
static baf_opcode_t* baf_write_address     = NULL;
static uint16_t      baf_write_buffer_size = 0;
/**
 * Pushes data to the write buffer.
 * @param type - the type of the data.
 * @param value - the data.
 * @param baf_write_address (global).
 */
#define BAF_PUSH(type, value)                                           \
    {                                                                   \
        *((type*)baf_write_address) = (value);                          \
        baf_write_address += sizeof(type);                              \
    }
/**
 * Pushes a function pointer to the write buffer.
 * @param return_type - the function's return type.
 * @param argument_types - the function's argument types.
 * @param pointer - the function pointer/address.
 * @param baf_write_address (global).
 */
#define BAF_PUSH_FUNCTION(return_type, argument_types, pointer)            \
    {                                                                      \
        *((return_type(**)(argument_types))baf_write_address) = (pointer); \
        baf_write_address += sizeof(return_type(*)(argument_types));       \
    }
// The first pass of the needs to insert placeholder instructions + a null
// address to mark where the jumps are for the second pass. Both of these
// opcodes are unused in the 6502.
#define BAF_JEQ_PLACEHOLDER 0xDA
#define BAF_JNE_PLACEHOLDER 0xDB

/**
 * Creates a function to push the instructions required to set up the
 * dereference of the given pointer. After calling, the pointer will be loaded
 * into the first pointer register and can be dereferenced with 'st<register>
 * (baf_pointer1),y' or 'ld<register> (baf_pointer1),y'.
 * Clobbers the <register> register.
 * @param register - the register to use to load the pointer into the zeropage.
 * @param address - the address of the pointer.
 * @param baf_write_address (global).
 * @param baf_pointer1 (global).
 */
#define BAF_PUSH_DEREFERNCE_FUNCTION(register)                               \
    static void baf_push_dereference_##register(uint8_t** address) {         \
        /*    ld<register>    address */                                     \
        BAF_PUSH(baf_opcode_t,   BAF_LD##register##_ABSOLUTE);               \
        BAF_PUSH(uint8_t**,      address);                                   \
        /*    st<register>    ptr1 */                                        \
        BAF_PUSH(baf_opcode_t,   BAF_ST##register##_ZEROPAGE);               \
        BAF_PUSH(baf_zeropage_t, baf_pointer1);                              \
        /*    ld<register>    address+1 */                                   \
        BAF_PUSH(baf_opcode_t,   BAF_LD##register##_ABSOLUTE);               \
        BAF_PUSH(uint8_t**,      BAF_INCREMENT_POINTER(uint8_t**, address)); \
        /*    st<register>    ptr1+1 */                                      \
        BAF_PUSH(baf_opcode_t,   BAF_ST##register##_ZEROPAGE);               \
        BAF_PUSH(baf_zeropage_t, 1+baf_pointer1);                            \
    }
BAF_PUSH_DEREFERNCE_FUNCTION(A)
BAF_PUSH_DEREFERNCE_FUNCTION(X)

// Global variables for execute instruction to pass variables back and forth
// with assembly.
static uint8_t  baf_execute_a            = 0;
static uint8_t  baf_execute_x            = 0;
static uint8_t  baf_execute_y            = 0;
static uint8_t* baf_execute_cmem_address = NULL;
/**
 * Runtime for execute '%' instruction.
 * TODO: switch from function parameters to global variable parameters.
 * @param cell_memory_address - the address of the current cell.
 * @param computer_memory_address - the address of the current location in
 *        computer memory.
 */
static void baf_execute(baf_opcode_t* cell_memory_address, uint8_t* computer_memory_address) {
    baf_execute_cmem_address = computer_memory_address;
    // Load current and next two cells into register variables.
    baf_execute_a = *cell_memory_address;
    baf_execute_x = cell_memory_address[1];
    baf_execute_y = cell_memory_address[2];

    // Overwrites address of subroutine to call in next assembly block with the
    // computer memory pointer's value.
    __asm__ volatile ("lda    %v",   baf_execute_cmem_address);
    __asm__ volatile ("sta    %g+1", ljsr                    );
    __asm__ volatile ("lda    %v+1", baf_execute_cmem_address);
    __asm__ volatile ("sta    %g+2", ljsr                    );
    // Executes subroutine.
    __asm__ volatile ("lda    %v",   baf_execute_a           );
    __asm__ volatile ("ldx    %v",   baf_execute_x           );
    __asm__ volatile ("ldy    %v",   baf_execute_y           );
 ljsr:
    __asm__ volatile ("jsr    %w",   NULL                    );
    // Retrieves resulting values.
    __asm__ volatile ("sta    %v",   baf_execute_a           );
    __asm__ volatile ("stx    %v",   baf_execute_x           );
    __asm__ volatile ("sty    %v",   baf_execute_y           );

    // Write back resulting values into cell memory.
    *cell_memory_address   = baf_execute_a;
    cell_memory_address[1] = baf_execute_x;
    cell_memory_address[2] = baf_execute_y;

    return;
    // If we don't include a jmp instruction, cc65, annoyingly, strips the label
    // from the resulting assembly.
    __asm__ volatile ("jmp    %g", ljsr);
}

/**
 * Checks and returns if the STOP key is pressed, meaning the program should be
 * aborted.
 */
static bool baf_abort_check() {
    if (0 != kbhit() && KEYBOARD_STOP == cgetc()) {
        puts("?ABORT");
        return true;
    }

    return false;
}

// TODO add bounds checking for instruction pushing.
// TODO add bounds checking for cell memory pointer.
/**
 * Performs the first pass of BASICfuck compilation, converting the text program
 * to machine code.
 * @param baf_read_address (global).
 * @param baf_write_address (global).
 * @param baf_pointer1 (global).
 * @return true if succeeded, false if ran out of memory.
 */
static bool baf_compile_first_pass() {
    uint8_t instruction = 0;
    // Used to count the number of times an instruction occurs.
    uint8_t operand = 0;

    /**
     * Computes the operand of counted instructions.
     * @param instruction.
     * @param baf_read_address (global).
     */
#define BAF_COMPUTE_OPERAND                         \
    {                                               \
        operand = 1;                                \
        while (instruction == *baf_read_address) {  \
            ++operand;                              \
            ++baf_read_address;                     \
        }                                           \
    }

    while (true) {
        instruction = *(baf_read_address++);

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

            // *cell_memory_pointer += operand;

            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            //    *cell_memory_pointer
            baf_push_dereference_A(baf_cell_memory_pointer);
            //    lda    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_LDA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
            //    clc
            BAF_PUSH(baf_opcode_t, BAF_CLC);
            //    adc    #operand
            BAF_PUSH(baf_opcode_t, BAF_ADC_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    sta    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_STA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
        } continue;

            // Moves the cell memory pointer to the right.
        case '<': {
            BAF_COMPUTE_OPERAND;

            // cell_memory_pointer -= operand;

            //    lda    cell_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, baf_cell_memory_pointer);
            //    sec
            BAF_PUSH(baf_opcode_t, BAF_SEC);
            //    sbc    #operand
            BAF_PUSH(baf_opcode_t, BAF_SBC_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    sta    cell_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, baf_cell_memory_pointer);
            //    bcs    lno_borrow
            BAF_PUSH(baf_opcode_t, BAF_BCS);
            BAF_PUSH(uint8_t,      3);
            //    dec    cell_memory_pointer+1
            BAF_PUSH(baf_opcode_t, BAF_DEC_ABSOLUTE);
            BAF_PUSH( baf_cell_t**
                    , BAF_INCREMENT_POINTER(baf_cell_t**, baf_cell_memory_pointer));
            // lno_borrow:
        } continue;

            // Moves the cell memory pointer to the right.
        case '>': {
            BAF_COMPUTE_OPERAND;

            // cell_memory_pointer += operand;

            //    lda    #operand
            BAF_PUSH(baf_opcode_t, BAF_LDA_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    clc
            BAF_PUSH(baf_opcode_t, BAF_CLC);
            //    adc    cell_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_ADC_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, baf_cell_memory_pointer);
            //    sta    cell_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**, baf_cell_memory_pointer);
            //    bcc    lno_carry
            BAF_PUSH(baf_opcode_t, BAF_BCC);
            BAF_PUSH(uint8_t,      3);
            //    inc    cell_memory_pointer+1
            BAF_PUSH(baf_opcode_t, BAF_INC_ABSOLUTE);
            BAF_PUSH( baf_cell_t**
                    , BAF_INCREMENT_POINTER(baf_cell_t**, baf_cell_memory_pointer));
            // lno_carry:
        } continue;

            // Prints the value of the current cell as a character.
        case '.': {
            // (void)putchar(*cell_memory_pointer);

            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            //    *cell_memory_pointer
            baf_push_dereference_A(baf_cell_memory_pointer);
            //    lda    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_LDA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
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
            // uint8_t character = s_wrapped_cgetc();
            // if (KEYBOARD_STOP == character) {
            //     (void)puts("?ABORT");
            //     return;
            // }
            // *baf_cell_memory_pointer = character;

            //    jsr    _s_wrapped_cgetc
            BAF_PUSH(baf_opcode_t, BAF_JSR);
            BAF_PUSH_FUNCTION(uint8_t, void, &s_wrapped_cgetc);
            //    cmp    #KEYBOARD_STOP
            BAF_PUSH(baf_opcode_t, BAF_CMP_IMMEDIATE);
            BAF_PUSH(uint8_t,      KEYBOARD_STOP);
            //    bne    ldont_abort
            BAF_PUSH(baf_opcode_t, BAF_BNE);
            BAF_PUSH(uint8_t,      8);
            //    lda    #>(abort_string);
            BAF_PUSH(baf_opcode_t, BAF_LDA_IMMEDIATE);
            BAF_PUSH(uint8_t,      (uint8_t)"?ABORT");
            //    ldx    #<(abort_string);
            BAF_PUSH(baf_opcode_t, BAF_LDX_IMMEDIATE);
            BAF_PUSH(uint8_t,      (uint8_t)((int16_t)"?ABORT">>8));
            //    jsr    _puts
            BAF_PUSH(baf_opcode_t, BAF_JSR);
            BAF_PUSH_FUNCTION(int, const char*, &puts);
            //    rts
            BAF_PUSH(baf_opcode_t, BAF_RTS);
            // ldont_abort:
            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            //    *cell_memory_pointer
            baf_push_dereference_X(baf_cell_memory_pointer);
            //    sta    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_STA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
        } continue;

            // '[' Jumps past the corresponding ']' if the current cell is 0.
            // ']' Jumps past the corresponding '[' if the current cell is not 0.
        case '[':
        case ']': {
            // if (0 == *cell_memory_pointer) {
            //     if (true == baf_abort_check) return;
            //     goto lmatching_jne;
            // }
            // OR
            // if (0 != *cell_memory_pointer) goto lmatching_jeq;

            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            //    *cell_memory_pointer
            baf_push_dereference_A(baf_cell_memory_pointer);
            //    lda    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_LDA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
            //    cmp    #$00
            BAF_PUSH(baf_opcode_t, BAF_CMP_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            if ('[' == instruction) {
                //    bne    lno_jump
                BAF_PUSH(baf_opcode_t, BAF_BNE);
                BAF_PUSH(uint8_t,      3);
                //    jmp    lmatching_jne
                BAF_PUSH(baf_opcode_t, BAF_JEQ_PLACEHOLDER);
            } else {
                //    beq    lno_jump
                BAF_PUSH(baf_opcode_t, BAF_BEQ);
                BAF_PUSH(uint8_t,      3);
                //    jmp    lmatching_jeq
                BAF_PUSH(baf_opcode_t, BAF_JNE_PLACEHOLDER);
            }
            BAF_PUSH(baf_opcode_t**, NULL);
            // lno_jump:
            if ('[' == instruction) {
                //    jsr    _baf_abort_check
                BAF_PUSH(baf_opcode_t, BAF_JSR);
                BAF_PUSH_FUNCTION(bool, void, &baf_abort_check);
                //    cmp    #true
                BAF_PUSH(baf_opcode_t, BAF_CMP_IMMEDIATE);
                BAF_PUSH(uint8_t,      true)
                //    bne    lno_abort
                BAF_PUSH(baf_opcode_t, BAF_BNE);
                BAF_PUSH(uint8_t,      1);
                //    rts
                BAF_PUSH(baf_opcode_t, BAF_RTS);
                // lno_abort:
            }
        } continue;

            // Writes the value at the computer memory pointer's address to the
            // current cell.
        case '@': {
            // *cell_memory_pointer = *computer_memory_pointer;

            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            //    *computer_memory_pointer
            baf_push_dereference_A(baf_computer_memory_pointer);
            //    lda    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_LDA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
            //    *computer_memory_pointer
            baf_push_dereference_X(baf_cell_memory_pointer);
            //    sta    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_STA_INDIRECT_Y);
            BAF_PUSH(baf_cell_t,   baf_pointer1);
        } continue;

            // Writes the value of the current cell to the computer memory
            // pointer's address.
        case '*': {
            // *computer_memory_pointer = *cell_memory_pointer;

            //    ldy    #$00
            BAF_PUSH(baf_opcode_t, BAF_LDY_IMMEDIATE);
            BAF_PUSH(uint8_t,      0);
            //    *cell_memory_pointer
            baf_push_dereference_A(baf_cell_memory_pointer);
            //    lda    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_LDA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
            //    *computer_memory_pointer
            baf_push_dereference_X(baf_computer_memory_pointer);
            //    sta    (ptr1),y
            BAF_PUSH(baf_opcode_t, BAF_STA_INDIRECT_Y);
            BAF_PUSH(uint8_t,      baf_pointer1);
        } continue;

            // Moves the computer memory pointer to the left.
        case '(': {
            BAF_COMPUTE_OPERAND;

            // computer_memory_pointer -= operand;

            //    lda    computer_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_LDA_ABSOLUTE);
            BAF_PUSH(uint8_t**,    baf_computer_memory_pointer);
            //    sec
            BAF_PUSH(baf_opcode_t, BAF_SEC);
            //    sbc    #operand
            BAF_PUSH(baf_opcode_t, BAF_SBC_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    sta    computer_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(uint8_t**,    baf_computer_memory_pointer);
            //    bcs    lno_borrow
            BAF_PUSH(baf_opcode_t, BAF_BCS);
            BAF_PUSH(uint8_t,      3);
            //    dec    computer_memory_pointer+1
            BAF_PUSH(baf_opcode_t, BAF_DEC_ABSOLUTE);
            BAF_PUSH( uint8_t**
                    , BAF_INCREMENT_POINTER(uint8_t**, baf_computer_memory_pointer));
            // lno_borrow:
        } continue;

            // Moves the computer memory pointer to the right.
        case ')': {
            BAF_COMPUTE_OPERAND;

            // computer_memory_pointer += operand;

            //    lda    #operand
            BAF_PUSH(baf_opcode_t, BAF_LDA_IMMEDIATE);
            BAF_PUSH(uint8_t,      operand);
            //    clc
            BAF_PUSH(baf_opcode_t, BAF_CLC);
            //    adc    computer_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_ADC_ABSOLUTE);
            BAF_PUSH(uint8_t**,    baf_computer_memory_pointer);
            //    sta    computer_memory_pointer
            BAF_PUSH(baf_opcode_t, BAF_STA_ABSOLUTE);
            BAF_PUSH(uint8_t**,    baf_computer_memory_pointer);
            //    bcc    lno_carry
            BAF_PUSH(baf_opcode_t, BAF_BCC);
            BAF_PUSH(uint8_t,      3);
            //    inc    computer_memory_pointer+1
            BAF_PUSH(baf_opcode_t, BAF_INC_ABSOLUTE);
            BAF_PUSH( uint8_t**
                    , BAF_INCREMENT_POINTER(uint8_t**, baf_computer_memory_pointer));
            // lno_carry:
        } continue;

            // Loads the value of the current and next 2 cells into the A, X,
            // and Y registers respectively, and then executes the current
            // location in computer memory as a subroutine.
        case '%': {
            // baf_execute(cell_memory_pointer, computer_memory_pointer);

            //    lda    cell_memory_pointer
            BAF_PUSH(baf_opcode_t,  BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**,  baf_cell_memory_pointer);
            //    ldx    cell_memory_pointer+1
            BAF_PUSH(baf_opcode_t,  BAF_LDX_ABSOLUTE);
            BAF_PUSH( baf_cell_t**
                    , BAF_INCREMENT_POINTER(baf_cell_t**, baf_cell_memory_pointer));
            //    jsr    pushax
            BAF_PUSH(baf_opcode_t,  BAF_JSR);
            BAF_PUSH(baf_opcode_t*, baf_pushax);
            //    lda    computer_memory_pointer
            BAF_PUSH(baf_opcode_t,  BAF_LDA_ABSOLUTE);
            BAF_PUSH(baf_cell_t**,  baf_computer_memory_pointer);
            //    ldx    computer_memory_pointer+1
            BAF_PUSH(baf_opcode_t,  BAF_LDX_ABSOLUTE);
            BAF_PUSH( baf_cell_t**
                    , BAF_INCREMENT_POINTER(baf_cell_t**, baf_computer_memory_pointer));
            //    jsr    _baf_execute
            BAF_PUSH(baf_opcode_t,  BAF_JSR);
            BAF_PUSH_FUNCTION(void, BAF_MACRO_ARG(baf_opcode_t*,uint8_t*), &baf_execute);
        } continue;

            // Ignores non-instructions.
        default: continue;
        }
    }
 lend_first_pass:

    return true;

    // We undefine this macro since it only makes sense within the context of
    // this function.
#undef BAF_COMPUTE_OPERAND
}

/**
 * Performs the second pass of BASICfuck compilation, calculating the addresses
 * for jump instructions.
 * @param baf_write_address (global).
 * @param baf_write_buffer_size (global).
 * @return true if succeeded, false if there is an unterminated loop.
 */
static bool baf_compile_second_pass() {
    baf_opcode_t* seek_address = NULL;
    uint8_t       loop_depth   = 0;

    baf_opcode_t* write_buffer_start = baf_write_address;
    // We subtract 3 because JEQ and JNE take up 3 bytes each.
    baf_opcode_t* write_buffer_end = baf_write_address + baf_write_buffer_size - 3;

    // Calculates addresses of paring JEQ and JNE instructions.
    while (baf_write_address < write_buffer_end) {
        switch (*baf_write_address) {
        case BAF_JEQ_PLACEHOLDER: {
            // If there is no null pointer after the placeholder, then this is
            // just some random value.
            if (NULL != *BAF_INCREMENT_POINTER(baf_opcode_t**, baf_write_address)) {
                ++baf_write_address;
                continue;
            }

            // Loop to find paring JNE.
            seek_address = 3 + baf_write_address;
            loop_depth   = 1;
            while (seek_address < write_buffer_end) {
                switch (*seek_address) {
                case BAF_JEQ_PLACEHOLDER: {
                    if (NULL != *BAF_INCREMENT_POINTER(baf_opcode_t**, seek_address)) {
                        ++seek_address;
                        continue;
                    }
                    ++loop_depth;
                    seek_address += 3;
                } break;
                case BAF_JNE_PLACEHOLDER: {
                    if (NULL != *BAF_INCREMENT_POINTER(baf_opcode_t**, seek_address)) {
                        ++seek_address;
                        continue;
                    }
                    --loop_depth;
                    seek_address += 3;
                } break;
                default: ++seek_address; continue;
                }

                if (0 == loop_depth) {
                    // Sets JEQ instruction to jump to accompanying JNE.
                    *BAF_INCREMENT_POINTER(baf_opcode_t**, baf_write_address) = seek_address;
                    // Sets JNE instruction to jump to accompanying JEQ.
                    // Add/subtract 3 from previous switch.
                    *BAF_INCREMENT_POINTER(baf_opcode_t**, seek_address-3) = 3 + baf_write_address;
                    break;
                }
            }

            baf_write_address += 3;
        } continue;

        default: ++baf_write_address; continue;
        }
    }

    baf_write_address = write_buffer_start;

    // Changes placeholder instructions to JMP and asserts that all JEQs have a
    // pairing JNE and vice-versa.
    while (baf_write_address < write_buffer_end) {
        switch (*baf_write_address) {
        case BAF_JEQ_PLACEHOLDER: {
            // If there is a null pointer after the placeholder, then there are
            // unmatched JNE and JEQ.
            if (NULL == *BAF_INCREMENT_POINTER(baf_opcode_t**, baf_write_address)) {
                return false;
            }

            *baf_write_address = BAF_JMP_ABSOLUTE;
            baf_write_address += 3;
        } break;

        case BAF_JNE_PLACEHOLDER: {
            // If there is a null pointer after the placeholder, then there are
            // unmatched JNE and JEQ.
            if (NULL == *BAF_INCREMENT_POINTER(baf_opcode_t**, baf_write_address)) {
                return false;
            }

            *baf_write_address = BAF_JMP_ABSOLUTE;
            baf_write_address += 3;
        } break;

        default: ++baf_write_address; break;
        }
    }

    return true;
}

BAFCompileResult baf_compile(const BAFCompiler* compiler) {
    // Global variable function parameter passing.
    baf_read_address            = compiler->read_buffer;
    baf_write_address           = compiler->write_buffer;
    baf_write_buffer_size       = compiler->write_buffer_size;
    baf_cell_memory_pointer     = compiler->cell_memory_pointer;
    baf_computer_memory_pointer = compiler->computer_memory_pointer;
    // We can't access assembly labels directly from C, so instead we use a
    // little inline assembly to copy them to C variables.
    __asm__ ("lda    #<(ptr1)  "               );
    __asm__ ("sta    %v        ", baf_pointer1 );
    __asm__ ("lda    #<(pushax)"               );
    __asm__ ("sta    %v        ", baf_pushax   );
    __asm__ ("lda    #>(pushax)"               );
    __asm__ ("sta    %v+1      ", baf_pushax   );

    // We clear the memory before compilation to prevent left-over code from
    // being interpreted as jumps by the second pass.
    (void)memset(baf_write_address, 0, baf_write_buffer_size);

    if (!baf_compile_first_pass())
        return BAF_COMPILE_OUT_OF_MEMORY;

    // Global variable function parameter passing.
    baf_write_address = compiler->write_buffer;

    if (!baf_compile_second_pass())
        return BAF_COMPILE_UNTERMINATED_LOOP;

    return BAF_COMPILE_SUCCESS;
}

#endif // BASICFUCK_IMPLEMENTATION

#endif // _BASICFUCK_H
