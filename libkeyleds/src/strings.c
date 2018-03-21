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
#include <string.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/features.h"


/** Lookup a string by id.
 * @param strings String group to scan, eg: `keyleds_feature_names`.
 * @param id Identifier to lookup.
 * @return A constant string in UTF8 encoding, or `NULL` on failure. It must not be freed.
 */
KEYLEDS_EXPORT const char * keyleds_lookup_string(const struct keyleds_indexed_string * strings,
                                                  unsigned id)
{
    unsigned idx;
    for (idx = 0; strings[idx].str != NULL; idx += 1) {
        if (strings[idx].id == id) { return strings[idx].str; }
    }
    return NULL;
}


/** Lookup a string id by name
 * @param strings String group to scan, eg: `keyleds_feature_names`.
 * @param str A string in UTF8 encoding.
 * @return The string identifier, or `KEYLEDS_STRING_INVALID` on failure.
 */
KEYLEDS_EXPORT unsigned keyleds_string_id(const struct keyleds_indexed_string * strings,
                                          const char * str)
{
    unsigned idx;
    for (idx = 0; strings[idx].str != NULL; idx += 1) {
        if (strcmp(strings[idx].str, str) == 0) { return strings[idx].id; }
    }
    return KEYLEDS_STRING_INVALID;
}

#define START_LIST(name)    const struct keyleds_indexed_string keyleds_##name[] = {
#define DEFSTR(id, str)         { (id), (str) },
#define END_LIST            { 0, NULL } };

KEYLEDS_EXPORT START_LIST(feature_names)
    DEFSTR(KEYLEDS_FEATURE_ROOT, "root")
    DEFSTR(KEYLEDS_FEATURE_FEATURE, "feature")
    DEFSTR(KEYLEDS_FEATURE_VERSION, "version")
    DEFSTR(KEYLEDS_FEATURE_NAME, "name")
    DEFSTR(KEYLEDS_FEATURE_DFU_CONTROL, "dfu-control")
    DEFSTR(KEYLEDS_FEATURE_BATTERY, "battery")
    DEFSTR(KEYLEDS_FEATURE_GAMEMODE, "gamemode")
    DEFSTR(KEYLEDS_FEATURE_KEYBOARD_LAYOUT_2, "layout2")
    DEFSTR(KEYLEDS_FEATURE_REPORTRATE, "reportrate")
    DEFSTR(KEYLEDS_FEATURE_LED_EFFECTS, "led-effects")
    DEFSTR(KEYLEDS_FEATURE_LEDS, "leds")
END_LIST

KEYLEDS_EXPORT START_LIST(protocol_types)
    DEFSTR(0, "application")
    DEFSTR(1, "bootloader")
    DEFSTR(2, "hardware")
END_LIST

KEYLEDS_EXPORT START_LIST(device_types)
    DEFSTR(KEYLEDS_DEVICE_TYPE_KEYBOARD, "keyboard")
    DEFSTR(KEYLEDS_DEVICE_TYPE_REMOTE, "remote")
    DEFSTR(KEYLEDS_DEVICE_TYPE_NUMPAD, "numpad")
    DEFSTR(KEYLEDS_DEVICE_TYPE_MOUSE, "mouse")
    DEFSTR(KEYLEDS_DEVICE_TYPE_TOUCHPAD, "touchpad")
    DEFSTR(KEYLEDS_DEVICE_TYPE_TRACKBALL, "trackball")
    DEFSTR(KEYLEDS_DEVICE_TYPE_PRESENTER, "presenter")
    DEFSTR(KEYLEDS_DEVICE_TYPE_RECEIVER, "receiver")
END_LIST

KEYLEDS_EXPORT START_LIST(block_id_names)
    DEFSTR(KEYLEDS_BLOCK_KEYS, "keys")
    DEFSTR(KEYLEDS_BLOCK_MULTIMEDIA, "media")
    DEFSTR(KEYLEDS_BLOCK_GKEYS, "gkeys")
    DEFSTR(KEYLEDS_BLOCK_LOGO, "logo")
    DEFSTR(KEYLEDS_BLOCK_MODES, "modes")
END_LIST

#ifdef INPUT_CODES_FOUND
#include <linux/input.h>

KEYLEDS_EXPORT START_LIST(keycode_names)
    DEFSTR(KEY_ESC, "ESC")
    DEFSTR(KEY_1, "1")
    DEFSTR(KEY_2, "2")
    DEFSTR(KEY_3, "3")
    DEFSTR(KEY_4, "4")
    DEFSTR(KEY_5, "5")
    DEFSTR(KEY_6, "6")
    DEFSTR(KEY_7, "7")
    DEFSTR(KEY_8, "8")
    DEFSTR(KEY_9, "9")
    DEFSTR(KEY_0, "0")
    DEFSTR(KEY_MINUS, "MINUS")
    DEFSTR(KEY_EQUAL, "EQUAL")
    DEFSTR(KEY_BACKSPACE, "BACKSPACE")
    DEFSTR(KEY_TAB, "TAB")
    DEFSTR(KEY_Q, "Q")
    DEFSTR(KEY_W, "W")
    DEFSTR(KEY_E, "E")
    DEFSTR(KEY_R, "R")
    DEFSTR(KEY_T, "T")
    DEFSTR(KEY_Y, "Y")
    DEFSTR(KEY_U, "U")
    DEFSTR(KEY_I, "I")
    DEFSTR(KEY_O, "O")
    DEFSTR(KEY_P, "P")
    DEFSTR(KEY_LEFTBRACE, "LBRACE")
    DEFSTR(KEY_RIGHTBRACE, "RBRACE")
    DEFSTR(KEY_ENTER, "ENTER")
    DEFSTR(KEY_LEFTCTRL, "LCTRL")
    DEFSTR(KEY_A, "A")
    DEFSTR(KEY_S, "S")
    DEFSTR(KEY_D, "D")
    DEFSTR(KEY_F, "F")
    DEFSTR(KEY_G, "G")
    DEFSTR(KEY_H, "H")
    DEFSTR(KEY_J, "J")
    DEFSTR(KEY_K, "K")
    DEFSTR(KEY_L, "L")
    DEFSTR(KEY_SEMICOLON, "SEMICOLON")
    DEFSTR(KEY_APOSTROPHE, "APOSTROPHE")
    DEFSTR(KEY_GRAVE, "GRAVE")
    DEFSTR(KEY_LEFTSHIFT, "LSHIFT")
    DEFSTR(KEY_BACKSLASH, "BACKSLASH")
    DEFSTR(KEY_Z, "Z")
    DEFSTR(KEY_X, "X")
    DEFSTR(KEY_C, "C")
    DEFSTR(KEY_V, "V")
    DEFSTR(KEY_B, "B")
    DEFSTR(KEY_N, "N")
    DEFSTR(KEY_M, "M")
    DEFSTR(KEY_COMMA, "COMMA")
    DEFSTR(KEY_DOT, "DOT")
    DEFSTR(KEY_SLASH, "SLASH")
    DEFSTR(KEY_RIGHTSHIFT, "RSHIFT")
    DEFSTR(KEY_KPASTERISK, "KPASTERISK")
    DEFSTR(KEY_LEFTALT, "LALT")
    DEFSTR(KEY_SPACE, "SPACE")
    DEFSTR(KEY_CAPSLOCK, "CAPSLOCK")
    DEFSTR(KEY_F1, "F1")
    DEFSTR(KEY_F2, "F2")
    DEFSTR(KEY_F3, "F3")
    DEFSTR(KEY_F4, "F4")
    DEFSTR(KEY_F5, "F5")
    DEFSTR(KEY_F6, "F6")
    DEFSTR(KEY_F7, "F7")
    DEFSTR(KEY_F8, "F8")
    DEFSTR(KEY_F9, "F9")
    DEFSTR(KEY_F10, "F10")
    DEFSTR(KEY_NUMLOCK, "NUMLOCK")
    DEFSTR(KEY_SCROLLLOCK, "SCROLLLOCK")
    DEFSTR(KEY_KP7, "KP7")
    DEFSTR(KEY_KP8, "KP8")
    DEFSTR(KEY_KP9, "KP9")
    DEFSTR(KEY_KPMINUS, "KPMINUS")
    DEFSTR(KEY_KP4, "KP4")
    DEFSTR(KEY_KP5, "KP5")
    DEFSTR(KEY_KP6, "KP6")
    DEFSTR(KEY_KPPLUS, "KPPLUS")
    DEFSTR(KEY_KP1, "KP1")
    DEFSTR(KEY_KP2, "KP2")
    DEFSTR(KEY_KP3, "KP3")
    DEFSTR(KEY_KP0, "KP0")
    DEFSTR(KEY_KPDOT, "KPDOT")
    DEFSTR(KEY_ZENKAKUHANKAKU, "ZENKAKUHANKAKU")
    DEFSTR(KEY_102ND, "102ND")
    DEFSTR(KEY_F11, "F11")
    DEFSTR(KEY_F12, "F12")
    DEFSTR(KEY_RO, "RO")
    DEFSTR(KEY_KATAKANA, "KATAKANA")
    DEFSTR(KEY_HIRAGANA, "HIRAGANA")
    DEFSTR(KEY_HENKAN, "HENKAN")
    DEFSTR(KEY_KATAKANAHIRAGANA, "KATAKANAHIRAGANA")
    DEFSTR(KEY_MUHENKAN, "MUHENKAN")
    DEFSTR(KEY_KPJPCOMMA, "KPJPCOMMA")
    DEFSTR(KEY_KPENTER, "KPENTER")
    DEFSTR(KEY_RIGHTCTRL, "RCTRL")
    DEFSTR(KEY_KPSLASH, "KPSLASH")
    DEFSTR(KEY_SYSRQ, "SYSRQ")
    DEFSTR(KEY_RIGHTALT, "RALT")
    DEFSTR(KEY_LINEFEED, "LINEFEED")
    DEFSTR(KEY_HOME, "HOME")
    DEFSTR(KEY_UP, "UP")
    DEFSTR(KEY_PAGEUP, "PAGEUP")
    DEFSTR(KEY_LEFT, "LEFT")
    DEFSTR(KEY_RIGHT, "RIGHT")
    DEFSTR(KEY_END, "END")
    DEFSTR(KEY_DOWN, "DOWN")
    DEFSTR(KEY_PAGEDOWN, "PAGEDOWN")
    DEFSTR(KEY_INSERT, "INSERT")
    DEFSTR(KEY_DELETE, "DELETE")
    DEFSTR(KEY_MACRO, "MACRO")
    DEFSTR(KEY_MUTE, "MUTE")
    DEFSTR(KEY_VOLUMEDOWN, "VOLUMEDOWN")
    DEFSTR(KEY_VOLUMEUP, "VOLUMEUP")
    DEFSTR(KEY_POWER, "POWER")
    DEFSTR(KEY_KPEQUAL, "KPEQUAL")
    DEFSTR(KEY_KPPLUSMINUS, "KPPLUSMINUS")
    DEFSTR(KEY_PAUSE, "PAUSE")
    DEFSTR(KEY_KPCOMMA, "KPCOMMA")
    DEFSTR(KEY_HANGEUL, "HANGEUL")
    DEFSTR(KEY_HANJA, "HANJA")
    DEFSTR(KEY_YEN, "YEN")
    DEFSTR(KEY_LEFTMETA, "LMETA")
    DEFSTR(KEY_RIGHTMETA, "RMETA")
    DEFSTR(KEY_COMPOSE, "COMPOSE")
    DEFSTR(KEY_STOP, "STOP")
    DEFSTR(KEY_AGAIN, "AGAIN")
    DEFSTR(KEY_PROPS, "PROPS")
    DEFSTR(KEY_UNDO, "UNDO")
    DEFSTR(KEY_FRONT, "FRONT")
    DEFSTR(KEY_COPY, "COPY")
    DEFSTR(KEY_OPEN, "OPEN")
    DEFSTR(KEY_PASTE, "PASTE")
    DEFSTR(KEY_FIND, "FIND")
    DEFSTR(KEY_CUT, "CUT")
    DEFSTR(KEY_HELP, "HELP")
    DEFSTR(KEY_CALC, "CALC")
    DEFSTR(KEY_SLEEP, "SLEEP")
    DEFSTR(KEY_WWW, "WWW")
    DEFSTR(KEY_SCREENLOCK, "SCREENLOCK")
    DEFSTR(KEY_BACK, "BACK")
    DEFSTR(KEY_FORWARD, "FORWARD")
    DEFSTR(KEY_EJECTCD, "EJECTCD")
    DEFSTR(KEY_NEXTSONG, "NEXTSONG")
    DEFSTR(KEY_PLAYPAUSE, "PLAYPAUSE")
    DEFSTR(KEY_PREVIOUSSONG, "PREVIOUSSONG")
    DEFSTR(KEY_STOPCD, "STOPCD")
    DEFSTR(KEY_REFRESH, "REFRESH")
    DEFSTR(KEY_EDIT, "EDIT")
    DEFSTR(KEY_SCROLLUP, "SCROLLUP")
    DEFSTR(KEY_SCROLLDOWN, "SCROLLDOWN")
    DEFSTR(KEY_F13, "F13")
    DEFSTR(KEY_F14, "F14")
    DEFSTR(KEY_F15, "F15")
    DEFSTR(KEY_F16, "F16")
    DEFSTR(KEY_F17, "F17")
    DEFSTR(KEY_F18, "F18")
    DEFSTR(KEY_F19, "F19")
    DEFSTR(KEY_F20, "F20")
    DEFSTR(KEY_F21, "F21")
    DEFSTR(KEY_F22, "F22")
    DEFSTR(KEY_F23, "F23")
    DEFSTR(KEY_F24, "F24")
END_LIST

#else
KEYLEDS_EXPORT START_LIST(keycode_names)
END_LIST
#endif
