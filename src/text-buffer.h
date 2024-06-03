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
 * Editable text buffers.
 *
 * Preprocessor parameters:
 * - TEXT_BUFFER_IMPLEMENTATION - Define in *one* source file to instantiate the
 *                                implementation.
 * - TEXT_BUFFER_DISABLE_HISTORY - If defined, strips out history stack code.
 */

#ifndef _TEXT_BUFFER_H
#define _TEXT_BUFFER_H

#include <stdint.h>



/**
 * Creates an editable text buffer, starting from the current position on the
 * screen, and stores what the user typed into the given buffer with a
 * null-terminator.
 *
 * The cursor on the screen will be moved to the line after the filled portion
 * of the text buffer once done.
 *
 * @param buffer - the buffer to store the typed characters into.
 * @param buffer_max_index - the maxiumum addressable index of the buffer.
 * @param history_stack - the stack to store previous user inputs in.
 * @param history_stack_size - the size of the history stack.
 * @param history_stack_index - a pointer to the current index into the history
 *        stack.
 */
#ifndef TEXT_BUFFER_DISABLE_HISTORY
void tb_edit_buffer(uint8_t *const buffer, uint8_t buffer_max_index, uint8_t *const history_stack, const uint16_t history_stack_size, uint16_t *const history_stack_index);
#else // TEXT_BUFFER_DISABLE_HISTORY
void tb_edit_buffer(uint8_t *const buffer, uint8_t buffer_max_index);
#endif // #else



#ifdef TEXT_BUFFER_IMPLEMENTATION

#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "keyboard.h"
#include "screen.h"

// Global variables for passing parameters between functions.
static uint8_t* tb_buffer;                        // The buffer currently being edited.
static uint8_t  tb_cursor;                        // The location of the user's cursor inside the buffer.
static uint8_t  tb_input_size;                    // How much of the buffer is taken up by the text typed by the user.

#ifndef TEXT_BUFFER_DISABLE_HISTORY
// Global variables for history stack to store and recall previous user inputs.
static uint8_t*  tb_history_stack;
static uint16_t  tb_history_stack_size;
static uint16_t* tb_history_stack_index;

/**
 * Increments the history stack index and loops it around if it goes out of
 * bounds.
 *
 * @param tb_history_stack_index (global) - current index into the history
 *                                          stack.
 * @param tb_history_stack_size (global) - the size of the history stack.
 */
static void tb_increment_stack_index() {
    ++(*tb_history_stack_index);
    if (*tb_history_stack_index >= tb_history_stack_size)
        *tb_history_stack_index = 0;
}

/**
 * Decrements the history stack index and loops it around if it goes out of
 * bounds.
 *
 * @param tb_history_stack_index (global) - current index into the history
 *                                          stack.
 * @param tb_history_stack_size (global) - the size of the history stack.
 */
static void tb_decrement_stack_index() {
    if (0 == *tb_history_stack_index) {
        *tb_history_stack_index = tb_history_stack_size - 1;
    } else {
        --(*tb_history_stack_index);
    }
}

/**
 * Saves the given null-terminated text buffer to the history stack for later
 * recollection.
 *
 * @param tb_current_buffer (global) - the null-terminated buffer to save.
 * @param tb_history_stack (global) - pointer to the history stack.
 * @param tb_history_stack_index (global) - current index into the history
 *                                          stack.
 */
static void tb_save_buffer() {
    uint8_t character;
    uint8_t buffer_index = 0;

    if (NULL == tb_buffer[buffer_index])
        return;

    do {
        character = tb_buffer[buffer_index];
        tb_history_stack[*tb_history_stack_index] = character;

        tb_increment_stack_index();
        ++buffer_index;
    } while (NULL != character);
}

/**
 * Recalls the previous input if foward_recall is false, else recalls the next
 * input from the history buffer.
 *
 * @param tb_current_buffer (global) - the null-terminated buffer to write to.
 * @param tb_buffer_cursor (global) - the user's cursor inside the buffer.
 * @param tb_input_size (global) - the amount of space used inside the buffer.
 * @param tb_forward_recall - which direction to move in in the history buffer.
 * @param tb_history_stack (global) - pointer to the history stack.
 * @param tb_history_stack_index (global) - current index into the history
 *        stack.
 */
static void tb_recall_buffer(const bool forward_recall) {
    uint8_t  character;
    uint16_t final_history_index = *tb_history_stack_index;

    // Moves forwards or backwards to the next block in the history stack.
    if (forward_recall) {
        while (NULL != tb_history_stack[*tb_history_stack_index])
            tb_increment_stack_index();
        tb_increment_stack_index();

    } else {
        tb_decrement_stack_index();
        do {
            tb_decrement_stack_index();
        } while (NULL != tb_history_stack[*tb_history_stack_index]);
        tb_increment_stack_index();
    }

    // If a NULL is found at the next block, that means that it's at the end of
    // the history buffer.
    if (NULL == tb_history_stack[*tb_history_stack_index]) {
        *tb_history_stack_index = final_history_index;
        return;
    }
    // Preservers the index of this block as we will stay here in case of
    // succesive movements through the history.
    final_history_index = *tb_history_stack_index;

    // Navigates visual cursor to the end of the buffer.
    for (; tb_cursor < tb_input_size; ++tb_cursor)
        (void)putchar(KEYBOARD_RIGHT);
    // Clears visual buffer.
    while (tb_cursor > 0) {
        (void)putchar(KEYBOARD_BACKSPACE);
        --tb_cursor;
    }

    // Reads from history buffer into buffer.
    while (true) {
        character            = tb_history_stack[*tb_history_stack_index];
        tb_buffer[tb_cursor] = character;
        (void)putchar(character);

        if (NULL == character)
            break;

        tb_increment_stack_index();
        ++tb_cursor;
    }
    tb_input_size = tb_cursor;

    // Restores history stack index to the start of the current block so that
    // moving forwards and backwords works properly.
    *tb_history_stack_index = final_history_index;
}
#endif // TEXT_BUFFER_DISABLE_HISTORY

#ifndef TEXT_BUFFER_DISABLE_HISTORY
void tb_edit_buffer(uint8_t *const buffer, uint8_t buffer_max_index, uint8_t *const history_stack, const uint16_t history_stack_size, uint16_t *const history_stack_index) {
#else // TEXT_BUFFER_DISABLE_HISTORY
void tb_edit_buffer(uint8_t *const buffer, uint8_t buffer_max_index) {
#endif // #else
    uint8_t new_cursor;
    uint8_t key;

    // Ensures the last byte is reserved for a null-terminator.
    buffer_max_index -= 1;

    // Loads parameters into global variables for this and other functions.
    tb_buffer              = buffer;
    tb_cursor              = 0;
    tb_input_size          = 0;
#ifndef TEXT_BUFFER_DISABLE_HISTORY
    tb_history_stack       = history_stack;
    tb_history_stack_size  = history_stack_size;
    tb_history_stack_index = history_stack_index;
#endif // TEXT_BUFFER_DISABLE_HISTORY



    while (true) {
        key = s_blinking_cgetc();

        switch (key) {
        // Finalizes the buffer and exits from this function.
        case KEYBOARD_ENTER:
            tb_buffer[tb_input_size] = NULL;               // write out null-terminator.
            for (; tb_cursor < tb_input_size; ++tb_cursor) // navigate to then end of buffer if neccesary.
                (void)putchar(KEYBOARD_RIGHT);
            (void)putchar('\n');

            goto lquit_editing_buffer;

        // "Clears" the input buffer and exits from this function.
        case KEYBOARD_STOP:
            tb_buffer[0] = NULL;                  // write null terminator to start of buffer, "clearing" it.
            (void)putchar('\n');

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

            (void)putchar(KEYBOARD_BACKSPACE);    // display backspace.
            // Shifts characters in buffer to the left, overwiting the deleted
            // character.
            (void)memmove(tb_buffer+tb_cursor - 1, tb_buffer+tb_cursor, tb_input_size - tb_cursor);
            --tb_input_size;
            --tb_cursor;

            break;

        // Handles arrow keys, moving through the buffer.
        case KEYBOARD_LEFT:
            if (tb_cursor > 0) {
                --tb_cursor;
                (void)putchar(KEYBOARD_LEFT);
            }
            break;

        case KEYBOARD_RIGHT:
            if (tb_cursor < tb_input_size) {
                ++tb_cursor;
                (void)putchar(KEYBOARD_RIGHT);
            }
            break;

        case KEYBOARD_UP:
            // Navigates to the next line up, or to the start of the buffer, if
            // there is no line there.
            new_cursor = tb_cursor > s_width ? tb_cursor - s_width : 0;
            for (; tb_cursor > new_cursor; --tb_cursor)
                (void)putchar(KEYBOARD_LEFT);

            break;

        case KEYBOARD_DOWN:
            // Navigates to the next line down, or to the end of the filled
            // buffer, if there is no line there.
            new_cursor = (tb_input_size - tb_cursor) > s_width ? tb_cursor + s_width : tb_input_size;
            for (; tb_cursor < new_cursor; ++tb_cursor)
                (void)putchar(KEYBOARD_RIGHT);

            break;

        // Handles HOME, moving to the start of the buffer.
        case KEYBOARD_HOME:
            for (; tb_cursor > 0; --tb_cursor)
                (void)putchar(KEYBOARD_LEFT);
            break;

        // Handles INST, inserting characters into the buffer.
        case KEYBOARD_INSERT:
            if (tb_input_size > buffer_max_index || tb_cursor == tb_input_size)
                break;

            (void)putchar(KEYBOARD_INSERT);       // display insertion.
            // Shifts characters in buffer to the right, making space for the
            // new one.
            (void)memmove(tb_buffer+tb_cursor + 1, tb_buffer+tb_cursor, tb_input_size - tb_cursor);
            tb_input_size++;
            tb_buffer[tb_cursor] = ' ';

            break;

#ifndef TEXT_BUFFER_DISABLE_HISTORY
        // Handles function keys, navigating through the history buffer.
        case KEYBOARD_F1:
            tb_recall_buffer(false);
            break;

        case KEYBOARD_F2:
            tb_recall_buffer(true);
            break;
#endif // TEXT_BUFFER_DISABLE_HISTORY

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
            (void)putchar(key);
        }
    }
 lquit_editing_buffer:

#ifndef TEXT_BUFFER_DISABLE_HISTORY
    tb_save_buffer();
#endif // TEXT_BUFFER_DISABLE_HISTORY
    return;
}

#endif // TEXT_BUFFER_IMPLEMENTATION

#endif // _TEXT_BUFFER_H
