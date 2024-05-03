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
 * Implementation of screen.h.
 */

#include "screen.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>



/**
 * Defined in header file.
 */
uchar screen_width  = 0
    , screen_height = 0;



/**
 * Defined in header file.
 */
uchar wrapped_cgetc() {
    uchar character = 0;
    do {
        character = cgetc();
    } while (0 == character);

    return character;
}

/**
 * Defined in header file.
 */
uchar blinking_cgetc() {
    uchar character;

    cursor(true);
    character = wrapped_cgetc();
    cursor(false);

    return character;
}



// Buffer used to convert numbers to strings.
uchar string_buffer[STRING_BUFFER_SIZE];

/*
 * Defined in header file.
 */
void utoa_fputs(const size_t digit_count, const uint value, const uchar radix) {
    size_t leading_zeros;

    (void)utoa(value, string_buffer, radix);

    if (0 != digit_count) {
        leading_zeros = digit_count - strlen(string_buffer);
        for (; leading_zeros > 0; --leading_zeros)
            (void)fputs("0", stdout);
    }

    (void)fputs(string_buffer, stdout);
};



/*
 * Defined in header file.
 */
bool is_control_character(const uchar character) {
#if defined(__CBM__)
    // PETSCII character set.
    return (character & 0x7F) < 0x20;
#elif defined(__ATARI__)
    // ATASCII character set.
    return (character >= 0x1B && character <= 0x1F)
        || (character >= 0x7D && character <= 0x7F)
        || (character >= 0x8B && character <= 0x8F)
        || (character >= 0xFD && character <= 0xFF);
#else
#error build target not supported
#endif
}
