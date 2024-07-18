/*
 * zlib license
 *
 * Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
 *
 * This software is provided ‘as-is’, without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */

/**
 * Abstract keyboard interface for the computers supported by cc65.
 *
 * Preprocessor returns:
 * - KEYBOARD_UP        - up arrow key.
 * - KEYBOARD_DOWN      - down arrow key.
 * - KEYBOARD_LEFT      - left arrow key.
 * - KEYBOARD_RIGHT     - right arrow key.
 * - KEYBOARD_BACKSPACE - backspace/delete key.
 * - KEYBOARD_INSERT    - insert key.
 * - KEYBOARD_ENTER     - enter/return key.
 * - KEYBOARD_STOP      - stop key.
 * - KEYBOARD_HOME      - home key.
 * - KEYBOARD_CLEAR     - clear key.
 * - KEYBOARD_F1        - function key 1.
 * - KEYBOARD_F2        - function key 2.
 * - KEYBOARD_*_STRING  - human readable key name. Not present for all keys.
 *
 * If a key is not avalible on the system it will be mapped to some other key.
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H



#if defined(__CBM__)
#  include <cbm.h>
#  define KEYBOARD_UP          CH_CURS_UP
#  define KEYBOARD_DOWN        CH_CURS_DOWN
#  define KEYBOARD_LEFT        CH_CURS_LEFT
#  define KEYBOARD_RIGHT       CH_CURS_RIGHT
#  define KEYBOARD_BACKSPACE   CH_DEL
#  define KEYBOARD_INSERT      CH_INS
#  define KEYBOARD_ENTER       CH_ENTER
#  define KEYBOARD_STOP        CH_STOP
#  define KEYBOARD_STOP_STRING "STOP"
#  define KEYBOARD_HOME        CH_HOME
#  define KEYBOARD_HOME_STRING "HOME"
   // Not defined in cbm.h, but the same for all CBM computers.
#  define KEYBOARD_CLEAR        0x93
#  define KEYBOARD_CLEAR_STRING "CLR"
#  if defined( __C16__) || defined( __C64__) || defined(__C128__) || defined(__VIC20__) || defined(__CBM510__) || defined(__CBM610__) || defined(__CX16__)
#    define KEYBOARD_F1        CH_F1
#    define KEYBOARD_F1_STRING "F1"
#    define KEYBOARD_F2        CH_F2
#    define KEYBOARD_F2_STRING "F2"
#  elif defined(__PET__) // __C16__ || __C64__ || __C128__ || __VIC20__ || __CBM510__ || __CBM610__ || __CX16__
     // Left arrow character key,
#    define KEYBOARD_F1        0x5F
#    define KEYBOARD_F1_STRING "\x5F (left arrow character)"
     // Up arrow character key.
#    define KEYBOARD_F2        0x5E
#    define KEYBOARD_F2_STRING "\x5E (up arrow character)"
#  else // __PET__
#    error build target not supported
#  endif

#elif defined(__ATARI__) // __CBM__
#  include <atari.h>
#  define KEYBOARD_UP           CH_CURS_UP
#  define KEYBOARD_DOWN         CH_CURS_DOWN
#  define KEYBOARD_LEFT         CH_CURS_LEFT
#  define KEYBOARD_RIGHT        CH_CURS_RIGHT
#  define KEYBOARD_BACKSPACE    CH_DEL
#  define KEYBOARD_INSERT       KEY_INSERT
#  define KEYBOARD_ENTER        CH_ENTER
#  define KEYBOARD_STOP         CH_ESC
#  define KEYBOARD_STOP_STRING  "ESC"
#  define KEYBOARD_HOME         CH_DELLINE
#  define KEYBOARD_HOME_STRING  "DELLINE (SHIFT+BACKSPACE)"
#  define KEYBOARD_CLEAR        CH_CLR
#  define KEYBOARD_CLEAR_STRING "CLEAR"
#  define KEYBOARD_F1           CH_F1
#  define KEYBOARD_F1_STRING    "F1 (ATARI+1)"
#  define KEYBOARD_F2           CH_F2
#  define KEYBOARD_F2_STRING    "F2 (ATARI+2)"

#else // __ATARI__
#  error build target not supported
#endif // #else



#endif // _KEYBOARD_H
