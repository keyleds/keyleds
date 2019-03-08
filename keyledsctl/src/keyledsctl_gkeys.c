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
#include <string.h>


struct gkeys_options {
    const char *    device;
    bool            enable;
};

bool parse_gkeys_options(int argc, char * argv[],
                         /*@out@*/ struct gkeys_options * options)
{
    int opt;
    options->device = NULL;
    options->enable = true;

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

    if (optind == argc) {
        fprintf(stderr, "%s: either on or off is required.\n", argv[0]);
        return false;
    }

    if (strcmp(argv[optind], "on") == 0) {
        options->enable = true;
    } else if (strcmp(argv[optind], "off") == 0) {
        options->enable = false;
    } else {
        fprintf(stderr, "%s: either on or off is required.\n", argv[0]);
        return false;
    }
    return true;
}

int main_gkeys(int argc, char * argv[])
{
    struct gkeys_options options;
    Keyleds * device;

    if (!parse_gkeys_options(argc, argv, &options)) { return 1; }

    device = auto_select_device(options.device);
    if (device == NULL) { return 2; }

    if (!keyleds_gkeys_enable(device, KEYLEDS_TARGET_DEFAULT, options.enable)) {
        fprintf(stderr, "Setting G-keys mode info failed: %s\n", keyleds_get_error_str());
        return 3;
    }
    return EXIT_SUCCESS;
}
