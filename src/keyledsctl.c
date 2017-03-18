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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "config.h"
#include "dev_enum.h"
#include "keyleds.h"
#include "logging.h"
#include "utils.h"

/****************************************************************************/

struct main_modes {
    const char *    name;
    int             (*entry)(int argc, char * argv[]);
    const char *    usage;
};

int main_help(int argc, char * argv[]);
int main_list(int argc, char * argv[]);
int main_info(int argc, char * argv[]);
int main_get_leds(int argc, char * argv[]);
int main_set_leds(int argc, char * argv[]);
int main_gamemode(int argc, char * argv[]);

static const struct main_modes main_modes[] = {
    { "help", main_help,
      "Usage: %s %s [subcommand]\n" },
    { "list", main_list,
      "Usage: %s [-dqv] %s\n" },
    { "info", main_info,
      "Usage: %s [-dqv] %s [device]\n" },
    { "get-leds", main_get_leds,
      "Usage: %s [-dqv] %s [-d path] [key1 [key2 [...]]]\n" },
    { "set-leds", main_set_leds,
      "Usage: %s [-dqv] %s [-d path] [key1=color1 [key2=color2 [...]]]\n" },
    { "gamemode", main_gamemode,
      "Usage: %s [-dqv] %s [-d path] [key1 [key2 [...]]]\n" },
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


struct main_options {
    /*@observer@*/ const char * mode;
    int                         keyleds_verbosity;
    int                         verbosity;
};

void main_usage(FILE * stream, const char * name)
{
    unsigned idx;
    (void)fprintf(stream, "Usage: %s [-dqv] ", name);
    for (idx = 0; idx < sizeof(main_modes) / sizeof(main_modes[0]); idx += 1) {
        (void)fprintf(stream, idx == 0 ? "%s" : "|%s", main_modes[idx].name);
    }
    (void)fputc('\n', stream);
}

bool parse_main_options(int argc, char * argv[], /*@out@*/ struct main_options * const options)
{
    int opt;
    options->mode       = NULL;
    options->keyleds_verbosity = KEYLEDS_LOG_WARNING;
    options->verbosity  = LOG_WARNING;

    while ((opt = getopt(argc, argv, "+dqv")) != -1) {
        switch (opt) {
        case 'd': options->keyleds_verbosity += 1; break;
        case 'q':
            options->keyleds_verbosity = KEYLEDS_LOG_ERROR;
            options->verbosity = LOG_ERROR;
            break;
        case 'v': options->verbosity += 1; break;
        default:
            return false;
        }
    }
    if (optind >= argc) { return false; }
    options->mode = argv[optind++];
    return true;
}

int main(int argc, char * argv[])
{
    struct main_options options;
    unsigned idx;

    if (!parse_main_options(argc, argv, &options)) {
        main_usage(stderr, argv[0]);
        return 1;
    }

    g_debug_level = options.verbosity;
    g_keyleds_debug_level = options.keyleds_verbosity;

    for (idx = 0; idx < sizeof(main_modes) / sizeof(main_modes[0]); idx += 1) {
        if (strcmp(main_modes[idx].name, options.mode) == 0) {
            return (*main_modes[idx].entry)(argc, argv);
        }
    }

    (void)fprintf(stderr, "%s: unknown mode -- '%s'\n", argv[0], options.mode);
    main_usage(stderr, argv[0]);
    return 1;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

static void reset_getopt(int argc, char * const argv[], const char * optstring)
{
    int saved_optind = optind;
    optind = 0; opterr = 0;
    (void)getopt(argc, argv, optstring);
    optind = saved_optind; opterr = 1;
}

int main_help(int argc, char * argv[])
{
    const char * mode;
    unsigned idx;

    if (optind >= argc) {
        main_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
    }

    mode = argv[optind];

    for (idx = 1; idx < sizeof(main_modes) / sizeof(main_modes[0]); idx += 1) {
        if (strcmp(main_modes[idx].name, mode) == 0) {
            (void)printf(main_modes[idx].usage, argv[0], mode);
            return EXIT_SUCCESS;
        }
    }
    (void)fprintf(stderr, "Unknown mode '%s'\n", mode);
    main_usage(stderr, argv[0]);
    return 1;
}

/****************************************************************************/

int main_list(int argc, char * argv[])
{
    struct dev_enum_item * items;
    unsigned items_nb, idx;
    (void)argc, (void)argv;

    if (!enum_list_devices(&items, &items_nb)) {
        return 2;
    }

    for (idx = 0; idx < items_nb; idx += 1) {
        (void)printf("%s %04x:%04x", items[idx].path,
                     items[idx].vendor_id, items[idx].product_id);
        if (items[idx].serial) { (void)printf(" [%s]", items[idx].serial); }
        (void)putchar('\n');
    }

    enum_free_list(items);
    return EXIT_SUCCESS;
}

/****************************************************************************/

int main_info(int argc, char * argv[])
{
    Keyleds * device;
    unsigned idx;
    char * name;
    const char * str;
    keyleds_device_type_t type;
    struct keyleds_device_version * info;
    unsigned feature_count;
    const char ** feature_names;
    unsigned * report_rates;
    struct keyleds_keyblocks_info * led_info;
    int result = EXIT_SUCCESS;

    device = auto_select_device(optind < argc ? argv[optind++] : NULL);
    if (device == NULL) { return 2; }

    /* Device name */
    if (!keyleds_get_device_name(device, KEYLEDS_TARGET_DEFAULT, &name)) {
        (void)fprintf(stderr, "Get device name failed: %s\n", keyleds_get_error_str());
        result = 3;
        goto err_main_info_close;
    }
    (void)printf("Name:           %s\n", name);
    free(name);

    /* Device type */
    if (!keyleds_get_device_type(device, KEYLEDS_TARGET_DEFAULT, &type)) {
        (void)fprintf(stderr, "Get device type failed: %s\n", keyleds_get_error_str());
        result = 3;
        goto err_main_info_close;
    }
    str = keyleds_lookup_string(keyleds_device_types, type);
    (void)printf("Type:           %s\n", str != NULL ? str : "unknown");

    /* Device software version */
    if (!keyleds_get_device_version(device, KEYLEDS_TARGET_DEFAULT, &info)) {
        (void)fprintf(stderr, "Get device version failed: %s\n", keyleds_get_error_str());
        result = 3;
        goto err_main_info_close;
    }
    (void)printf("Model:          %02x%02x%02x%02x%02x%02x\n",
                 info->model[0], info->model[1], info->model[2],
                 info->model[3], info->model[4], info->model[5]);
    (void)printf("Serial:         %02x%02x%02x%02x\n",
                 info->serial[0], info->serial[1], info->serial[2], info->serial[3]);

    for (idx = 0; idx < info->length; idx += 1) {
        (void)printf("Firmware[%04x]: %s %s v%d.%d.%x",
               info->protocols[idx].product_id,
               keyleds_lookup_string(keyleds_protocol_types,
                                     (unsigned)info->protocols[idx].type),
               info->protocols[idx].prefix,
               info->protocols[idx].version_major,
               info->protocols[idx].version_minor,
               info->protocols[idx].build
        );
        if (info->protocols[idx].is_active) { (void)printf(" [active]"); }
        (void)putchar('\n');
    }
    keyleds_free_device_version(info);

    /* Device feature support */
    feature_count = keyleds_get_feature_count(device, KEYLEDS_TARGET_DEFAULT);
    feature_names = malloc(feature_count * sizeof(feature_names[0]));
    (void)printf("Features:       [");
    for (idx = 1; idx <= feature_count; idx += 1) {
        uint16_t fid = keyleds_get_feature_id(device, KEYLEDS_TARGET_DEFAULT, idx);
        feature_names[idx - 1] = keyleds_lookup_string(keyleds_feature_names, fid);
        (void)printf(idx == 1 ? "%04x" : ", %04x", fid);
    }
    (void)printf("]\nKnown features:");
    for (idx = 0; idx < feature_count; idx += 1) {
        if (feature_names[idx] != NULL) { (void)printf(" %s", feature_names[idx]); }
    }
    (void)putchar('\n');
    free(feature_names);

    /* Reportrate feature */
    if (keyleds_get_reportrates(device, KEYLEDS_TARGET_DEFAULT, &report_rates)) {
        unsigned current_rate = 0;
        keyleds_get_reportrate(device, KEYLEDS_TARGET_DEFAULT, &current_rate);
        (void)printf("Report rates:  ");
        for (idx = 0; report_rates[idx] > 0; idx += 1) {
            (void)printf(report_rates[idx] == current_rate ? " [%dms]" : " %dms",
                         report_rates[idx]);
        }
        (void)putchar('\n');
        free(report_rates);
    }

    /* Leds feature */
    if (keyleds_get_block_info(device, KEYLEDS_TARGET_DEFAULT, &led_info)) {
        for (idx = 0; idx < led_info->length; idx += 1) {
            (void)printf("LED block[%02x]:  %3d keys, max_rgb(%d, %d, %d)\n",
                         led_info->blocks[idx].block_id,
                         led_info->blocks[idx].nb_keys,
                         led_info->blocks[idx].red,
                         led_info->blocks[idx].green,
                         led_info->blocks[idx].blue);
        }
        keyleds_free_block_info(led_info);
    }

err_main_info_close:
    keyleds_close(device);
    return result;
}

/****************************************************************************/

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

    reset_getopt(argc, argv, "b:d:");

    while((opt = getopt(argc, argv, "b:d:")) != -1) {
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
    if (keyleds_get_leds(device, KEYLEDS_TARGET_DEFAULT, options.block_id,
                         keys, 0, nb_keys) < 0) {
        fprintf(stderr, "Failed to read led status: %s\n", keyleds_get_error_str());
        return 5;
    }

    for (idx = 0; idx < nb_keys; idx += 1) {
        if (keys[idx].id == 0) { continue; }
        if (options.block_id == KEYLEDS_BLOCK_KEYS) {
            const char * name = keyleds_lookup_string(keyleds_keycode_names,
                                                      keys[idx].keycode);
            if (name == NULL) {
                printf("x%02x", keys[idx].keycode);
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

/****************************************************************************/

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
                if (block_id == KEYLEDS_BLOCK_KEYS) {
                    code = keyleds_translate_keycode(code);
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

/****************************************************************************/

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
            fprintf(stderr, "test, %d, %d\n", optind, argc);
            if (!parse_keycode(argv[optind + idx], KEYLEDS_BLOCK_KEYS, &keycode)) {
                fprintf(stderr, "%s: invalid keycode %s\n", argv[0], argv[optind + idx]);
                goto err_gamemode_free_ids;
            }
            keyid = keyleds_translate_keycode(keycode);
            if (keyid == KEYLEDS_KEY_ID_INVALID) {
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
