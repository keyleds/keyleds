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
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/device.h"
#include "keyleds/error.h"
#include "keyleds/features.h"
#include "keyleds/hid_parser.h"
#include "keyleds/logging.h"


Keyleds * keyleds_open(const char * path, uint8_t app_id)
{
    Keyleds * dev = malloc(sizeof(Keyleds));
    struct hidraw_report_descriptor descriptor;
    unsigned version;

    dev->app_id = app_id;
    do { dev->ping_seq = rand(); } while (dev->ping_seq == 0);
    dev->timeout = KEYLEDS_CALL_TIMEOUT_US;

    /* Open device */
    KEYLEDS_LOG(DEBUG, "Opening device %s", path);
    if ((dev->fd = open(path, O_RDWR)) < 0) {
        keyleds_set_error_errno();
        goto error_free_dev;
    }
    fcntl(dev->fd, F_SETFD, FD_CLOEXEC);

    /* Read REPORT descriptor */
    if (ioctl(dev->fd, HIDIOCGRDESCSIZE, &descriptor.size) < 0) {
        keyleds_set_error_errno();
        goto error_close_fd;
    }
    if (ioctl(dev->fd, HIDIOCGRDESC, &descriptor) < 0) {
        keyleds_set_error_errno();
        goto error_close_fd;
    }
    KEYLEDS_LOG(DEBUG, "Parsing report descriptor (%d bytes)", descriptor.size);

    /* Parse report descriptor */
    if (!keyleds_parse_hid(descriptor.value, descriptor.size,
                           &dev->reports, &dev->max_report_size)) {
        keyleds_set_error(KEYLEDS_ERROR_HIDREPORT);
        goto error_close_fd;
    }
    if (dev->max_report_size == 0) {
        keyleds_set_error(KEYLEDS_ERROR_HIDREPORT);
        goto error_free_reports;
    }

    if (!keyleds_get_protocol(dev, KEYLEDS_TARGET_DEFAULT, &version, NULL)) {
        goto error_free_reports;
    }

    if (version < 2) {
        keyleds_set_error(KEYLEDS_ERROR_HIDVERSION);
        goto error_free_reports;
    }

    if (!keyleds_ping(dev, KEYLEDS_TARGET_DEFAULT)) {
        goto error_free_reports;
    }

    dev->features = malloc(sizeof(struct keyleds_device_feature));
    dev->features[0].id = 0;

    KEYLEDS_LOG(INFO, "Opened device %s protocol version %d", path, version);
    return dev;

error_free_reports:
    free(dev->reports);
error_close_fd:
    close(dev->fd);
error_free_dev:
    free(dev);
    return NULL;
}

void keyleds_close(Keyleds * device)
{
    assert(device != NULL);
    close(device->fd);
    free(device->reports);
    free(device->features);
    free(device);
}

void keyleds_set_timeout(Keyleds * device, unsigned us)
{
    assert(device != NULL);
    device->timeout = us;
}

int keyleds_device_fd(Keyleds * device)
{
    assert(device != NULL);
    return device->fd;
}

bool keyleds_flush_fd(Keyleds * device)
{
    assert(device != NULL);
    uint8_t buffer[device->max_report_size + 1];
    ssize_t nread;

    fcntl(device->fd, F_SETFL, O_NONBLOCK);
    while ((nread = read(device->fd, buffer, device->max_report_size + 1)) > 0) {
        /* do nothing */
    }
    if (errno != EAGAIN) {
        keyleds_set_error_errno();
        return false;
    }
    fcntl(device->fd, F_SETFL, 0);
    return true;
}

/****************************************************************************/

#ifndef NDEBUG
static void format_buffer(const uint8_t * data, unsigned size, char * buffer)
{
    for (unsigned idx = 0; idx < size; idx += 1) {
        sprintf(buffer + 3 * idx, "%02x ", data[idx]);
    }
    buffer[3 * size - 1] = '\0';
}
#endif


bool keyleds_send(Keyleds * device, uint8_t target_id, uint8_t feature_idx,
                  uint8_t function, size_t length, const uint8_t * data)
{
    assert(device != NULL);
    assert(function <= 0xf);
    assert(length + 3 <= device->max_report_size);
    assert(length == 0 || data != NULL);

    unsigned idx = 0;
    while (device->reports[idx].size < 3 + length) { idx += 1; }

    size_t report_size = device->reports[idx].size;
    uint8_t buffer[1 + report_size];

    buffer[0] = device->reports[idx].id;
    buffer[1] = target_id;
    buffer[2] = feature_idx;
    buffer[3] = function << 4 | device->app_id;
    memcpy(&buffer[4], data, length);
    memset(&buffer[4 + length], 0, report_size - 3 - length);

#ifndef NDEBUG
    if (g_keyleds_debug_level >= KEYLEDS_LOG_DEBUG) {
        char debug_buffer[3 * (1 + report_size) + 1];
        format_buffer(buffer, (1 + report_size), debug_buffer);
        KEYLEDS_LOG(DEBUG, "Send [%s]", debug_buffer);
    }
#endif

    ssize_t nwritten = write(device->fd, buffer, 1 + report_size);
    if (nwritten < 0) {
        keyleds_set_error_errno();
        return false;
    }
    if ((size_t)nwritten != 1 + report_size) {
        KEYLEDS_LOG(DEBUG, "Unexpected write size %zd on fd %d", nwritten, device->fd);
        keyleds_set_error(KEYLEDS_ERROR_IO_LENGTH);
        return false;
    }
    return true;
}

bool keyleds_receive(Keyleds * device, uint8_t target_id, uint8_t feature_idx,
                     uint8_t * message, size_t * size)
{
    int err, idx;
    ssize_t nread;

    assert(device != NULL);
    assert(message != NULL);

    do {
        if (device->timeout > 0) {
            fd_set set;
            struct timeval timeout;

            FD_ZERO(&set);
            FD_SET(device->fd, &set);
            timeout.tv_sec = KEYLEDS_CALL_TIMEOUT_US / 1000000;
            timeout.tv_usec = KEYLEDS_CALL_TIMEOUT_US % 1000000;
            if ((err = select(device->fd + 1, &set, NULL, NULL, &timeout)) < 0) {
                keyleds_set_error_errno();
                return false;
            }
            if (err == 0) {
                KEYLEDS_LOG(INFO, "Device timeout while reading fd %d", device->fd);
                keyleds_set_error(KEYLEDS_ERROR_TIMEDOUT);
                return false;
            }
        }

        if ((nread = read(device->fd, message, device->max_report_size + 1)) < 0) {
            keyleds_set_error_errno();
            return false;
        }
#ifndef NDEBUG
        if (g_keyleds_debug_level >= KEYLEDS_LOG_DEBUG) {
            char debug_buffer[3 * nread + 1];
            format_buffer(message, nread, debug_buffer);
            KEYLEDS_LOG(DEBUG, "Recv [%s]", debug_buffer);
        }
#endif
        for (idx = 0; device->reports[idx].id != DEVICE_REPORT_INVALID; idx += 1)
        {
            if (device->reports[idx].id == message[0]) { break; }
        }
        if (device->reports[idx].id == DEVICE_REPORT_INVALID) { continue; }

        if (nread != 1 + device->reports[idx].size) {
            KEYLEDS_LOG(DEBUG, "Unexpected read size %zd on fd %d", nread, device->fd);
            keyleds_set_error(KEYLEDS_ERROR_IO_LENGTH);
            return false;
        }

    } while(!(
        message[1] == target_id && (                /* message is from this device */
        (
            message[2] == feature_idx &&            /* message is for correct feature */
            (message[3] & 0xf) == device->app_id    /* message is for us */
        ) || (
            message[2] == 0xff &&                   /* message is an error */
            message[3] == feature_idx &&            /* message if for correct feature */
            (message[4] & 0xf) == device->app_id    /* message is for us */
        ) || (                                          /* special handling for getprotocol */
            message[2] == 0x8f &&                       /* message is HIDPP1 error */
            message[3] == KEYLEDS_FEATURE_IDX_ROOT &&   /* feature is root feature */
            (message[4] & 0xf) == device->app_id        /* message is for us */
        )
    )));

    if (message[2] == 0xff) {
        keyleds_set_error_hidpp(message[5]);
        return false;
    }

    if (size) { *size = nread; }
    return true;
}

int keyleds_call(Keyleds * device, uint8_t * result, size_t result_len,
    uint8_t target_id, uint16_t feature_id, uint8_t function,
    size_t length, const uint8_t * data)
{
    assert(device != NULL);
    assert(result != NULL || result_len == 0);
    assert(function <= 0xf);

    uint8_t feature_idx;
    if (feature_id == KEYLEDS_FEATURE_ROOT) {
        feature_idx = KEYLEDS_FEATURE_IDX_ROOT;
    } else {
        feature_idx = keyleds_get_feature_index(device, target_id, feature_id);
        if (feature_idx == 0) { return -1; }
    }

    if (!keyleds_send(device, target_id, feature_idx, function, length, data)) { return -1; }

    uint8_t buffer[1 + device->max_report_size];
    size_t nread;
    if (!keyleds_receive(device, target_id, feature_idx, buffer, &nread)) { return -1; }

    const uint8_t * res_data = keyleds_response_data(device, buffer);
    size_t ret = nread - (res_data - buffer);
    if (result_len < ret) { ret = result_len; }
    if (result != NULL) { memcpy(result, res_data, ret); }

    return ret;
}
