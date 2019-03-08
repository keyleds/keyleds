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
#include "utils.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


struct set_leds_directive {
    keyleds_block_id_t  block_id;
    uint8_t             id;
    struct color        color;
};

struct set_leds_options {
    const char *                device;
    struct set_leds_directive * directives;
    unsigned                    directives_nb;
};

bool parse_set_leds_options(int argc, char * argv[],
                            /*@out@*/ struct set_leds_options * const options)
{
    int opt;
    keyleds_block_id_t block_id = KEYLEDS_BLOCK_KEYS;
    options->device = NULL;
    options->directives = NULL;
    options->directives_nb = 0;

    reset_getopt(argc, argv, "-b:d:");

    while ((opt = getopt(argc, argv, "-b:d:")) != -1) {
        switch (opt) {
        case 1: {/* non-option */
            char * keyname = optarg;
            char * equal = strchr(keyname, '=');
            unsigned code;
            struct color color;

            if (equal == NULL) {
                fprintf(stderr, "%s: no '=' in directive -- '%s'\n", argv[0], optarg);
                goto err_free_options;
            }

            if (strncasecmp("all=", keyname, 4) == 0) {
                code = KEYLEDS_KEY_ID_INVALID;

            } else {
                char string[equal - keyname + 1];
                memcpy(string, keyname, equal - keyname);
                string[equal - keyname] = '\0';

                if (!parse_keycode(string, block_id, &code)) {
                    fprintf(stderr, "%s: invalid key in directive -- '%s'\n", argv[0], optarg);
                    goto err_free_options;
                }
                if (block_id == KEYLEDS_BLOCK_KEYS || block_id == KEYLEDS_BLOCK_MULTIMEDIA) {
                    uint8_t value;
                    keyleds_translate_keycode(code, NULL, &value);
                    code = value;
                }
            }

            if (!parse_color(equal + 1, &color)) {
                fprintf(stderr, "%s: invalid color in directive -- '%s'\n", argv[0], optarg);
                goto err_free_options;
            }

            options->directives = realloc(options->directives,
                                          (options->directives_nb + 1)
                                          * sizeof(options->directives[0]));
            options->directives[options->directives_nb].block_id = block_id;
            options->directives[options->directives_nb].id = code;
            memcpy(&options->directives[options->directives_nb].color, &color, sizeof(color));
            options->directives_nb += 1;
            break;
        }
        case 'b':
            block_id = keyleds_string_id(keyleds_block_id_names, optarg);
            if (block_id == (keyleds_block_id_t)KEYLEDS_STRING_INVALID) {
                fprintf(stderr, "%s: invalid key block name -- '%s'\n", argv[0], optarg);
                goto err_free_options;
            }
            break;
        case 'd':
            if (options->device != NULL) {
                fprintf(stderr, "%s: -d option can only be used once.\n", argv[0]);
                goto err_free_options;
            }
            if (options->directives_nb > 0) {
                fprintf(stderr, "%s: -d option must come before directives.\n", argv[0]);
                goto err_free_options;
            }
            options->device = optarg;
            break;
        default:
            goto err_free_options;
        }
    }
    return true;

err_free_options:
    free(options->directives);
    return false;
}

int main_set_leds(int argc, char * argv[])
{
    struct set_leds_options options;
    Keyleds * device;
    if (!parse_set_leds_options(argc, argv, &options)) { return 1; }

    device = auto_select_device(options.device);
    if (device == NULL) { return 2; }

    {
    struct keyleds_key_color keys[options.directives_nb];
    unsigned keys_nb = 0, idx;
    keyleds_block_id_t block_id = KEYLEDS_BLOCK_INVALID;

    for (idx = 0; idx < options.directives_nb; idx += 1) {
        if (options.directives[idx].id != KEYLEDS_KEY_ID_INVALID &&
            options.directives[idx].block_id == block_id) {
            keys[keys_nb].id = options.directives[idx].id;
            keys[keys_nb].red = options.directives[idx].color.red;
            keys[keys_nb].green = options.directives[idx].color.green;
            keys[keys_nb].blue = options.directives[idx].color.blue;
            keys_nb += 1;
            continue;
        }
        if (keys_nb > 0) {
            if (!keyleds_set_leds(device, KEYLEDS_TARGET_DEFAULT,
                                block_id, keys, keys_nb)) {
                fprintf(stderr, "%s: set leds -- %s\n", argv[0], keyleds_get_error_str());
            }
            keys_nb = 0;
        }
        if (options.directives[idx].id == KEYLEDS_KEY_ID_INVALID) {
            if (!keyleds_set_led_block(
                device, KEYLEDS_TARGET_DEFAULT,
                options.directives[idx].block_id,
                options.directives[idx].color.red,
                options.directives[idx].color.green,
                options.directives[idx].color.blue
            )) {
                fprintf(stderr, "%s: set led block %02x -- %s\n", argv[0],
                        options.directives[idx].block_id, keyleds_get_error_str());
            }
        } else {
            keys[keys_nb].id = options.directives[idx].id;
            keys[keys_nb].red = options.directives[idx].color.red;
            keys[keys_nb].green = options.directives[idx].color.green;
            keys[keys_nb].blue = options.directives[idx].color.blue;
            keys_nb += 1;
            block_id = options.directives[idx].block_id;
        }
    }

    if (keys_nb > 0) {
        if (!keyleds_set_leds(device, KEYLEDS_TARGET_DEFAULT,
                              block_id, keys, keys_nb)) {
            fprintf(stderr, "%s: set leds -- %s\n", argv[0], keyleds_get_error_str());
        }
    }

    }
    keyleds_commit_leds(device, KEYLEDS_TARGET_DEFAULT);

    keyleds_close(device);
    free(options.directives);
    return EXIT_SUCCESS;
}
