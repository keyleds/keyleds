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
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "dev_enum.h"
#include "keyleds.h"
#include "logging.h"

Keyleds * auto_select_device(const char * dev_path)
{
    Keyleds * device = NULL;
    struct dev_enum_item * items;
    unsigned items_nb;

    if (dev_path == NULL) {
        dev_path = getenv("KEYLEDS_DEVICE");
    }
    if (dev_path != NULL) {
        if (strchr(dev_path, '/') == NULL) {
            struct dev_enum_item * item;
            if (!enum_find_by_serial(dev_path, &item)) {
                (void)fprintf(stderr, "Could not locate device with serial %s\n", dev_path);
                return NULL;
            }
            device = keyleds_open(item->path, KEYLEDSCTL_APP_ID);
            enum_free_item(item);
        } else {
            device = keyleds_open(dev_path, KEYLEDSCTL_APP_ID);
        }
        if (device == NULL) {
            (void)fprintf(stderr, "Cannot open %s: %s\n", dev_path,
                          keyleds_get_error_str());
        }
        return device;
    }

    if (!enum_list_devices(&items, &items_nb)) {
        (void)fprintf(stderr, "Cannot list devices: %s\n", keyleds_get_error_str());
        return NULL;
    }

    if (items_nb == 1) {
        LOG(INFO, "Selecting device %s", items[0].path);
        device = keyleds_open(items[0].path, KEYLEDSCTL_APP_ID);
        if (device == NULL) {
            (void)fprintf(stderr, "Cannot open %s: %s\n", items[0].path,
                          keyleds_get_error_str());
        }
    } else {
        (void)fprintf(stderr, "More than one device found, use -d device or "
                              "set KEYLEDS_DEVICE environment variable.\n");
    }

    enum_free_list(items);
    return device;
}

void enum_free_item(struct dev_enum_item * item)
{
    free(item->path);
    free(item->serial);
    free(item->description);
    free(item);
}

void enum_free_list(struct dev_enum_item * list)
{
    for (unsigned idx = 0; list[idx].path != NULL; idx += 1) {
        free(list[idx].path);
        free(list[idx].serial);
        free(list[idx].description);
    }
    free(list);
}
