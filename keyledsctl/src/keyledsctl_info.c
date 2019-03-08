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


struct info_options {
    const char *        device;
};

bool parse_info_options(int argc, char * argv[], /*@out@*/ struct info_options * options)
{
    int opt;
    options->device = NULL;

    reset_getopt(argc, argv, "-d:");
    while((opt = getopt(argc, argv, "-d:")) != -1) {
        switch(opt) {
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

int main_info(int argc, char * argv[])
{
    struct info_options options;
    Keyleds * device;
    unsigned idx;
    char * name;
    const char * str;
    keyleds_device_type_t type;
    struct keyleds_device_version * info;
    unsigned feature_count;
    const char ** feature_names;
    unsigned gkeys_number;
    unsigned * report_rates;
    struct keyleds_keyblocks_info * led_info;
    int result = EXIT_SUCCESS;

    if (!parse_info_options(argc, argv, &options)) { return 1; }

    device = auto_select_device(options.device);
    if (device == NULL) { return 2; }

    /* Device name */
    if (!keyleds_get_device_name(device, KEYLEDS_TARGET_DEFAULT, &name)) {
        (void)fprintf(stderr, "Get device name failed: %s\n", keyleds_get_error_str());
        result = 3;
        goto err_main_info_close;
    }
    (void)printf("Name:           %s\n", name);
    keyleds_free_device_name(name);

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

    /* GKeys features */
    if (keyleds_gkeys_count(device, KEYLEDS_TARGET_DEFAULT, &gkeys_number)) {
        (void)printf("G-keys: %u\n", gkeys_number);
    }

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
        keyleds_free_reportrates(report_rates);
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
