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
#ifndef KEYLEDS_DEVICE_H
#define KEYLEDS_DEVICE_H

#include <stdint.h>

struct keyleds_device_reports {
    uint8_t     id;
    uint8_t     size;
};
#define DEVICE_REPORT_INVALID   (0xff)

struct keyleds_device_feature {
    uint8_t     target_id;
    uint16_t    id;
    uint8_t     index;
    bool        reserved;
    bool        hidden;
    bool        obsolete;
};

struct keyleds_device {
    int         fd;                             /* device file descriptor */
    uint8_t     app_id;                         /* our application identifier */
    uint8_t     ping_seq;                       /* using for resyncing after errors */
    unsigned    timeout;                        /* read timeout in microseconds */

    struct keyleds_device_reports * reports;    /* list of device-supported hid reports */
    unsigned    max_report_size;                /* maximum number of bytes in a report */

    struct keyleds_device_feature * features;   /* feature index cache */
};

/****************************************************************************/
/* Core functions */

bool keyleds_send(Keyleds * device, uint8_t target_id, uint8_t feature_idx,
                  uint8_t function, size_t length, const uint8_t * data);
bool keyleds_receive(Keyleds * device, uint8_t target_id, uint8_t feature_idx,
                     uint8_t * message, size_t * size);
int keyleds_call(Keyleds * device, /*@null@*/ /*@out@*/ uint8_t * result, size_t result_len,
                 uint8_t target_id, uint16_t feature_id, uint8_t function,
                 size_t length, const uint8_t * data);

/****************************************************************************/
/* Helpers */

static inline const uint8_t * keyleds_response_data(Keyleds * device, const uint8_t * message)
{
    (void)device;
    return message + 4;
}

/****************************************************************************/


#endif
