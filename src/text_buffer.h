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
 * Interactive text buffers.
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
 * @param history_stack - the stack to store previous user inputs in. Set to
 *        NULL to disable. The values of history_stack_size and
 *        history_stack_index do not matter if NULL.
 * @param history_stack_size - the size of the history stack.
 * @param history_stack_index - a pointer to the current index into the history
 *        stack.
 */
void edit_buffer(uint8_t *const buffer, uint8_t buffer_max_index, uint8_t *const history_stack, const uint16_t history_stack_size, uint16_t *const history_stack_index);



#endif // _TEXT_BUFFER_H
