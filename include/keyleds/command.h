#ifndef KEYLEDS_COMMAND_H
#define KEYLEDS_COMMAND_H

struct keyleds_command {
    uint8_t     target_id;
    uint8_t     feature_idx;
    uint8_t     function;
    uint8_t     app_id;
    unsigned    length;
    uint8_t     data[];
};
typedef struct keyleds_command KeyledsCommand;

KeyledsCommand * keyleds_command_alloc(uint8_t target_id,
                                       uint8_t feature_idx, uint8_t function,
                                       uint8_t app_id, unsigned length);
KeyledsCommand * keyleds_command_alloc_empty(unsigned length);
void keyleds_command_free(/*@only@*/ /*@out@*/ KeyledsCommand *);

#endif
