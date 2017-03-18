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
    int         fd;                             /* device file descriptor */
    uint8_t     app_id;                         /* our application identifier */
    uint8_t     ping_seq;                       /* using for resyncing after errors */

    struct keyleds_device_reports * reports;    /* list of device-supported hid reports */
    unsigned    max_report_size;                /* maximum number of bytes in a report */

    struct keyleds_device_feature * features;   /* feature index cache */
};

bool keyleds_send(Keyleds * device, const struct keyleds_command * command);
bool keyleds_receive(Keyleds * device, struct keyleds_command * response);
bool keyleds_call_command(Keyleds * device, const struct keyleds_command * command,
                          struct keyleds_command * response);
int keyleds_call(Keyleds * device, /*@null@*/ /*@out@*/ uint8_t * result, unsigned result_len,
                 uint8_t target_id, uint16_t feature_id, uint8_t function,
                 unsigned length, ...);

#endif
