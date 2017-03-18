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
#ifndef DEV_ENUM_H
#define DEV_ENUM_H

#include "keyleds.h"

struct dev_enum_item {
    char *      path;
    uint16_t    vendor_id;
    uint16_t    product_id;
    char *      serial;
    char *      description;
};

Keyleds * auto_select_device(const char * dev_path);

bool enum_find_by_serial(const char * serial, /*@out@*/ struct dev_enum_item ** out);
bool enum_list_devices(/*@out@*/ struct dev_enum_item ** out, /*@out@*/ unsigned * out_nb);

void enum_free_item(/*@only@*/ /*@out@*/ struct dev_enum_item * item);
void enum_free_list(/*@only@*/ /*@out@*/ struct dev_enum_item * list);

#endif
