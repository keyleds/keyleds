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
#include "keyleds/command.h"
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

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_GAMEMODE, F_GET_MAX, 0) < 0) {
        return false;
    }
    *nb = (unsigned)data[0];
    return true;
}

static bool gamemode_send(Keyleds * device, uint8_t target_id,
                          const uint8_t * ids, unsigned ids_nb, bool set)
{
    struct keyleds_command * command = NULL, * response = NULL;
    unsigned offset, idx;
    uint8_t feature_idx;
    bool result = false;

    assert(device != NULL);
    assert(ids != NULL);

    feature_idx = keyleds_get_feature_index(device, target_id,
                                            KEYLEDS_FEATURE_GAMEMODE);
    if (feature_idx == 0) { return false; }

    command = keyleds_command_alloc(target_id, feature_idx, set ? F_BLOCK_KEYS : F_UNBLOCK_KEYS,
                                    device->app_id, KEYS_PER_COMMAND);
    if (command == NULL) { goto err_gamemode_set_free_cmd; }
    response = keyleds_command_alloc_empty(0);
    if (response == NULL) { goto err_gamemode_set_free_cmd; }

    for (offset = 0; offset < ids_nb; offset += KEYS_PER_COMMAND) {
        for(idx = 0; idx < KEYS_PER_COMMAND && idx + offset < ids_nb; idx += 1) {
            command->data[idx] = ids[idx + offset];
        }
        command->length = idx;
        if (!keyleds_call_command(device, command, response)) {
            goto err_gamemode_set_free_cmd;
        }
    }

    result = true;

err_gamemode_set_free_cmd:
    keyleds_command_free(command);
    keyleds_command_free(response);
    return result;
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
                     target_id, KEYLEDS_FEATURE_GAMEMODE, F_CLEAR, 0) < 0) {
        return false;
    }
    return true;
}
