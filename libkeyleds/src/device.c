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
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "config.h"
#include "keyleds.h"
#include "keyleds/command.h"
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

    /* Open device */
    KEYLEDS_LOG(DEBUG, "Opening device %s", path);
    if ((dev->fd = open(path, O_RDWR)) < 0) {
        keyleds_set_error_errno();
        KEYLEDS_LOG(DEBUG, "Open failed: %s", keyleds_get_error_str());
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
        keyleds_set_error_string("could not parse report descriptor.");
        goto error_close_fd;
    }
    if (dev->max_report_size == 0) {
        keyleds_set_error_string("no valid report id found in report descriptor.");
        goto error_free_reports;
    }

    if (!keyleds_get_protocol(dev, KEYLEDS_TARGET_DEFAULT, &version, NULL)) {
        keyleds_set_error_string("invalid hid device");
        goto error_free_reports;
    }

    if (version < 2) {
        keyleds_set_error_string("hid++ v1 device");
        goto error_free_reports;
    }

    if (!keyleds_ping(dev, KEYLEDS_TARGET_DEFAULT)) {
        keyleds_set_error_string("synchronization with device failed");
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

int keyleds_device_fd(Keyleds * device)
{
    assert(device != NULL);
    return device->fd;
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


bool keyleds_send(Keyleds * device, const struct keyleds_command * cmd)
{
    unsigned idx;
    ssize_t size, nwritten;

    assert(device != NULL);
    assert(cmd != NULL);
    assert(cmd->function <= 0xf);
    assert(cmd->app_id <= 0xf);
    assert(cmd->length + 3 <= device->max_report_size);

    for (idx = 0; device->reports[idx].size < 3 + cmd->length; idx += 1) {
        if (device->reports[idx].id == DEVICE_REPORT_INVALID) {
            KEYLEDS_LOG(ERROR, "Command data of %u bytes exceeds max report size of %u bytes",
                        cmd->length, device->max_report_size);
            return false;
        }
    }
    size = 1 + device->reports[idx].size;

    {
        uint8_t buffer[size];
        buffer[0] = device->reports[idx].id;
        buffer[1] = cmd->target_id;
        buffer[2] = cmd->feature_idx;
        buffer[3] = cmd->function << 4 | cmd->app_id;
        memcpy(&buffer[4], cmd->data, cmd->length);
        memset(&buffer[4 + cmd->length], 0, size - 4 - cmd->length);
#ifndef NDEBUG
        if (g_keyleds_debug_level >= KEYLEDS_LOG_DEBUG) {
            char debug_buffer[3 * size + 1];
            format_buffer(buffer, size, debug_buffer);
            KEYLEDS_LOG(DEBUG, "Send [%s]", debug_buffer);
        }
#endif
        nwritten = write(device->fd, buffer, size);
    }
    return nwritten == size;
}

bool keyleds_receive(Keyleds * device, struct keyleds_command * cmd)
{
    fd_set set;
    struct timeval timeout;
    int nread;

    assert(device != NULL);
    assert(cmd != NULL);

#if KEYLEDS_CALL_TIMEOUT_US > 0
    FD_ZERO(&set);
    FD_SET(device->fd, &set);
    timeout.tv_sec = KEYLEDS_CALL_TIMEOUT_US / 1000000;
    timeout.tv_usec = KEYLEDS_CALL_TIMEOUT_US % 1000000;
    if (select(device->fd + 1, &set, NULL, NULL, &timeout) <= 0) {
        KEYLEDS_LOG(WARNING, "Device timeout while reading fd %d", device->fd);
        return false;
    }
#endif

    {
        uint8_t buffer[device->max_report_size + 1];
        if ((nread = read(device->fd, buffer, device->max_report_size + 1)) < 0) {
            return false;
        }
#ifndef NDEBUG
        if (g_keyleds_debug_level >= KEYLEDS_LOG_DEBUG) {
            char debug_buffer[3 * nread + 1];
            format_buffer(buffer, nread, debug_buffer);
            KEYLEDS_LOG(DEBUG, "Recv [%s]", debug_buffer);
        }
#endif
        cmd->target_id      = buffer[1];
        cmd->feature_idx    = buffer[2];
        cmd->function       = buffer[3] >> 4;
        cmd->app_id         = buffer[3] & 0xf;
        memcpy(cmd->data, &buffer[4],
               cmd->length < (unsigned)nread - 4 ? cmd->length : (unsigned)nread - 4);
        cmd->length         = nread - 4;
    }
    return true;
}

bool keyleds_call_command(Keyleds * device, const struct keyleds_command * command,
                          struct keyleds_command * response)
{
    unsigned response_length = response->length;
    if (!keyleds_send(device, command)) { return false; }

    do {
        response->length = response_length; /* it is modified every call */
        if (!keyleds_receive(device, response)) { return false; }
    } while (response->target_id != command->target_id ||
             response->app_id != command->app_id);

    if (response->feature_idx != command->feature_idx) {
        KEYLEDS_LOG(ERROR, "Invalid response feature: 0x%02x (expected 0x%02x)",
                      response->feature_idx, command->feature_idx);
        return false;
    }
    if (response->function != command->function) {
        KEYLEDS_LOG(ERROR, "Invalid response function: 0x%x (expected 0x%x)",
                      response->function, command->function);
        return false;
    }
    return true;
}

int keyleds_call(Keyleds * device, uint8_t * result, unsigned result_len,
    uint8_t target_id, uint16_t feature_id, uint8_t function, unsigned length, ...)
{
    va_list ap;
    unsigned idx;
    uint8_t feature_idx;
    struct keyleds_command * command, * response;
    int ret;

    assert(device != NULL);
    assert(result != NULL || result_len == 0);
    assert(function <= 0xf);

    if (feature_id == KEYLEDS_FEATURE_ROOT) {
        feature_idx = KEYLEDS_FEATURE_IDX_ROOT;
    } else {
        feature_idx = keyleds_get_feature_index(device, target_id, feature_id);
        if (feature_idx == 0) {
            KEYLEDS_LOG(ERROR, "Unknown feature %04x", feature_id);
            return -1;
        }
    }

    command = keyleds_command_alloc(
        target_id, feature_idx, function, device->app_id, length
    );
    va_start(ap, length);
    for (idx = 0; idx < length; idx += 1) {
        command->data[idx] = va_arg(ap, int);
    }
    va_end(ap);
    response = keyleds_command_alloc_empty(result_len);

    if (keyleds_call_command(device, command, response)) {
        ret = result_len < response->length ? result_len : response->length;
        if (result != NULL) {
            memcpy(result, response->data, ret);
        }
    } else {
        ret = -1;
    }

    keyleds_command_free(command);
    keyleds_command_free(response);
    return ret;
}
