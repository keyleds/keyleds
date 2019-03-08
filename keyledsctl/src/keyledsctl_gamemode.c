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


struct gamemode_options {
    const char *        device;
    uint8_t *           key_ids;
    unsigned            key_ids_nb;
};

bool parse_gamemode_options(int argc, char * argv[],
                            /*@out@*/ struct gamemode_options * options)
{
    int opt;
    options->device = NULL;
    options->key_ids = NULL;
    options->key_ids_nb = 0;

    reset_getopt(argc, argv, "+d:");

    while((opt = getopt(argc, argv, "+d:")) != -1) {
        switch(opt) {
        case 'd':
            if (options->device != NULL) {
                fprintf(stderr, "%s: -d option can only be used once.\n", argv[0]);
                return false;
            }
            options->device = optarg;
            break;
        default:
            return false;
        }
    }

    if (optind < argc) {
        int idx;
        options->key_ids = malloc((argc - optind) * sizeof(options->key_ids[0]));

        for (idx = 0; idx < argc - optind; idx += 1) {
            unsigned keycode;
            uint8_t keyid;
            if (!parse_keycode(argv[optind + idx], KEYLEDS_BLOCK_KEYS, &keycode)) {
                fprintf(stderr, "%s: invalid keycode %s\n", argv[0], argv[optind + idx]);
                goto err_gamemode_free_ids;
            }
            if (!keyleds_translate_keycode(keycode, NULL, &keyid)) {
                fprintf(stderr, "%s: invalid keycode %s\n", argv[0], argv[optind + idx]);
                goto err_gamemode_free_ids;
            }
            options->key_ids[idx] = keyid;
        }
        options->key_ids_nb = argc - optind;
        optind = argc;
    }
    return true;

err_gamemode_free_ids:
    free(options->key_ids);
    return false;
}

int main_gamemode(int argc, char * argv[])
{
    struct gamemode_options options;
    Keyleds * device;
    int result = EXIT_SUCCESS;

    if (!parse_gamemode_options(argc, argv, &options)) { return 1; }

    device = auto_select_device(options.device);
    if (device == NULL) { return 2; }

    if (!keyleds_gamemode_reset(device, KEYLEDS_TARGET_DEFAULT) ||
        (options.key_ids_nb > 0 &&
         !keyleds_gamemode_set(device, KEYLEDS_TARGET_DEFAULT,
                               options.key_ids, options.key_ids_nb))) {
        fprintf(stderr, "Clear all gamemode keys failed: %s\n", keyleds_get_error_str());
        result = 2;
    }

    keyleds_close(device);
    free(options.key_ids);
    return result;
}
