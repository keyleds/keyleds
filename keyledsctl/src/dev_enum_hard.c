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
#include <sys/types.h>
#include <dirent.h>
#include <linux/hidraw.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "config.h"
#include "dev_enum.h"
#include "keyleds.h"
#include "logging.h"

static const char dev_root[] = "/dev";


bool enum_find_by_serial(const char * serial, struct dev_enum_item ** out)
{
    (void)serial, (void)out;
    LOG(ERROR, "Requires libudev support compiled in.");
    return false;
}

bool enum_list_devices(struct dev_enum_item ** out, unsigned * out_nb)
{
    struct dev_enum_item * items = NULL;
    unsigned items_nb = 0;

    DIR * dir;
    struct dirent * entry;
    char path[sizeof(dev_root) + 1 + sizeof(entry->d_name) + 1];

    strcpy(path, dev_root);
    strcat(path, "/");

    if ((dir = opendir(dev_root)) == NULL) { return false; }

    while ((entry = readdir(dir)) != NULL) {
        Keyleds * device;
        struct hidraw_devinfo devinfo;

        if (strncmp(entry->d_name, "hidraw", 6) != 0) { continue; }

        strcpy(path + sizeof(dev_root) + 1 - 1, entry->d_name);

        if ((device = keyleds_open(path, KEYLEDSCTL_APP_ID)) == NULL) { continue; }

        if (ioctl(keyleds_device_fd(device), HIDIOCGRAWINFO, &devinfo) >= 0) {
            items = realloc(items, (items_nb + 1) * sizeof(struct dev_enum_item));
            items[items_nb].path = malloc(strlen(path) + 1);
            strcpy(items[items_nb].path, path);
            items[items_nb].vendor_id = devinfo.vendor;
            items[items_nb].product_id = devinfo.product;
            items[items_nb].serial = NULL;
            items[items_nb].description = NULL;
            items_nb += 1;
        }

        /* FIXME 'items'
         * dev_enum_hard.c:64:13: error: Common realloc mistake: 'items' nulled but not freed upon failure [memleakOnRealloc]
         *  items = realloc(items, (items_nb + 1) * sizeof(struct dev_enum_item));
         */

        keyleds_close(device);
    }
    closedir(dir);

    *out = items;
    *out_nb = items_nb;
    return true;
}
