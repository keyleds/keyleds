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
#ifndef KEYLEDS_H
#define KEYLEDS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define LOGITECH_VENDOR_ID  ((uint16_t)0x046d)

struct keyleds_device;
typedef struct keyleds_device Keyleds;
#define KEYLEDS_TARGET_DEFAULT (0xff)

/****************************************************************************/
/* Device setup */

Keyleds * keyleds_open(const char * path, uint8_t app_id);
void keyleds_close(Keyleds * device);
int keyleds_device_fd(Keyleds * device);

/****************************************************************************/
/* Basic device communication */

typedef enum {
    KEYLEDS_DEVICE_HANDLER_DEVICE = (1<<0),
    KEYLEDS_DEVICE_HANDLER_GAMING = (1<<1),
    KEYLEDS_DEVICE_HANDLER_PREFERENCE = (1<<2),
    KEYLEDS_DEVICE_HANDLER_FEATURE = (1<<7)
} keyleds_device_handler_t;

bool keyleds_get_protocol(Keyleds * device, uint8_t target_id,
                          unsigned * version, keyleds_device_handler_t * handler);
bool keyleds_ping(Keyleds * device, uint8_t target_id);
unsigned keyleds_get_feature_count(Keyleds * dev, uint8_t target_id);
uint16_t keyleds_get_feature_id(Keyleds * dev, uint8_t target_id, uint8_t feature_idx);
uint8_t keyleds_get_feature_index(Keyleds * dev, uint8_t target_id, uint16_t feature_id);

/****************************************************************************/
/* Device information */

struct keyleds_device_version {
    uint8_t     serial[4];
    uint16_t    transport;
    uint8_t     model[6];

    unsigned    length;
    struct {
                uint8_t     type;
                char        prefix[4];
                unsigned    version_major;
                unsigned    version_minor;
                unsigned    build;
                bool        is_active;
                uint16_t    product_id;
                uint8_t     misc[5];
    }           protocols[];
};

typedef enum {
    KEYLEDS_DEVICE_TYPE_KEYBOARD = 0,
    KEYLEDS_DEVICE_TYPE_REMOTE = 1,
    KEYLEDS_DEVICE_TYPE_NUMPAD = 2,
    KEYLEDS_DEVICE_TYPE_MOUSE = 3,
    KEYLEDS_DEVICE_TYPE_TOUCHPAD = 4,
    KEYLEDS_DEVICE_TYPE_TRACKBALL = 5,
    KEYLEDS_DEVICE_TYPE_PRESENTER = 6,
    KEYLEDS_DEVICE_TYPE_RECEIVER = 7
} keyleds_device_type_t;

bool keyleds_get_device_name(Keyleds * device, uint8_t target_id,
                             /*@out@*/ char ** out);
bool keyleds_get_device_type(Keyleds * device, uint8_t target_id,
                             /*@out@*/ keyleds_device_type_t * out);
bool keyleds_get_device_version(Keyleds * device, uint8_t target_id,
                                /*@out@*/ struct keyleds_device_version ** out);

/****************************************************************************/
/* Reportrate feature */

bool keyleds_get_reportrates(Keyleds * device, uint8_t target_id, /*@out@*/ unsigned ** out);
bool keyleds_get_reportrate(Keyleds * device, uint8_t target_id, /*@out@*/ unsigned * rate);
bool keyleds_set_reportrate(Keyleds * device, uint8_t target_id, unsigned rate);

/****************************************************************************/
/* Leds features */

typedef enum {
    KEYLEDS_BLOCK_KEYS = (1<<0),
    KEYLEDS_BLOCK_MULTIMEDIA = (1<<1),
    KEYLEDS_BLOCK_GKEYS = (1<<2),
    KEYLEDS_BLOCK_LOGO = (1<<4),
    KEYLEDS_BLOCK_MODES = (1<<6),
    KEYLEDS_BLOCK_INVALID = -1
} keyleds_block_id_t;

struct keyleds_keyblocks_info {
    unsigned    length;
    struct {
        keyleds_block_id_t block_id;
        uint16_t    nb_keys;    /* number of keys in block */
        uint8_t     red;        /* maximum allowable value for red channel */
        uint8_t     green;      /* maximum allowable value for green channel */
        uint8_t     blue;       /* maximum allowable value for blue channel */
    }           blocks[];
};

struct keyleds_key_color {
    unsigned    keycode;        /* as reported when pressing keys */
    uint8_t     id;
    uint8_t     red;
    uint8_t     green;
    uint8_t     blue;
};
#define KEYLEDS_KEY_ID_INVALID  (0)

bool keyleds_get_block_info(Keyleds * device, uint8_t target_id,
                            /*@out@*/ struct keyleds_keyblocks_info ** out);
void keyleds_free_block_info(/*@only@*/ /*@out@*/ struct keyleds_keyblocks_info * info);
bool keyleds_get_leds(Keyleds * device, uint8_t target_id, keyleds_block_id_t block_id,
                      struct keyleds_key_color * keys, uint16_t offset, unsigned keys_nb);
bool keyleds_set_leds(Keyleds * device, uint8_t target_id, keyleds_block_id_t block_id,
                      struct keyleds_key_color * keys, unsigned keys_nb);
bool keyleds_set_led_block(Keyleds * device, uint8_t target_id, keyleds_block_id_t block_id,
                           uint8_t red, uint8_t green, uint8_t blue);
bool keyleds_commit_leds(Keyleds * device, uint8_t target_id);

/****************************************************************************/
/* Leds features */

bool keyleds_gamemode_max(Keyleds * device, uint8_t target_id, /*@out@*/ unsigned * nb);
bool keyleds_gamemode_set(Keyleds * device, uint8_t target_id,
                          const uint8_t * ids, unsigned ids_nb);
bool keyleds_gamemode_clear(Keyleds * device, uint8_t target_id,
                            const uint8_t * ids, unsigned ids_nb);
bool keyleds_gamemode_reset(Keyleds * device, uint8_t target_id);

/****************************************************************************/
/* Error and logging */

/*@observer@*/ const char * keyleds_get_error_str();

extern /*@null@*/ FILE * g_keyleds_debug_stream;
extern int g_keyleds_debug_level;
extern int g_keyleds_debug_hid;
#define KEYLEDS_LOG_ERROR       (1)
#define KEYLEDS_LOG_WARNING     (2)
#define KEYLEDS_LOG_INFO        (3)
#define KEYLEDS_LOG_DEBUG       (4)

/****************************************************************************/
/* String definitions and utility functions */

struct keyleds_indexed_string {unsigned id; /*@null@*/ /*@observer@*/ const char * str;};
#define KEYLEDS_STRING_INVALID  ((unsigned)-1)

extern const struct keyleds_indexed_string keyleds_feature_names[];
extern const struct keyleds_indexed_string keyleds_protocol_types[];
extern const struct keyleds_indexed_string keyleds_device_types[];
extern const struct keyleds_indexed_string keyleds_block_id_names[];
extern const struct keyleds_indexed_string keyleds_keycode_names[];

/*@observer@*/ const char * keyleds_lookup_string(const struct keyleds_indexed_string *,
                                                  unsigned id);
unsigned keyleds_string_id(const struct keyleds_indexed_string *,
                           const char * str);

unsigned keyleds_translate_scancode(uint8_t scancode);
uint8_t keyleds_translate_keycode(unsigned keycode);

#endif
