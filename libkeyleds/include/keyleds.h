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

#ifdef __cplusplus
extern "C" {
#endif

#define LOGITECH_VENDOR_ID  ((uint16_t)0x046d)
#define KEYLEDS_TARGET_DEFAULT ((uint8_t)0xff)

typedef struct keyleds_device Keyleds;

/****************************************************************************/
/* Device setup */

#define KEYLEDS_APP_ID_MIN  ((uint8_t)0x0)
#define KEYLEDS_APP_ID_MAX  ((uint8_t)0xf)

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
bool keyleds_ping(Keyleds * device, uint8_t target_id); /* re-sync with device after error */
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
                             /*@out@*/ char ** out);    /* caller must free() on success */
bool keyleds_get_device_type(Keyleds * device, uint8_t target_id,
                             /*@out@*/ keyleds_device_type_t * out);
bool keyleds_get_device_version(Keyleds * device, uint8_t target_id,
                                /*@out@*/ struct keyleds_device_version ** out);
void keyleds_free_device_version(/*@only@*/ /*@out@*/ struct keyleds_device_version *);

/****************************************************************************/
/* Gamemode feature */

bool keyleds_gamemode_max(Keyleds * device, uint8_t target_id, /*@out@*/ unsigned * nb);
bool keyleds_gamemode_set(Keyleds * device, uint8_t target_id,
                          const uint8_t * ids, unsigned ids_nb);    /* add some keys */
bool keyleds_gamemode_clear(Keyleds * device, uint8_t target_id,
                            const uint8_t * ids, unsigned ids_nb);  /* remove some keys */
bool keyleds_gamemode_reset(Keyleds * device, uint8_t target_id);   /* remove all keys */

/****************************************************************************/
/* Keyboard layout feature */

typedef enum {
    KEYLEDS_KEYBOARD_LAYOUT_FRA = 5,
    KEYLEDS_KEYBOARD_LAYOUT_INVALID = -1
} keyleds_keyboard_layout_t;

keyleds_keyboard_layout_t keyleds_keyboard_layout(Keyleds * device, uint8_t target_id);

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
    uint8_t     id;             /* as reported by keyboard */
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
                      const struct keyleds_key_color * keys, unsigned keys_nb);
bool keyleds_set_led_block(Keyleds * device, uint8_t target_id, keyleds_block_id_t block_id,
                           uint8_t red, uint8_t green, uint8_t blue);
bool keyleds_commit_leds(Keyleds * device, uint8_t target_id);

/****************************************************************************/
/* Error and logging */

typedef enum {
    KEYLEDS_NO_ERROR    = 0,
    KEYLEDS_ERROR_ERRNO,         /* system error, look it up in errno */
    KEYLEDS_ERROR_DEVICE,        /* error return by device */
    KEYLEDS_ERROR_IO_LENGTH,
    KEYLEDS_ERROR_HIDREPORT,
    KEYLEDS_ERROR_HIDVERSION,
    KEYLEDS_ERROR_FEATURE_NOT_FOUND,
    KEYLEDS_ERROR_TIMEDOUT,
    KEYLEDS_ERROR_RESPONSE
} keyleds_error_t;

/*@observer@*/ const char * keyleds_get_error_str();
keyleds_error_t             keyleds_get_errno();

extern /*@null@*/ FILE * g_keyleds_debug_stream;
extern int g_keyleds_debug_level;
extern int g_keyleds_debug_hid;     /* set to KEYLEDS_LOG_DEBUG to see parser output */
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

/* keycode is code event (eg KEY_F1)
 * scancode is physical key id as defined by USB standards
 * The keycode/scancode translation only applies to keys from block 0x01 (keys block) */
unsigned keyleds_translate_scancode(uint8_t scancode);
uint8_t keyleds_translate_keycode(unsigned keycode);

#ifdef __cplusplus
}
#endif

#endif
