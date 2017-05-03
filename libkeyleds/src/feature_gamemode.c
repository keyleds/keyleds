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

enum leds_feature_function {
    F_GET_MAX = 0,
    F_BLOCK_KEYS = 1,
    F_UNBLOCK_KEYS = 2,
    F_CLEAR = 3
};

#define KEYS_PER_COMMAND    (16)


bool keyleds_gamemode_max(Keyleds * device, uint8_t target_id, unsigned * nb)
{
    uint8_t data[1];

    assert(device != NULL);
    assert(nb != NULL);

    if (keyleds_call(device, data, sizeof(data),
                     target_id, KEYLEDS_FEATURE_GAMEMODE, F_GET_MAX,
                     0, NULL) < 0) {
        return false;
    }
    *nb = (unsigned)data[0];
    return true;
}

static bool gamemode_send(Keyleds * device, uint8_t target_id,
                          const uint8_t * ids, unsigned ids_nb, bool set)
{
    assert(device != NULL);
    assert(ids != NULL);

    for (unsigned offset = 0; offset < ids_nb; offset += KEYS_PER_COMMAND) {
        unsigned batch_size = KEYS_PER_COMMAND;
        if (batch_size > ids_nb - offset) { batch_size = ids_nb - offset; }
        if (keyleds_call(device, NULL, 0,
                         target_id, KEYLEDS_FEATURE_GAMEMODE, set ? F_BLOCK_KEYS : F_UNBLOCK_KEYS,
                         batch_size, ids + offset) < 0) {
            return false;
        }
    }
    return true;
}

bool keyleds_gamemode_set(Keyleds * device, uint8_t target_id, const uint8_t * ids, unsigned ids_nb)
{
    return gamemode_send(device, target_id, ids, ids_nb, true);
}

bool keyleds_gamemode_clear(Keyleds * device, uint8_t target_id, const uint8_t * ids, unsigned ids_nb)
{
    return gamemode_send(device, target_id, ids, ids_nb, false);
}

bool keyleds_gamemode_reset(Keyleds * device, uint8_t target_id)
{
    assert(device != NULL);

    if (keyleds_call(device, NULL, 0,
                     target_id, KEYLEDS_FEATURE_GAMEMODE, F_CLEAR, 0, NULL) < 0) {
        return false;
    }
    return true;
}
