#ifndef KEYLEDS_DEVICE_H
#define KEYLEDS_DEVICE_H

#include <stdint.h>

struct keyleds_command;

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
    char *      path;   /* path used to open */
    int         fd;
    uint8_t     app_id;
    uint8_t     ping_seq;

    struct keyleds_device_reports * reports;
    unsigned    max_report_size;

    struct keyleds_device_feature * features;

    uint8_t *   buffer;
};

bool keyleds_send(Keyleds * device, const struct keyleds_command * command);
bool keyleds_receive(Keyleds * device, struct keyleds_command * response);
bool keyleds_flush_queue(Keyleds * device);
bool keyleds_call_command(Keyleds * device, const struct keyleds_command * command,
                          struct keyleds_command * response);
int keyleds_call(Keyleds * device, /*@null@*/ /*@out@*/ uint8_t * result, unsigned result_len,
                 uint8_t target_id, uint16_t feature_id, uint8_t function,
                 unsigned length, ...);

#endif
