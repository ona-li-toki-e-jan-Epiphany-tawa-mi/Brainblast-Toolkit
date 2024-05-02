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
 * Implementation of text_buffer.h
 */

#include "text_buffer.h"

#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "keyboard.h"
#include "screen.h"



/**
 * A stack to store previous inputs so they can be recalled.
 */
uchar history_stack[HISTORY_STACK_SIZE];
uint  history_stack_index = 0;

/**
 * Increments the history stack index and loops it around if it goes out of
 * bounds.
 */
void increment_stack_index() {
    ++history_stack_index;
    if (history_stack_index >= HISTORY_STACK_SIZE)
        history_stack_index = 0;
}

/**
 * Decrements the history stack index and loops it around if it goes out of
 * bounds.
 */
void decrement_stack_index() {
    if (0 == history_stack_index) {
        history_stack_index = HISTORY_STACK_SIZE - 1;
    } else {
        --history_stack_index;
    }
}



// Global variables for passing parameters between functions.
uchar* current_buffer;                            // The buffer currently being edited.
uchar  buffer_cursor;                             // The location of the user's cursor inside the buffer.
uchar  input_size;                                // How much of the buffer is taken up by the text typed by the user.

/**
 * Saves the given null-terminated text buffer to the history stack for later
 * recollection.
 *
 * @param current_buffer (global) - the null-terminated buffer to save.
 */
void save_buffer() {
    uchar character;
    uchar buffer_index = 0;

    do {
        character = current_buffer[buffer_index];
        history_stack[history_stack_index] = character;

        increment_stack_index();
        ++buffer_index;
    } while (NULL != character);
}

/**
 * Recalls the previous input if foward_recall is false, else recalls the next
 * input from the history buffer.
 *
 * @param current_buffer (global) - the null-terminated buffer to write to.
 * @param buffer_cursor (global) - the user's cursor inside the buffer.
 * @param input_size (global) - the amount of space used inside the buffer.
 * @param forward_recall - which direction to move in in the history buffer.
 */
void recall_buffer(const bool forward_recall) {
    uchar character;
    uint  final_history_index = history_stack_index;

    // Moves forwards or backwards to the next block in the history stack.
    if (forward_recall) {
        while (NULL != history_stack[history_stack_index])
            increment_stack_index();
        increment_stack_index();

    } else {
        decrement_stack_index();
        do {
            decrement_stack_index();
        } while (NULL != history_stack[history_stack_index]);
        increment_stack_index();
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
    for (; buffer_cursor < input_size; ++buffer_cursor)
        (void)putchar(KEYBOARD_RIGHT);
    // Clears visual buffer.
    while (buffer_cursor > 0) {
        (void)putchar(KEYBOARD_BACKSPACE);
        --buffer_cursor;
    }

    // Reads from history buffer into buffer.
    while (true) {
        character                     = history_stack[history_stack_index];
        current_buffer[buffer_cursor] = character;
        (void)putchar(character);

        if (NULL == character)
            break;

        increment_stack_index();
        ++buffer_cursor;
    }
    input_size = buffer_cursor;

    // Restores history stack index to the start of the current block so that
    // moving forwards and backwords works properly.
    history_stack_index = final_history_index;
}



/**
 * Defined in header file.
 */
void edit_buffer(uchar *const buffer, uchar buffer_max_index) {
    uchar new_cursor;
    uchar key;

    // Ensures the last byte is reserved for a null-terminator.
    buffer_max_index -= 1;

    // Loads parameters into global variables for this and other functions.
    current_buffer = buffer;
    buffer_cursor  = 0;
    input_size     = 0;


    while (true) {
        key = blinking_cgetc();

        switch (key) {
        // Finalizes the buffer and exits from this function.
        case KEYBOARD_ENTER:
            current_buffer[input_size] = NULL;                  // write out null-terminator.
            for (; buffer_cursor < input_size; ++buffer_cursor) // navigate to then end of buffer if neccesary.
                (void)putchar(KEYBOARD_RIGHT);
            (void)putchar('\n');

            goto lquit_editing_buffer;

        // "Clears" the input buffer and exits from this function.
        case KEYBOARD_STOP:
            current_buffer[0] = NULL;             // write null terminator to start of buffer, "clearing" it.
            (void)putchar('\n');

            goto lquit_editing_buffer;

        // Clears the screen and input buffer.
        case KEYBOARD_CLEAR:
            buffer_cursor = 0;
            input_size    = 0;
            clrscr();
            break;

        // Deletes characters from the buffer.
        case KEYBOARD_BACKSPACE:
            if (buffer_cursor == 0)
                break;

            (void)putchar(KEYBOARD_BACKSPACE);    // display backspace.
            // Shifts characters in buffer to the left, overwiting the deleted
            // character.
            (void)memmove(current_buffer+buffer_cursor - 1, current_buffer+buffer_cursor, input_size - buffer_cursor);
            --input_size;
            --buffer_cursor;

            break;

        // Handles arrow keys, moving through the buffer.
        case KEYBOARD_LEFT:
            if (buffer_cursor > 0) {
                --buffer_cursor;
                (void)putchar(KEYBOARD_LEFT);
            }
            break;

        case KEYBOARD_RIGHT:
            if (buffer_cursor < input_size) {
                ++buffer_cursor;
                (void)putchar(KEYBOARD_RIGHT);
            }
            break;

        case KEYBOARD_UP:
            // Navigates to the next line up, or to the start of the buffer, if
            // there is no line there.
            new_cursor = buffer_cursor > screen_width ? buffer_cursor - screen_width : 0;
            for (; buffer_cursor > new_cursor; --buffer_cursor)
                (void)putchar(KEYBOARD_LEFT);

            break;

        case KEYBOARD_DOWN:
            // Navigates to the next line down, or to the end of the filled
            // buffer, if there is no line there.
            new_cursor = (input_size - buffer_cursor) > screen_width ? buffer_cursor + screen_width : input_size;
            for (; buffer_cursor < new_cursor; ++buffer_cursor)
                (void)putchar(KEYBOARD_RIGHT);

            break;

        // Handles HOME, moving to the start of the buffer.
        case KEYBOARD_HOME:
            for (; buffer_cursor > 0; --buffer_cursor)
                (void)putchar(KEYBOARD_LEFT);
            break;

        // Handles INST, inserting characters into the buffer.
        case KEYBOARD_INSERT:
            if (input_size > buffer_max_index || buffer_cursor == input_size)
                break;

            (void)putchar(KEYBOARD_INSERT);       // display insertion.
            // Shifts characters in buffer to the right, making space for the
            // new one.
            (void)memmove(current_buffer+buffer_cursor + 1, current_buffer+buffer_cursor, input_size - buffer_cursor);
            input_size++;
            current_buffer[buffer_cursor] = ' ';

            break;

        // Handles function keys, navigating through the history buffer.
        case KEYBOARD_F1:
            recall_buffer(false);
            break;

        case KEYBOARD_F2:
            recall_buffer(true);
            break;

        // Handles typing characters.
        default:
            if ((key & 0x7F) < 0x20)              // filter out unhandled control characters.
                break;
            if (buffer_cursor > buffer_max_index)
                break;

            if (buffer_cursor == input_size)      // increase buffer size if adding characters to the end.
                ++input_size;
            current_buffer[buffer_cursor] = key;
            ++buffer_cursor;
            (void)putchar(key);
        }
    }
 lquit_editing_buffer:

    save_buffer();
    return;
}
