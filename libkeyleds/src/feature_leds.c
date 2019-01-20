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

enum leds_feature_function {    /* Function table for KEYLEDS_FEATURE_LEDS */
    F_GET_KEYBLOCKS = 0,
    F_GET_BLOCK_INFO = 1,
    F_GET_LEDS = 2,
    F_SET_LEDS = 3,
    F_SET_BLOCK_LEDS = 4,
    F_COMMIT = 5
};


/** Get the full description of LED blocks.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param [out] out Address of a pointer that will hold the description on success.
 *                  Must be freed using keyleds_free_block_info().
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_get_block_info(Keyleds * device, uint8_t target_id,
                                           struct keyleds_keyblocks_info ** out)
{
    uint8_t data[5];    /* max of F_GET_KEYBLOCKS and F_GET_BLOCK_INFO */
    unsigned idx, info_idx, length;
    uint16_t mask;
    struct keyleds_keyblocks_info * info;

    assert(device != NULL);
    assert(out != NULL);

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_LEDS, F_GET_KEYBLOCKS, 0, NULL) < 0) {
        return false;
    }

    /* mask is a bitfield, each bit tells the presence or absence of a block */
    mask = (uint16_t)((data[0] << 8) | data[1]);

    /* count blocks */
    length = 0;
    for (idx = 0; idx < 16; idx += 1) {
        if ((mask & (1 << idx)) != 0) { length += 1; }
    }
    if (length == 0) { return false; }

    /* initialize block info structure with space enough for the blocks */
    info = malloc(sizeof(*info) + length * sizeof(info->blocks[0]));
    if (!info) { keyleds_set_error_errno(); return false; }
    info->length = length;

    /* use the block info query on every block */
    info_idx = 0;
    for (idx = 0; idx < 16; idx += 1) {
        keyleds_block_id_t block_id = (keyleds_block_id_t)(1<<idx);
        if ((mask & block_id) == 0) { continue; }

        if (keyleds_call(device, data, (unsigned)sizeof(data),
                         target_id, KEYLEDS_FEATURE_LEDS, F_GET_BLOCK_INFO,
                         2, (uint8_t[]){(uint8_t)(block_id >> 8), (uint8_t)block_id}) < 0) { continue; }

        info->blocks[info_idx].block_id = block_id;
        info->blocks[info_idx].nb_keys = (uint16_t)(data[0] << 8 | data[1]);
        info->blocks[info_idx].red = data[2];
        info->blocks[info_idx].green = data[3];
        info->blocks[info_idx].blue = data[4];
        info_idx += 1;
    }

    *out = info;
    return true;
}


/** Free LED block description returned by keyleds_get_block_info().
 * @param info Description to free. Can be `NULL`.
 */
KEYLEDS_EXPORT void keyleds_free_block_info(struct keyleds_keyblocks_info * info)
{
    free(info);
}


/** Retrieve current LED status for a block.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param block_id Block to target.
 * @param [out] keys Color table with `keys_nb` entries, filled upon success.
 * @param offset Which key to start at.
 * @param keys_nb Number of keys to retrieve.
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_get_leds(Keyleds * device, uint8_t target_id,
                                     keyleds_block_id_t block_id,
                                     struct keyleds_key_color * keys, uint16_t offset, unsigned keys_nb)
{
    unsigned done = 0;

    assert(device != NULL);
    assert((unsigned)block_id <= UINT16_MAX);
    assert(keys != NULL);
    assert(keys_nb + offset <= UINT16_MAX);

    /* retrieve keys in chunks, as big as max_report_size allows */
    while (done < keys_nb) {
        uint8_t data[device->max_report_size];
        ssize_t data_size;
        unsigned data_offset;

        data_size = keyleds_call(device, data, sizeof(data),
                                 target_id, KEYLEDS_FEATURE_LEDS, F_GET_LEDS,
                                 4, (uint8_t[]){(uint8_t)(block_id >> 8), (uint8_t)block_id,
                                                (uint8_t)(offset >> 8), (uint8_t)offset});
        if (data_size < 0) { return false; }
        if (data[2] != (offset >> 8) ||     /* validate the offset in the reply is correct */
            data[3] != (offset & 0x00ff)) {
            keyleds_set_error(KEYLEDS_ERROR_RESPONSE);
            return false;
        }

        for (data_offset = 4; data_offset < (unsigned)data_size; data_offset += 4) {
            keys[done].id = data[data_offset];
            keys[done].red = data[data_offset + 1];
            keys[done].green = data[data_offset + 2];
            keys[done].blue = data[data_offset + 3];
            done += 1;
            offset = (uint16_t)(offset + 1);
            if (done >= keys_nb) { break; }
        }
    }
    return true;
}


/** Set the color of a set of LEDs.
 * Updates an internal buffer on the device. Actual lights are not updated until
 * keyleds_commit_leds() is called.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param block_id Block to target, all keys must belong to this block.
 * @param keys Color table with `keys_nb` entries.
 * @param keys_nb Number of keys to send.
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_set_leds(Keyleds * device, uint8_t target_id,
                                     keyleds_block_id_t block_id,
                                     const struct keyleds_key_color * keys, unsigned keys_nb)
{
    assert(device != NULL);
    assert((unsigned)block_id <= UINT16_MAX);
    assert(keys != NULL);
    assert(keys_nb <= UINT16_MAX);

    unsigned per_call = (device->max_report_size - 3 - 4) / 4;  /* 4 bytes per key, mins headers */
    unsigned offset, idx;

    uint8_t data[4 + per_call * 4];
    data[0] = (uint8_t)(block_id >> 8);
    data[1] = (uint8_t)(block_id >> 0);

    /* Send keys in chunks */
    for (offset = 0; offset < keys_nb; offset += per_call) {
        unsigned batch_length = offset + per_call > keys_nb ? keys_nb - offset : per_call;
        data[2] = (uint8_t)(batch_length >> 8);
        data[3] = (uint8_t)(batch_length >> 0);
        for (idx = 0; idx < batch_length; idx += 1) {
            data[4 + idx * 4 + 0] = keys[offset + idx].id;
            data[4 + idx * 4 + 1] = keys[offset + idx].red;
            data[4 + idx * 4 + 2] = keys[offset + idx].green;
            data[4 + idx * 4 + 3] = keys[offset + idx].blue;
        }

        if (keyleds_call(device, NULL, 0,
                         target_id, KEYLEDS_FEATURE_LEDS, F_SET_LEDS,
                         4 + batch_length * 4, data) < 0) {
            return false;
        }
    }
    return true;
}


/** Reset a full LED block to uniform color.
 * Updates an internal buffer on the device. Actual lights are not updated until
 * keyleds_commit_leds() is called.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param block_id Block to target.
 * @param red Red brightness from 0 (off) to 255 (full power).
 * @param green Green brightness from 0 (off) to 255 (full power).
 * @param blue Blue brightness from 0 (off) to 255 (full power).
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_set_led_block(Keyleds * device, uint8_t target_id,
                                          keyleds_block_id_t block_id,
                                          uint8_t red, uint8_t green, uint8_t blue)
{
    assert(device != NULL);
    assert((unsigned)block_id <= UINT16_MAX);
    return keyleds_call(device, NULL, 0, target_id, KEYLEDS_FEATURE_LEDS, F_SET_BLOCK_LEDS,
                        5, (uint8_t[]){(uint8_t)(block_id >> 8), (uint8_t)block_id,
                                       red, green, blue}) >= 0;
}


/** Apply LED state changes.
 * Other functions only update an internal buffer on the device. The actual colors
 * of the lights are only shown after a commit.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @return `true` on success, `false` on error.
 * @note This device function takes relatively longer than most others. Testing on a G410
 *       showed 10 to 15 milliseconds runtimes.
 */
KEYLEDS_EXPORT bool keyleds_commit_leds(Keyleds * device, uint8_t target_id)
{
    assert(device != NULL);
    return keyleds_call(device, NULL, 0, target_id, KEYLEDS_FEATURE_LEDS, F_COMMIT,
                        0, NULL) >= 0;
}
