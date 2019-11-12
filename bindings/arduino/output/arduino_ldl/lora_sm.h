#ifndef __LORA_SM_H
#define __LORA_SM_H

/** @file */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_sm_internal.h"

#include <stdint.h>

struct lora_key {
    
    uint8_t value[16U];
};

struct lora_eui {
    
    uint8_t value[8U];
};

/** default in-memory security module state */
struct lora_sm {
    
    struct lora_key keys[8U];    
};

void LDL_SM_init(struct lora_sm *self, const void *appKey, const void *nwkKey);

void *LDL_SM_getKey(struct lora_sm *self, enum lora_sm_key key);

#ifdef __cplusplus
}
#endif

/** @} */

#endif
