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
#include <errno.h>
#include <string.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/error.h"
#include "keyleds/logging.h"

static const char * const error_strings[] = {
    "no error",
    NULL,                                       /* errno-based error */
    NULL,                                       /* device_error_string */
    "wrong I/O length",
    "invalid device (could not parse report descriptor)",
    "invalid device (no hid++ support)",
    "invalid device (hid++ v1)",
    "feature not found on device",
    "synchronization with device failed",
    "invalid response from device",
    "invalid argument"
};

static const char * const device_error_strings[] = {
    "no error",
    "unknown device error",
    "invalid argument sent to device",
    "out of range value sent to device",
    "hardware error",
    "internal logitech error",
    "invalid feature index sent to device",
    "invalid function id sent to device",
    "device busy",
    "unsupported operation"
};


static thread_local keyleds_error_t keyleds_errno = KEYLEDS_NO_ERROR;
static thread_local int keyleds_saved_errno = 0;
#ifdef POSIX_STRERROR_R_FOUND
static thread_local char keyleds_error_buffer[256];
#endif


/** Get user-readable message for last failed libkeyleds call.
 * @return A pointer to the error message. It does not have to be freed, but
 *         it is only valid until this function is called again in the same thread.
 */
KEYLEDS_EXPORT const char * keyleds_get_error_str()
{
    if (keyleds_errno == KEYLEDS_ERROR_ERRNO) {
#ifdef POSIX_STRERROR_R_FOUND
        strerror_r(keyleds_saved_errno, keyleds_error_buffer,
                   sizeof(keyleds_error_buffer));
        return keyleds_error_buffer;
#else
        return strerror(keyleds_saved_errno);
#endif
    } else if (keyleds_errno == KEYLEDS_ERROR_DEVICE) {
        return device_error_strings[keyleds_saved_errno];
    }
    return error_strings[keyleds_errno];
}


/** Get error code for last failed libkeyleds call.
 * If the error is of type `KEYLEDS_ERROR_ERRNO`, the value of `errno` that was
 * saved at the time of error is copied back into `errno`.
 */
KEYLEDS_EXPORT keyleds_error_t keyleds_get_errno()
{
    if (keyleds_errno == KEYLEDS_ERROR_ERRNO) {
        errno = keyleds_saved_errno;
    }
    return keyleds_errno;
}


/** Set an error condition initiated by the operating system.
 * Uses the error code from `errno`.
 */
void keyleds_set_error_errno()
{
    keyleds_errno = KEYLEDS_ERROR_ERRNO;
    keyleds_saved_errno = errno;
    KEYLEDS_LOG(DEBUG, "%s", keyleds_get_error_str());
}


/** Set an error condition initiated by the device through an HIDPP error.
 * @param code Error value received from device.
 */
void keyleds_set_error_hidpp(uint8_t code)
{
    if (code >= sizeof(device_error_strings) / sizeof(device_error_strings[0])) {
        code = 1;
    }
    keyleds_errno = KEYLEDS_ERROR_DEVICE;
    keyleds_saved_errno = (signed)(unsigned)code;
    KEYLEDS_LOG(DEBUG, "%s", device_error_strings[code]);
}


/** Set an error condition initiated by libkeyleds.
 * @param err Error code.
 */
void keyleds_set_error(keyleds_error_t err)
{
    assert(err >= 0);
    assert(err != KEYLEDS_ERROR_ERRNO);
    assert(err != KEYLEDS_ERROR_DEVICE);
    assert((unsigned)err < sizeof(error_strings) / sizeof(error_strings[0]));
    keyleds_errno = err;
    KEYLEDS_LOG(DEBUG, "%s", error_strings[err]);
}
