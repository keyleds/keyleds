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

static thread_local char keyleds_error_buffer[256];
static thread_local const char * keyleds_error = "no error";

const char * keyleds_get_error_str()
{
    return keyleds_error;
}

void keyleds_set_error_errno()
{
#ifdef POSIX_STRERROR_R_FOUND
    strerror_r(errno, keyleds_error_buffer, sizeof(keyleds_error_buffer));
    keyleds_error = keyleds_error_buffer;
#else
    keyleds_error = strerror(errno);
#endif
}

void keyleds_set_error_string(const char * str)
{
    assert(str != NULL);
    keyleds_error = str;
}
