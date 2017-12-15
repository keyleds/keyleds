/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include "config.h"
#include "keyleds.h"

static const uint8_t scancode_to_keycode[256] = {
      0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,    /* 0x00 */
     50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,    /* 0x10 */
      4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,    /* 0x20 */
     27,  0, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,    /* 0x30 */
     65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,    /* 0x40 */
    105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,    /* 0x50 */
     72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,    /* 0x60 */
    191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,    /* 0x70 */
    115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,    /* 0x80 */
    122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    /* 0x90 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    /* 0xa0 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    /* 0xb0 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    /* 0xc0 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    /* 0xd0 */
     29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,    /* 0xe0 */
    150,158,159,128,136,177,178,176,142,152,173,140,  0,  0,  0,  0     /* 0xf0 */
};
/* Duplicate scancodes for 43, 113, 114, 115, 128, 136 might not work
 *  - 0x31 and 0x32 both result in 43 (backslash).
 *      => Logitech seems to use 0x32, so 0x31 is killed in the table
 */

static const uint8_t keycode_to_scancode[] = {
    0x00, 0x29, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, /* 0x00 */
    0x24, 0x25, 0x26, 0x27, 0x2d, 0x2e, 0x2a, 0x2b, /* 0x08 */
    0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c, /* 0x10 */
    0x12, 0x13, 0x2f, 0x30, 0x28, 0xe0, 0x04, 0x16, /* 0x18 */
    0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f, 0x33, /* 0x20 */
    0x34, 0x35, 0xe1, 0x32, 0x1d, 0x1b, 0x06, 0x19, /* 0x28 */
    0x05, 0x11, 0x10, 0x36, 0x37, 0x38, 0xe5, 0x55, /* 0x30 */
    0xe2, 0x2c, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, /* 0x38 */
    0x3f, 0x40, 0x41, 0x42, 0x43, 0x53, 0x47, 0x5f, /* 0x40 */
    0x60, 0x61, 0x56, 0x5c, 0x5d, 0x5e, 0x57, 0x59, /* 0x48 */
    0x5a, 0x5b, 0x62, 0x63, 0x00, 0x94, 0x64, 0x44, /* 0x50 */
    0x45, 0x87, 0x92, 0x93, 0x8a, 0x88, 0x8b, 0x8c, /* 0x58 */
    0x58, 0xe4, 0x54, 0x46, 0xe6, 0x00, 0x4a, 0x52, /* 0x60 */
    0x4b, 0x50, 0x4f, 0x4d, 0x51, 0x4e, 0x49, 0x4c, /* 0x68 */
    0x00, 0xef, 0xee, 0xed, 0x66, 0x67, 0x00, 0x48, /* 0x70 */
    0x00, 0x85, 0x90, 0x91, 0x89, 0xe3, 0xe7, 0x65, /* 0x78 */
    0xf3, 0x79, 0x76, 0x7a, 0x77, 0x7c, 0x74, 0x7d, /* 0x80 */
    0xf4, 0x7b, 0x75, 0x00, 0xfb, 0x00, 0xf8, 0x00, /* 0x88 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, /* 0x90 */
    0xf9, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf1, 0xf2, /* 0x98 */
    0x00, 0xec, 0x00, 0xeb, 0xe8, 0xea, 0xe9, 0x00, /* 0xa0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, /* 0xa8 */
    0xf7, 0xf5, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x68, /* 0xb0 */
    0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, /* 0xb8 */
    0x71, 0x72, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00  /* 0xc0 */
};

KEYLEDS_EXPORT unsigned keyleds_translate_scancode(keyleds_block_id_t block, uint8_t scancode)
{
    if (block == KEYLEDS_BLOCK_KEYS) {
        return (unsigned)scancode_to_keycode[scancode];
    }
    if (block == KEYLEDS_BLOCK_MULTIMEDIA) {
        switch (scancode) {
            case 0xb5: return 163;
            case 0xb6: return 165;
            case 0xb7: return 166;
            case 0xcd: return 164;
            case 0xe2: return 113;
            case 0xe9: return 114;
            case 0xea: return 115;
            default: return 0;
        }
    }
    return 0;
}

KEYLEDS_EXPORT bool keyleds_translate_keycode(unsigned keycode, keyleds_block_id_t * blockptr,
                                              uint8_t * scancodeptr)
{
    keyleds_block_id_t block;
    uint8_t scancode = 0;

    switch (keycode) {
        case 113: scancode = 0xe2, block = KEYLEDS_BLOCK_MULTIMEDIA; break;
        case 114: scancode = 0xe9, block = KEYLEDS_BLOCK_MULTIMEDIA; break;
        case 115: scancode = 0xea, block = KEYLEDS_BLOCK_MULTIMEDIA; break;
        case 163: scancode = 0xb5, block = KEYLEDS_BLOCK_MULTIMEDIA; break;
        case 164: scancode = 0xcd, block = KEYLEDS_BLOCK_MULTIMEDIA; break;
        case 165: scancode = 0xb6, block = KEYLEDS_BLOCK_MULTIMEDIA; break;
        case 166: scancode = 0xb7, block = KEYLEDS_BLOCK_MULTIMEDIA; break;
        default:
            if (keycode < sizeof(keycode_to_scancode) / sizeof(keycode_to_scancode[0])) {
                scancode = keycode_to_scancode[keycode], block = KEYLEDS_BLOCK_KEYS;
            }
    }
    if (scancode == 0) { return false; }

    if (blockptr) { *blockptr = block; }
    if (scancodeptr) { *scancodeptr = scancode; }
    return true;
}
