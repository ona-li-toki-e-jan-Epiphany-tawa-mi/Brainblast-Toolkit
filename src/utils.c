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
 * Implementation of utils.h.
 */

#include "utils.h"

#include <stdlib.h>
#include <string.h>



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
