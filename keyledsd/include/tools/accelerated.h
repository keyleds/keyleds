#ifndef TOOLS_ACCELERATED_H
#define TOOLS_ACCELERATED_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

void blend(uint8_t * restrict a, const uint8_t * restrict b, unsigned length);

#ifdef __cplusplus
}
#endif

#endif
