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

/** Enable or disable custom GKeys behavior.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param cb Device's target identifier. See keyleds_open().
 * @param userdata `true` to enable, `false` to disable. When disabled, keys map to F keys.
 */
KEYLEDS_EXPORT void keyleds_gkeys_set_cb(Keyleds * device, uint8_t target_id,
                                         keyleds_gkeys_cb cb, void * userdata)
{
    keyleds_get_feature_index(device, target_id, KEYLEDS_FEATURE_GKEYS);
    keyleds_get_feature_index(device, target_id, KEYLEDS_FEATURE_MKEYS);
    keyleds_get_feature_index(device, target_id, KEYLEDS_FEATURE_MRKEYS);
    device->gkeys_cb = cb;
    device->userdata = userdata;
}

/** Set lit MKeys
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param mask A bit mask of MKeys leds to turn on, bit0 for M1, bit1 for M2, …
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_mkeys_set(Keyleds * device, uint8_t target_id, uint8_t mask)
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
KEYLEDS_EXPORT bool keyleds_mrkeys_set(Keyleds * device, uint8_t target_id, uint8_t mask)
{
    assert(device != NULL);

    if (keyleds_call(device, NULL, 0,
                     target_id, KEYLEDS_FEATURE_MRKEYS, F_SET_MRKEYS,
                     1, (uint8_t[]){mask}) < 0) {
        return false;
    }
    return true;
}

void keyleds_gkeys_filter(Keyleds * device, uint8_t message[], ssize_t buflen)
{
    if (buflen < 5) { return; }

    keyleds_gkeys_cb callback = device->gkeys_cb;
    if (!callback) { return; }

    uint8_t target_id = message[1];
    uint8_t feature_idx = message[2];
    keyleds_gkeys_type_t key_type;

    if (feature_idx == keyleds_get_feature_index(device, target_id, KEYLEDS_FEATURE_GKEYS)) {
        key_type = KEYLEDS_GKEYS_GKEY;
    } else if (feature_idx == keyleds_get_feature_index(device, target_id, KEYLEDS_FEATURE_MKEYS)) {
        key_type = KEYLEDS_GKEYS_MKEY;
    } else if (feature_idx == keyleds_get_feature_index(device, target_id, KEYLEDS_FEATURE_MRKEYS)) {
        key_type = KEYLEDS_GKEYS_MRKEY;
    } else {
        return;
    }

    callback(device, target_id, key_type,
             (uint16_t)(message[5] << 8 | message[4]), device->userdata);
}
