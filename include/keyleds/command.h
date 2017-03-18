#ifndef KEYLEDS_COMMAND_H
#define KEYLEDS_COMMAND_H

struct keyleds_command {
    uint8_t     target_id;      /* which device to talk to */
    uint8_t     feature_idx;    /* index of feature to call */
    uint8_t     function;       /* index of feature's function */
    uint8_t     app_id;         /* our application identifier */
    unsigned    length;         /* number of uint8_t in data */
    uint8_t     data[];
};
typedef struct keyleds_command KeyledsCommand;

KeyledsCommand * keyleds_command_alloc(uint8_t target_id,
                                       uint8_t feature_idx, uint8_t function,
                                       uint8_t app_id, unsigned length);
KeyledsCommand * keyleds_command_alloc_empty(unsigned length);
void keyleds_command_free(/*@only@*/ /*@out@*/ KeyledsCommand *);

#endif
