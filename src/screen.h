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

/**
 * Screen utilities.
 */

#ifndef _SCREEN_H
#define _SCREEN_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>



/**
 * The width and height of the screen. Must be initalized at some point with
 * screensize(), or some other method, else they will be set to 0.
 */
extern uint8_t screen_width
             , screen_height;



/**
 * On some machines, cgetc() doesn't block like expected, and instead returns
 * immediately. This version adds in a check to ensure the blocking behavior.
 *
 * @return the value of the typed character.
 */
uint8_t wrapped_cgetc();

/**
 * Runs wrapped_cgetc() with a blinking cursor.
 *
 * To set a blinking cursor (easily,) you need to use the cursor() function from
 * conio.h, but it seems to error with "Illegal function call" or something when
 * used in a complex function, so I have it pulled out into this separate one.
 *
 * @return the value of the typed character.
 */
uint8_t blinking_cgetc();



// Size of buffer used to convert numbers to strings.
#define STRING_BUFFER_SIZE 6

/*
 * Uses utoa() to convert the value to a string and prints it with leading
 * zeros (no newline.)
 *
 * NOTE: that the buffer used for this function has the size of
 * STRING_BUFFER_SIZE, and does not check for overflows; be careful!
 *
 * @param digit_count - the number of digits to print. If the resulting number
 *                      has less than this number of digits it will be prepended
 *                      with zeros. A value of 0 disables this and simply prints
 *                      the number.
 * @param value - the value to print.
 * @param radix - the base to use to generate the number string.
 */
void utoa_fputs(const size_t digit_count, const uint16_t value, const uint8_t radix);



/**
 * Returns whether the given character is a screen control character.
 */
bool is_control_character(const uint8_t character);



#endif // _SCREEN_H
