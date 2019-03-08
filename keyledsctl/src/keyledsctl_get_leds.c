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
#include "keyledsctl.h"

#include "dev_enum.h"
#include "keyleds.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


struct get_leds_options {
    const char *        device;
    keyleds_block_id_t  block_id;
};

bool parse_get_leds_options(int argc, char * argv[],
                            /*@out@*/ struct get_leds_options * options)
{
    int opt;
    options->device = NULL;
    options->block_id = KEYLEDS_BLOCK_KEYS;

    reset_getopt(argc, argv, "-b:d:");

    while((opt = getopt(argc, argv, "-b:d:")) != -1) {
        switch(opt) {
        case 'b':
            options->block_id = keyleds_string_id(keyleds_block_id_names, optarg);
            if (options->block_id == (keyleds_block_id_t)KEYLEDS_STRING_INVALID) {
                fprintf(stderr, "%s: invalid key block name -- '%s'\n", argv[0], optarg);
                return false;
            }
            break;
        case 'd':
            if (options->device != NULL) {
                fprintf(stderr, "%s: -d option can only be used once.\n", argv[0]);
                return false;
            }
            options->device = optarg;
            break;
        case 1:
            fprintf(stderr, "%s: unexpected argument -- '%s'\n", argv[0], optarg);
            /* fall through */
        default:
            return false;
        }
    }
    return true;
}

int main_get_leds(int argc, char * argv[])
{
    struct get_leds_options options;
    Keyleds * device;
    struct keyleds_keyblocks_info * led_info;
    unsigned idx, nb_keys = 0;

    if (!parse_get_leds_options(argc, argv, &options)) { return 1; }

    device = auto_select_device(options.device);
    if (device == NULL) { return 2; }

    if (!keyleds_get_block_info(device, KEYLEDS_TARGET_DEFAULT, &led_info)) {
        fprintf(stderr, "Fetching led info failed: %s\n", keyleds_get_error_str());
        return 3;
    }
    for (idx = 0; idx < led_info->length; idx += 1) {
        if (led_info->blocks[idx].block_id == options.block_id) {
            nb_keys = led_info->blocks[idx].nb_keys;
            break;
        }
    }
    if (idx >= led_info->length) {
        fprintf(stderr, "Led block %02x not found\n", options.block_id);
        return 4;
    }
    keyleds_free_block_info(led_info);

    {
    struct keyleds_key_color keys[nb_keys];
    if (!keyleds_get_leds(device, KEYLEDS_TARGET_DEFAULT, options.block_id,
                          keys, 0, nb_keys)) {
        fprintf(stderr, "Failed to read led status: %s\n", keyleds_get_error_str());
        return 5;
    }

    for (idx = 0; idx < nb_keys; idx += 1) {
        if (keys[idx].id == 0) { continue; }
        if (options.block_id == KEYLEDS_BLOCK_KEYS || options.block_id == KEYLEDS_BLOCK_MULTIMEDIA) {
            const unsigned keycode = keyleds_translate_scancode(options.block_id, keys[idx].id);
            if (keycode == 0) { continue; }
            const char * name = keyleds_lookup_string(keyleds_keycode_names, keycode);
            if (name == NULL) {
                printf("x%02x", keycode);
            } else {
                printf("%s", name);
            }
        } else {
            printf("x%02x", keys[idx].id);
        }
        printf("=#%02x%02x%02x\n", keys[idx].red, keys[idx].green, keys[idx].blue);
    }

    }
    keyleds_close(device);
    return EXIT_SUCCESS;
}
