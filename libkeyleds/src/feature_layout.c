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
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/device.h"
#include "keyleds/error.h"
#include "keyleds/features.h"
#include "keyleds/logging.h"

enum layout_feature_function {
    F_GET_LAYOUT = 0
};

KEYLEDS_EXPORT keyleds_keyboard_layout_t keyleds_keyboard_layout(Keyleds * device, uint8_t target_id)
{
    uint8_t data[1];

    assert(device != NULL);

    if (keyleds_call(device, data, sizeof(data),
                     target_id, KEYLEDS_FEATURE_KEYBOARD_LAYOUT_2, F_GET_LAYOUT, 0, NULL) < 0) {
        return KEYLEDS_KEYBOARD_LAYOUT_INVALID;
    }
    return (keyleds_keyboard_layout_t)data[0];
}
