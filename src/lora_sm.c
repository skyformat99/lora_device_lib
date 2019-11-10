/* Copyright (c) 2019 Cameron Harper
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */

#include "lora_sm.h"
#include "lora_ops.h"
#include "lora_aes.h"
#include "lora_cmac.h"
#include "lora_ctr.h"

#include <string.h>

//void LDL_SM_updateSessionKey(struct lora_sm *self, enum lora_sm_key key, enum lora_sm_key root, const struct lora_block *iv) __attribute__((weak));
//uint32_t LDL_SM_mic(struct lora_sm *self, enum lora_sm_key key, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen) __attribute__((weak));
//void LDL_SM_ecb(struct lora_sm *self, enum lora_sm_key key, void *b) __attribute__((weak));
//void LDL_SM_ctr(struct lora_sm *self, enum lora_sm_key key, const struct lora_block *iv, void *data, uint8_t len) __attribute__((weak));

/* functions **********************************************************/

void LDL_SM_init(struct lora_sm *self, const void *appKey, const void *nwkKey, const void *devEUI)
{
    struct lora_block iv;
    const uint8_t *eui = devEUI;
    
    (void)memcpy(self->keys[LORA_SM_KEY_APP].value, appKey, sizeof(self->keys[LORA_SM_KEY_APP].value)); 
    (void)memcpy(self->keys[LORA_SM_KEY_NWK].value, nwkKey, sizeof(self->keys[LORA_SM_KEY_NWK].value)); 
    
    iv.value[1] = eui[7];
    iv.value[2] = eui[6];
    iv.value[3] = eui[5];            
    iv.value[4] = eui[4];
    iv.value[5] = eui[3];
    iv.value[6] = eui[2];
    iv.value[7] = eui[1];
    iv.value[8] = eui[0];            
    iv.value[9] = 0U;
    iv.value[10] = 0U;
    iv.value[11] = 0U;            
    iv.value[12] = 0U;    
    iv.value[13] = 0U;            
    iv.value[14] = 0U;
    iv.value[15] = 0U;
    
    iv.value[0] = LORA_SM_KEY_JSINT;
    (void)memcpy(self->keys[LORA_SM_KEY_JSINT].value, iv.value, sizeof(self->keys[LORA_SM_KEY_JSINT].value)); 
    LDL_SM_ecb(self, LORA_SM_KEY_NWK, self->keys[LORA_SM_KEY_JSINT].value);
        
    iv.value[0] = LORA_SM_KEY_JSENC;
    (void)memcpy(self->keys[LORA_SM_KEY_JSENC].value, iv.value, sizeof(self->keys[LORA_SM_KEY_JSENC].value)); 
    LDL_SM_ecb(self, LORA_SM_KEY_NWK, self->keys[LORA_SM_KEY_JSENC].value);
}

/**! [LDL_SM_restore] */
bool LDL_SM_restore(struct lora_sm *self)
{
    return false;
}
/**! [LDL_SM_restore] */

/**! [LDL_SM_beginUpdateSessionKey] */
void LDL_SM_beginUpdateSessionKey(struct lora_sm *self)
{
}
/**! [LDL_SM_beginUpdateSessionKey] */

/**! [LDL_SM_endUpdateSessionKey] */
void LDL_SM_endUpdateSessionKey(struct lora_sm *self)
{
}
/**! [LDL_SM_endUpdateSessionKey] */

/**! [LDL_SM_updateSessionKey] */
void LDL_SM_updateSessionKey(struct lora_sm *self, enum lora_sm_key key, enum lora_sm_key root, const struct lora_block *iv)
{
    struct lora_aes_ctx ctx;
    const struct lora_key *rkey;
    
    switch(root){
    default:    
    case LORA_SM_KEY_NWK:
        rkey = &self->keys[LORA_SM_KEY_NWK];
        break;
    case LORA_SM_KEY_APP:
    case LORA_SM_KEY_FNWKSINT:
    case LORA_SM_KEY_APPS:
    case LORA_SM_KEY_SNWKSINT:
    case LORA_SM_KEY_NWKSENC:    
    case LORA_SM_KEY_JSENC:
    case LORA_SM_KEY_JSINT:
        rkey = &self->keys[root];    
        break;    
    }
    
    switch(key){
    case LORA_SM_KEY_FNWKSINT:
    case LORA_SM_KEY_APPS:
    case LORA_SM_KEY_SNWKSINT:
    case LORA_SM_KEY_NWKSENC:    
    
        LDL_AES_init(&ctx, rkey->value);
        
        (void)memcpy(self->keys[key].value, iv->value, sizeof(self->keys[key].value));
        
        LDL_AES_encrypt(&ctx, self->keys[key].value);
        break;
    
    default:
        /* not a session key*/
        break;
    }    
}
/**! [LDL_SM_updateSessionKey] */

/**! [LDL_SM_mic] */
uint32_t LDL_SM_mic(struct lora_sm *self, enum lora_sm_key key, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen)
{
    uint32_t retval;
    uint32_t mic;
    struct lora_aes_ctx aes_ctx;
    struct lora_cmac_ctx ctx;    
    
    LDL_AES_init(&aes_ctx, self->keys[key].value);
    LDL_CMAC_init(&ctx, &aes_ctx);
    LDL_CMAC_update(&ctx, hdr, hdrLen);
    LDL_CMAC_update(&ctx, data, dataLen);
    LDL_CMAC_finish(&ctx, &mic, sizeof(mic));
    
    retval = ((mic>>24)&0xffUL)       |
             ((mic<<8)&0xff0000UL)    |
             ((mic>>8)&0xff00UL)      |
             ((mic<<24)&0xff000000UL);
    
    return retval;
}
/**! [LDL_SM_mic] */

/**! [LDL_SM_ecb] */
void LDL_SM_ecb(struct lora_sm *self, enum lora_sm_key key, void *b)
{
    struct lora_aes_ctx ctx;
    
    LDL_AES_init(&ctx, self->keys[key].value);
    LDL_AES_encrypt(&ctx, b);
}
/**! [LDL_SM_ecb] */

/**! [LDL_SM_ctr] */
void LDL_SM_ctr(struct lora_sm *self, enum lora_sm_key key, const struct lora_block *iv, void *data, uint8_t len)
{
    struct lora_aes_ctx ctx;

    LDL_AES_init(&ctx, self->keys[key].value);
    LDL_CTR_encrypt(&ctx, iv->value, data, data, len);
}
/**! [LDL_SM_ctr] */
