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
 * General utilites
 */

#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>

#include "types.h"



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
void utoa_fputs(const size_t digit_count, const uint value, const uchar radix);



#endif // _UTILS_H
