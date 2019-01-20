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

enum version_feature_function {     /* Function table for KEYLEDS_FEATURE_VERSION */
    F_GET_DEVICE_INFO = 0,
    F_GET_FIRMWARE_INFO = 1
};

enum devinfo_feature_function {     /* Function table for KEYLEDS_FEATURE_NAME */
    F_GET_NAME_LENGTH = 0,
    F_GET_NAME = 1,
    G_GET_TYPE = 2
};


/** Query firmware version.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param [out] out A pointer that will hold the version information on success.
 *                  Must be freed with keyleds_free_device_version().
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_get_device_version(Keyleds * device, uint8_t target_id,
                                               struct keyleds_device_version ** out)
{
    uint8_t data[16];   /* max of F_GET_DEVICE_INFO and F_GET_FIRMWARE_INFO */
    unsigned length, idx;
    struct keyleds_device_version * info;

    assert(device != NULL);
    assert(out != NULL);

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_VERSION, F_GET_DEVICE_INFO,
                     0, NULL) < 0) {
        return false;
    }

    length = (unsigned)data[0];
    info = malloc(sizeof(*info) + length * sizeof(info->protocols[0]));
    if (info == NULL) { keyleds_set_error_errno(); return false;}
    memcpy(info->serial, &data[1], 4);
    info->transport = (uint16_t)(data[5] << 8 | data[6]);
    memcpy(info->model, &data[7], 6);
    info->length = length;

    /* Call F_GET_FIRMWARE_INFO for each firmware slot reported by F_GET_DEVICE_INFO */
    for (idx = 0; idx < length; idx += 1) {
        if (keyleds_call(device, data, (unsigned)sizeof(data),
                         target_id, KEYLEDS_FEATURE_VERSION, F_GET_FIRMWARE_INFO,
                         1, (uint8_t[]){(uint8_t)idx}) < 0) {
            goto err_get_dev_info_free;
        }
        info->protocols[idx].type = data[0];
        memcpy(info->protocols[idx].prefix, &data[1], 3);
        info->protocols[idx].prefix[3] = '\0';      /* device-provided string is not nul-terminated */
        info->protocols[idx].version_major = 100
                                           +  10 * (unsigned)(data[4] >> 4)
                                           +   1 * (unsigned)(data[4] & 0xf);
        info->protocols[idx].version_minor =  10 * (unsigned)(data[5] >> 4)
                                           +   1 * (unsigned)(data[5] & 0xf);
        info->protocols[idx].build = (unsigned)((data[6] << 8) | data[7]);
        info->protocols[idx].is_active = (data[8] & (1<<0)) != 0;
        info->protocols[idx].product_id = (uint16_t)((data[9] << 8) | data[10]);
        memcpy(info->protocols[idx].misc, &data[11], 5);
    }

    *out = info;
    return true;

err_get_dev_info_free:
    free(info);
    return false;
}


/** Free version data returned by keyleds_get_device_version().
 * @param version Version data to free. Can be `NULL`.
 */
KEYLEDS_EXPORT void keyleds_free_device_version(struct keyleds_device_version * version)
{
    free(version);
}


/** Query device name.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param [out] out A pointer that will hold the name on success, in UTF8 format.
 *                  Must be freed with keyleds_free_device_name().
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_get_device_name(Keyleds * device, uint8_t target_id, char ** out)
{
    uint8_t data[1];
    unsigned length, done;
    char * buffer;

    assert(device != NULL);
    assert(out != NULL);

    /* Step 1 - ask device how many bytes are needed to get its name */
    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_NAME, F_GET_NAME_LENGTH, 0, NULL) < 0) {
        return false;
    }

    length = (unsigned)data[0];
    if ((buffer = malloc((length + 1) * sizeof(char))) == NULL) {
        keyleds_set_error_errno();
        return false;
    }

    /* Step 2 - retrieve name, chunk by chunk as report size allows */
    done = 0;
    while (done < length) {
        ssize_t nread = keyleds_call(device, (uint8_t*)buffer + done, length - done,
                                     target_id, KEYLEDS_FEATURE_NAME, F_GET_NAME,
                                     1, (uint8_t[]){(uint8_t)done});
        if (nread < 0) {
            free(buffer);
            return false;
        }

        done += (unsigned)nread;
    }
    buffer[length] = '\0';  /* device-provided string is not nul-terminated */

    *out = buffer;
    return true;
}

/** Free device name returned by keyleds_get_device_name().
 * @param name Name to free. Can be `NULL`.
 */
KEYLEDS_EXPORT void keyleds_free_device_name(char * name)
{
    free(name);
}


/** Query device name.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier. See keyleds_open().
 * @param [out] out Returned device type.
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_get_device_type(Keyleds * device, uint8_t target_id,
                                            keyleds_device_type_t * out)
{
    uint8_t data[1];

    assert(device != NULL);
    assert(out != NULL);

    if (keyleds_call(device, data, (unsigned)sizeof(data),
                     target_id, KEYLEDS_FEATURE_NAME, G_GET_TYPE, 0, NULL) < 0) {
        return false;
    }

    *out = (keyleds_device_type_t)data[0];
    return true;
}
