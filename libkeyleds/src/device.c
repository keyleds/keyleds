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


/** Open a device file.
 * @param path Path to a HID device node to open.
 * @param app_id Application identifier to use for all communication with the device.
 * @return Opaque pointer representing the device, or `NULL` on failure, in which case
 *         the error can be retrieved with keyleds_get_errno().
 * The opening process checks the device's report descriptor, and only talks to it if
 * it matches a supported protocol. It should be safe to use on any device node.
 * On successful return, the device is fully initialized and can be used with all other
 * functions.
 * @remark The underlying file descriptor will not be inherited on fork, but keyleds_close()
 * still must be closed in the child to free resources.
 * @sa keyleds_close
 */
KEYLEDS_EXPORT Keyleds * keyleds_open(const char * path, uint8_t app_id)
{
    Keyleds * dev = malloc(sizeof(Keyleds));
    struct hidraw_report_descriptor descriptor;
    unsigned version;

    dev->app_id = app_id;
    do { dev->ping_seq = (uint8_t)rand(); } while (dev->ping_seq == 0);
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
        keyleds_set_error(KEYLEDS_ERROR_HIDNOPP);
        goto error_free_reports;
    }

    /* Check device's protocol version */
    if (!keyleds_get_protocol(dev, KEYLEDS_TARGET_DEFAULT, &version, NULL)) {
        goto error_free_reports;
    }

    if (version < 2) { /* all supported devices are version 2 or higher */
        keyleds_set_error(KEYLEDS_ERROR_HIDVERSION);
        goto error_free_reports;
    }

    /* Ensure HIDPP is functionning properly */
    if (!keyleds_ping(dev, KEYLEDS_TARGET_DEFAULT)) {
        goto error_free_reports;
    }

    /* Setup feature table cache */
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

/** Close a device.
 * @param device Opaque pointer returned by keyleds_open().
 * @post All resources have been freed, and given device is no longer valid.
 */
KEYLEDS_EXPORT void keyleds_close(Keyleds * device)
{
    assert(device != NULL);
    close(device->fd);
    free(device->reports);
    free(device->features);
    free(device);
}

/** Set HIDPP command timeout.
 * @param device Open device as returned by keyleds_open().
 * @param us The timeout in microseconds. Special value 0 disables timeouts.
 */
KEYLEDS_EXPORT void keyleds_set_timeout(Keyleds * device, unsigned us)
{
    assert(device != NULL);
    device->timeout = us;
}

/** Get underlying device file descriptor.
 * @param device Open device as returned by keyleds_open().
 */
KEYLEDS_EXPORT int keyleds_device_fd(Keyleds * device)
{
    assert(device != NULL);
    return device->fd;
}

/** Flush inbound report queue.
 * Simply discard inbound messages that may be queued as a result of another process
 * interacting with the device or spontaneous reports due to keypresses.
 * @param device Open device as returned by keyleds_open().
 * @return `true` on success, `false` on error.
 */
KEYLEDS_EXPORT bool keyleds_flush_fd(Keyleds * device)
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
static void format_buffer(const uint8_t * data, size_t size, char * buffer)
{
    for (size_t idx = 0; idx < size; idx += 1) {
        sprintf(buffer + 3 * idx, "%02x ", data[idx]);
    }
    buffer[3 * size - 1] = '\0';
}
#endif


/** Send a report to the device, running an on-device function.
 * May block if the outgoing queue is full.
 * @param device Open device as returned by keyleds_open().
 * @param target_id Device's target identifier, for devices behind a unifying receiver.
 *                  for the receiver itself, or for directly attached devices, use
 *                  KEYLEDS_TARGET_DEFAULT.
 * @param feature_idx Address of the target feature.
 * @param function Code of the function. Meaning depends on specific feature.
 * @param length Size, in bytes of the payload.
 * @param data Pointer to the payload. Unused if length is 0.
 * @return `true` on success, `false` on failure.
 */
bool keyleds_send(Keyleds * device, uint8_t target_id, uint8_t feature_idx,
                  uint8_t function, size_t length, const uint8_t * data)
{
    assert(device != NULL);
    assert(function <= 0xf);
    assert(length + 3 <= device->max_report_size);
    assert(length == 0 || data != NULL);

    /* Find the smallest report type that fits the payload */
    unsigned idx = 0;
    while (device->reports[idx].size < 3 + length) { idx += 1; }

    /* Fill the report */
    size_t report_size = device->reports[idx].size;
    uint8_t buffer[1 + report_size];

    buffer[0] = device->reports[idx].id;
    buffer[1] = target_id;
    buffer[2] = feature_idx;
    buffer[3] = (uint8_t)(function << 4 | device->app_id);
    if (length > 0) { memcpy(&buffer[4], data, length); }
    memset(&buffer[4 + length], 0, report_size - 3 - length);

#ifndef NDEBUG
    if (g_keyleds_debug_level >= KEYLEDS_LOG_DEBUG) {
        char debug_buffer[3 * (1 + report_size) + 1];
        format_buffer(buffer, (1 + report_size), debug_buffer);
        KEYLEDS_LOG(DEBUG, "Send [%s]", debug_buffer);
    }
#endif

    /* Send the report to the device */
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

/** Receive a single report from the device.
 * Wait for incoming reports, filtering out irrelevant ones until either the expected
 * report is received or timeout occurs (see keyleds_set_timeout()).
 * @param device Open device as returned by keyleds_open().
 * @param target_id The device's target identifier, for devices behind a unifying receiver.
 *                  for the receiver itself, or for directly attached devices, use
 *                  KEYLEDS_TARGET_DEFAULT.
 * @param feature_idx Address of the feature we expect a report from.
 * @param [out] message A buffer to write data into. It must be large enough to
 *                      hold `device->max_report_size + 1` bytes.
 * @param [out] size The number of bytes actually written into `message`. May be NULL.
 * @return `true` on success, `false` on failure.
 * @bug Timeout countdown restarts everytime a report is received.
 */
bool keyleds_receive(Keyleds * device, uint8_t target_id, uint8_t feature_idx,
                     uint8_t * message, size_t * size)
{
    int err, idx;
    ssize_t nread;

    assert(device != NULL);
    assert(message != NULL);

    do {
        /* If a timeout is defined, use select() call to wakeup even if no report comes */
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

        /* Read a report from the device */
        if ((nread = read(device->fd, message, device->max_report_size + 1)) < 0) {
            keyleds_set_error_errno();
            return false;
        }
#ifndef NDEBUG
        if (g_keyleds_debug_level >= KEYLEDS_LOG_DEBUG) {
            char debug_buffer[3 * nread + 1];
            format_buffer(message, (size_t)nread, debug_buffer);
            KEYLEDS_LOG(DEBUG, "Recv [%s]", debug_buffer);
        }
#endif
        /* Check the received report type against our known report types */
        for (idx = 0; device->reports[idx].id != DEVICE_REPORT_INVALID; idx += 1)
        {
            if (device->reports[idx].id == message[0]) { break; }
        }
        if (device->reports[idx].id == DEVICE_REPORT_INVALID) { continue; }

        /* Double-check that received report matches the expected size */
        if (nread != 1 + device->reports[idx].size) {
            KEYLEDS_LOG(DEBUG, "Unexpected read size %zd on fd %d", nread, device->fd);
            keyleds_set_error(KEYLEDS_ERROR_IO_LENGTH);
            return false;
        }

    } while(!(
        message[1] == target_id && (                /* message is from this device */
        (
            message[2] == feature_idx &&            /* message is for correct feature */
            (message[3] & 0xf) == device->app_id    /* message is for our application */
        ) || (
            message[2] == 0xff &&                   /* message is an error */
            message[3] == feature_idx &&            /* message if for correct feature */
            (message[4] & 0xf) == device->app_id    /* message is for our application */
        ) || (                                          /* special handling for getprotocol */
            message[2] == 0x8f &&                       /* message is HIDPP1 error */
            message[3] == KEYLEDS_FEATURE_IDX_ROOT &&   /* feature is root feature */
            (message[4] & 0xf) == device->app_id        /* message is for our application */
        )
    )));
    /* All good, we got a valid report */

    if (message[2] == 0xff) {
        /* if report describes an error mark it there and bail out */
        keyleds_set_error_hidpp(message[5]);
        return false;
    }

    if (size) { *size = (size_t)nread; }
    return true;
}


/** Call a function on the device.
 * Send a report to the device, request a function to be run and wait for the result.
 * This is a wrapper for the most common use of keyleds_send() and keyleds_receive().
 * @param device Open device as returned by keyleds_open().
 * @param [out] result Buffer to copy the return payload into. May be NULL if not interested.
 * @param result_len Size of the result buffer. If received payload overflows, it will be
 *                   truncated.
 * @param target_id The device's target identifier, for devices behind a unifying receiver.
 *                  for the receiver itself, or for directly attached devices, use
 *                  KEYLEDS_TARGET_DEFAULT.
 * @param feature_id Code of the feature to call, from one of the `KEYLEDS_FEATURE_*` values.
 * @param function Code of the function. Meaning depends on specific feature.
 * @param length Size of the payload to send in report, pointed to by `data`.
 * @param [in] data Payload to send in report. Unused if `length` is 0.
 * @return The actual number of bytes of the received payload. May be greater than
 *         `result_len` if the result was truncated. On error, -1 is returned instead.
 */
ssize_t keyleds_call(Keyleds * device, uint8_t * result, size_t result_len,
                     uint8_t target_id, uint16_t feature_id, uint8_t function,
                     size_t length, const uint8_t * data)
{
    assert(device != NULL);
    assert(result != NULL || result_len == 0);
    assert(function <= 0xf);

    /* Resolve feature code into feature index */
    uint8_t feature_idx;
    if (feature_id == KEYLEDS_FEATURE_ROOT) {
        /* must be special cased, because KEYLEDS_FEATURE_IDX_ROOT is 0 */
        feature_idx = KEYLEDS_FEATURE_IDX_ROOT;
    } else {
        feature_idx = keyleds_get_feature_index(device, target_id, feature_id);
        if (feature_idx == 0) { return -1; }
    }

    /* Exchange with the device */
    if (!keyleds_send(device, target_id, feature_idx, function, length, data)) { return -1; }

    uint8_t buffer[1 + device->max_report_size];
    size_t nread;
    if (!keyleds_receive(device, target_id, feature_idx, buffer, &nread)) { return -1; }

    /* Copy payload, truncating to protect from buffer overflows */
    const uint8_t * res_data = keyleds_response_data(device, buffer);
    size_t ret = nread - (size_t)(res_data - buffer);
    if (result_len < ret) { ret = result_len; }
    if (result != NULL) { memcpy(result, res_data, ret); }

    return (ssize_t)ret;
}
