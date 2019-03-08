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

#include <stdio.h>
#include <stdlib.h>


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
