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
#include <stdint.h>
#include <stdlib.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/command.h"
#include "keyleds/device.h"
#include "keyleds/features.h"
#include "keyleds/logging.h"

enum root_feature_function {
    F_GET_FEATURE = 0,
    F_PING = 1
};
enum feature_feature_function {
    F_GET_FEATURE_COUNT = 0,
    F_GET_FEATURE_ID = 1
};

bool keyleds_get_protocol(struct keyleds_device * device, uint8_t target_id,
                          unsigned * version, keyleds_device_handler_t * handler)
{
    KeyledsCommand * command, * response;
    bool result = false;

    command = keyleds_command_alloc(
        target_id, KEYLEDS_FEATURE_IDX_ROOT, F_PING,
        device->app_id, 0
    );
    response = keyleds_command_alloc_empty(2);

    if (keyleds_send(device, command) &&
        keyleds_receive(device, response)) {
        if (response->feature_idx == KEYLEDS_FEATURE_IDX_ROOT &&
            response->function == F_PING &&
            response->app_id == device->app_id) {
            if (version != NULL) { *version = (unsigned)response->data[0]; }
            if (handler != NULL) { *handler = (keyleds_device_handler_t)response->data[1]; }
            result = true;
        }
    }

    keyleds_command_free(command);
    keyleds_command_free(response);
    return result;
}

/* Re-synchronize exchanges: send a ping and discard all received
 * reports until we get the matching pong.
 */
bool keyleds_ping(Keyleds * device, uint8_t target_id)
{
    KeyledsCommand * command, * response;
    bool result = false;

    uint8_t payload = device->ping_seq;
    device->ping_seq = payload == UINT8_MAX ? (uint8_t)1 : payload + 1;

    command = keyleds_command_alloc(
        target_id, KEYLEDS_FEATURE_IDX_ROOT, F_PING,
        device->app_id, 3);
    command->data[0] = 0;
    command->data[1] = 0;
    command->data[2] = payload;
    response = keyleds_command_alloc_empty(3);

    if (!keyleds_send(device, command)) { goto ping_err_free_commands; }

    do {
        response->length = 3;   /* gets overwritten at each invocation */
        if (!keyleds_receive(device, response)) { goto ping_err_free_commands; }

        if (response->feature_idx == KEYLEDS_FEATURE_IDX_ROOT &&
            response->function == F_PING &&
            response->app_id == device->app_id &&
            response->data[2] == payload) {
            result = true;
        }
    } while (!result);

ping_err_free_commands:
    keyleds_command_free(command);
    keyleds_command_free(response);
    return result;
}

unsigned keyleds_get_feature_count(struct keyleds_device * device, uint8_t target_id)
{
    uint8_t data[1];
    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_FEATURE, F_GET_FEATURE_COUNT, 0) < 0) {
        return 0;
    }
    return (unsigned)data[0];
}

uint16_t keyleds_get_feature_id(struct keyleds_device * device,
                                uint8_t target_id, uint8_t feature_idx)
{
    unsigned idx;
    uint16_t feature_id;
    uint8_t data[3];

    if (feature_idx == KEYLEDS_FEATURE_IDX_ROOT) { return KEYLEDS_FEATURE_ROOT; }
    if (feature_idx == KEYLEDS_FEATURE_IDX_FEATURE) { return KEYLEDS_FEATURE_FEATURE; }

    for (idx = 0; device->features[idx].id != 0; idx += 1) {
        if (device->features[idx].target_id == target_id &&
            device->features[idx].index == feature_idx) {
            return device->features[idx].id;
        }
    }

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_FEATURE, F_GET_FEATURE_ID,
                     1, feature_idx) < 0) {
        KEYLEDS_LOG(ERROR, "get_feature_id failed");
        return 0;
    }

    feature_id = (data[0] << 8) | data[1];
    device->features = realloc(device->features, (idx + 2) * sizeof(device->features[0]));
    device->features[idx].target_id = target_id;
    device->features[idx].id = feature_id;
    device->features[idx].index = feature_idx;
    device->features[idx].reserved = (data[2] & (1<<5)) != 0;
    device->features[idx].hidden = (data[2] & (1<<6)) != 0;
    device->features[idx].obsolete = (data[2] & (1<<7)) != 0;
    device->features[idx + 1].id = 0;
    KEYLEDS_LOG(DEBUG, "feature %04x is at %d [%02x]",
                       feature_id, feature_idx, data[2]);
    return feature_id;
}

uint8_t keyleds_get_feature_index(struct keyleds_device * device,
                                  uint8_t target_id, uint16_t feature_id)
{
    unsigned idx;
    uint8_t feature_idx;
    uint8_t data[2];

    if (feature_id == KEYLEDS_FEATURE_ROOT) { return KEYLEDS_FEATURE_IDX_ROOT; }
    if (feature_id == KEYLEDS_FEATURE_FEATURE) { return KEYLEDS_FEATURE_IDX_FEATURE; }

    for (idx = 0; device->features[idx].id != 0; idx += 1) {
        if (device->features[idx].target_id == target_id &&
            device->features[idx].id == feature_id) {
            return device->features[idx].index;
        }
    }

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_ROOT, F_GET_FEATURE,
                     2, feature_id >> 8, feature_id) < 0) {
        KEYLEDS_LOG(ERROR, "get_feature_index failed");
        return 0;
    }

    feature_idx = data[0];
    device->features = realloc(device->features, (idx + 2) * sizeof(device->features[0]));
    device->features[idx].target_id = target_id;
    device->features[idx].id = feature_id;
    device->features[idx].index = feature_idx;
    device->features[idx].reserved = (data[1] & (1<<5)) != 0;
    device->features[idx].hidden = (data[1] & (1<<6)) != 0;
    device->features[idx].obsolete = (data[1] & (1<<7)) != 0;
    device->features[idx + 1].id = 0;
    KEYLEDS_LOG(DEBUG, "feature %04x is at %d [%02x]",
                       feature_id, feature_idx, data[1]);
    return feature_idx;
}
