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
#include "keyleds/features.h"
#include "keyleds/logging.h"

enum gkeys_feature_function {    /* Function table for KEYLEDS_FEATURE_GKEYS */
    F_GET_GKEYS_COUNT = 0,
    F_ENABLE_GKEYS = 2,
};

enum mkeys_feature_function {    /* Function table for KEYLEDS_FEATURE_MKEYS */
    F_SET_MKEYS = 1,
};

enum mrkeys_feature_function {   /* Function table for KEYLEDS_FEATURE_MRKEYS */
    F_SET_MRKEYS = 0,
};

/** Get the number of GKeys.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param [out] nb Number of GKeys on keyboard.
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_gkeys_count(Keyleds * device, uint8_t target_id, unsigned * nb)
{
    uint8_t data[1];

    assert(device != NULL);
    assert(nb != NULL);

    if (keyleds_call(device, data, sizeof(data),
                     target_id, KEYLEDS_FEATURE_GKEYS, F_GET_GKEYS_COUNT,
                     0, NULL) < 0) {
        return false;
    }
    *nb = (unsigned)data[0];
    return true;
}


/** Enable or disable custom GKeys behavior.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param enabled `true` to enable, `false` to disable. When disabled, keys map to F keys.
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_gkeys_enable(Keyleds * device, uint8_t target_id, bool enabled)
{
    assert(device != NULL);

    if (keyleds_call(device, NULL, 0,
                     target_id, KEYLEDS_FEATURE_GKEYS, F_ENABLE_GKEYS,
                     1, (uint8_t[]){(uint8_t)(enabled ? 1u : 0u)}) < 0) {
        return false;
    }
    return true;
}

/** Set lit MKeys
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param mask A bit mask of MKeys leds to turn on, bit0 for M1, bit1 for M2, …
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_mkeys_set(Keyleds * device, uint8_t target_id, unsigned char mask)
{
    assert(device != NULL);

    if (keyleds_call(device, NULL, 0,
                     target_id, KEYLEDS_FEATURE_MKEYS, F_SET_MKEYS,
                     1, (uint8_t[]){mask}) < 0) {
        return false;
    }
    return true;
}

/** Set lit MRKeys
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param mask A bit mask of MRKeys leds to turn on, bit0 for MR.
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_mrkeys_set(Keyleds * device, uint8_t target_id, unsigned char mask)
{
    assert(device != NULL);

    if (keyleds_call(device, NULL, 0,
                     target_id, KEYLEDS_FEATURE_MRKEYS, F_SET_MRKEYS,
                     1, (uint8_t[]){mask}) < 0) {
        return false;
    }
    return true;
}
