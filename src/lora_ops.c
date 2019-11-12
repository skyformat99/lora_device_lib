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

#include "lora_ops.h"
#include "lora_sm_internal.h"
#include "lora_mac.h"
#include "lora_system.h"
#include "lora_frame.h"
#include "lora_debug.h"

/* static function prototypes *****************************************/

static void hdrDataDown2(struct lora_block *iv, uint16_t confirmCounter, uint32_t devAddr, uint32_t downCounter, uint8_t len);
static void hdrDataDown(struct lora_block *iv, uint32_t devAddr, uint32_t downCounter, uint8_t len);
static void hdrDataUp2(struct lora_block *iv, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, uint8_t len);
static void hdrDataUp(struct lora_block *iv, uint32_t devAddr, uint32_t upCounter, uint8_t len);
static void dataIV(struct lora_block *iv, uint32_t devAddr, bool upstream, uint32_t counter);
static uint32_t deriveDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter);
static uint32_t micDataUp(struct lora_mac *self, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len);
static uint32_t micDataUp2(struct lora_mac *self, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len);

/* functions **********************************************************/

void LDL_OPS_syncDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t derived;
    
    derived = deriveDownCounter(self, port, counter);
    
    if((self->ctx.version > 0U) && (port == 0U)){
        
        self->ctx.nwkDown = (uint16_t)(derived >> 16);
    }
    else{
        
        self->ctx.appDown = (uint16_t)(derived >> 16);
    }    
}

void LDL_OPS_deriveKeys(struct lora_mac *self, uint32_t joinNonce, uint32_t netID, uint16_t devNonce)
{
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block iv;
    
    /* iv.value[0] below */
    
    iv.value[1] = joinNonce;
    iv.value[2] = joinNonce >> 8;
    iv.value[3] = joinNonce >> 16;
    
    iv.value[4] = netID;
    iv.value[5] = netID >> 8;
    iv.value[6] = netID >> 16;
    
    iv.value[7] = devNonce;
    iv.value[8] = devNonce >> 8;
    
    iv.value[9] = 0U;
    iv.value[10] = 0U;
    iv.value[11] = 0U;
    iv.value[12] = 0U;
    iv.value[13] = 0U;
    iv.value[14] = 0U;
    iv.value[15] = 0U;   
    
    LDL_SM_beginUpdateSessionKey(self->sm); 
    {    
        iv.value[0] = 2U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_APPS, LORA_SM_KEY_NWK, &iv);
        
        iv.value[0] = 1U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_FNWKSINT, LORA_SM_KEY_NWK, &iv);    
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_SNWKSINT, LORA_SM_KEY_NWK, &iv);
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_NWKSENC, LORA_SM_KEY_NWK, &iv);
    }
    LDL_SM_endUpdateSessionKey(self->sm);    
}

void LDL_OPS_deriveKeys2(struct lora_mac *self, uint32_t joinNonce, const uint8_t *joinEUI, const uint8_t *devEUI, uint16_t devNonce)
{
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block iv;
    
    /* iv.value[0] below */
    
    iv.value[1] = joinNonce;
    iv.value[2] = joinNonce >> 8;
    iv.value[3] = joinNonce >> 16;
    
    iv.value[4] = joinEUI[7];
    iv.value[5] = joinEUI[6];
    iv.value[6] = joinEUI[5];
    iv.value[7] = joinEUI[4];
    iv.value[8] = joinEUI[3];
    iv.value[9] = joinEUI[2];
    iv.value[10] = joinEUI[1];
    iv.value[11] = joinEUI[0];
    
    iv.value[12] = devNonce;    
    iv.value[13] = devNonce >> 8;
    
    iv.value[14] = 0U;
    iv.value[15] = 0U;
    
    LDL_SM_beginUpdateSessionKey(self->sm); 
    {
        iv.value[0] = 1U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_FNWKSINT, LORA_SM_KEY_NWK, &iv);    
        
        iv.value[0] = 2U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_APPS, LORA_SM_KEY_NWK, &iv);
        
        iv.value[0] = 3U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_SNWKSINT, LORA_SM_KEY_NWK, &iv);
        
        iv.value[0] = 4U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_NWKSENC, LORA_SM_KEY_NWK, &iv);                
        
        iv.value[1] = devEUI[7];
        iv.value[2] = devEUI[6];
        iv.value[3] = devEUI[5];            
        iv.value[4] = devEUI[4];
        iv.value[5] = devEUI[3];
        iv.value[6] = devEUI[2];
        iv.value[7] = devEUI[1];
        iv.value[8] = devEUI[0];            
        iv.value[9] = 0U;
        iv.value[10] = 0U;
        iv.value[11] = 0U;            
        iv.value[12] = 0U;    
        iv.value[13] = 0U;            
        iv.value[14] = 0U;
        iv.value[15] = 0U;
        
        iv.value[0] = 5U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_JSENC, LORA_SM_KEY_NWK, &iv);                
        
        iv.value[0] = 6U;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_JSINT, LORA_SM_KEY_NWK, &iv);                
    }    
    LDL_SM_endUpdateSessionKey(self->sm);    
}

uint8_t LDL_OPS_prepareData(struct lora_mac *self, const struct lora_frame_data *f, uint8_t *out, uint8_t max)
{
    struct lora_block iv;
    struct lora_frame_data_offset off;
    uint32_t mic;
    uint8_t retval = 0U;
    
    dataIV(&iv, f->devAddr, true, f->counter);
    
    retval = LDL_Frame_putData(f, out, max, &off);
    
    if(self->ctx.version == 1U){
        
        if(f->optsLen > 0U){
            
            LDL_SM_ctr(self->sm, LORA_SM_KEY_NWKSENC, &iv, &out[off.opts], f->optsLen);    
        }
        else{
                        
            LDL_SM_ctr(self->sm, (f->port == 0U) ? LORA_SM_KEY_NWKSENC : LORA_SM_KEY_APPS, &iv, &out[off.data], f->dataLen);                
        }
        
        mic = micDataUp2(self, 0U, self->tx.rate, self->tx.chIndex, f->devAddr, f->counter, out, retval-sizeof(mic));//do mic
    }
    else{
        
        LDL_SM_ctr(self->sm, (f->port == 0U) ? LORA_SM_KEY_NWKSENC : LORA_SM_KEY_APPS, &iv, &out[off.data], f->dataLen);                
        
        mic = micDataUp(self, f->devAddr, f->counter, out, retval-sizeof(mic));
    }
    
    LDL_Frame_updateMIC(out, retval, mic);        
    
    return retval;
}

uint8_t LDL_OPS_prepareJoinRequest(struct lora_mac *self, const struct lora_frame_join_request *f, uint8_t *out, uint8_t max)
{
    uint32_t mic;
    uint8_t retval;
    
    retval = LDL_Frame_putJoinRequest(f, out, max);
    
    mic = LDL_SM_mic(self->sm, LORA_SM_KEY_NWK, NULL, 0U, out, retval-sizeof(mic));
    
    LDL_Frame_updateMIC(out, retval, mic);
    
    return retval;
}

bool LDL_OPS_receiveFrame(struct lora_mac *self, struct lora_frame_down *f, uint8_t *in, uint8_t len)
{
    bool retval = false;
    enum lora_sm_key key;
    uint32_t mic;
    uint32_t counter;
    enum lora_frame_type type;
    struct lora_system_identity id;
    struct lora_block hdr;
    
    switch(self->op){
    default:
        break;
        
    case LORA_OP_JOINING:
    case LORA_OP_REJOINING:
    
        if(LDL_Frame_peek(in, len, &type)){
            
            if(type == FRAME_TYPE_JOIN_ACCEPT){
                
                key = (self->op == LORA_OP_JOINING) ? LORA_SM_KEY_APP : LORA_SM_KEY_JSENC;
                
                LDL_SM_ecb(self->sm, key, &in[1U]);
        
                if(len == LDL_Frame_sizeofJoinAccept(true)){
                    
                    LDL_SM_ecb(self->sm, key, &in[LDL_Frame_sizeofJoinAccept(false)]);
                }
                
                if(LDL_Frame_decode(f, in, len)){
                    
                    if(f->optNeg){
                        
                        uint8_t joinReqType;
                        
                        switch(self->op){
                        default:
                        case LORA_OP_JOINING:
                            joinReqType = 0xffU;
                            break;                    
                        case LORA_OP_REJOINING:                        
                            joinReqType = 2U;   // todo, this woudl have been set on initiation
                            break;
                        }
                        
                        uint8_t _hdr[] = {
                            joinReqType,
                            id.joinEUI[7],
                            id.joinEUI[6],
                            id.joinEUI[5],
                            id.joinEUI[4],
                            id.joinEUI[3],
                            id.joinEUI[2],
                            id.joinEUI[1],
                            id.joinEUI[0],
                            self->devNonce,
                            self->devNonce >> 8        
                        };
                        
                        mic = LDL_SM_mic(self->sm, LORA_SM_KEY_JSINT, _hdr, sizeof(_hdr), in, len-sizeof(mic));
                    }
                    else{
                        
                        mic = LDL_SM_mic(self->sm, LORA_SM_KEY_NWK, NULL, 0U, in, len-sizeof(mic));                        
                    }
                    
                    if(f->mic == mic){
                        
                        retval = true;                    
                    }
                    else{
                        
                        LORA_DEBUG(self->app, "joinAccept MIC failed")
                    }                
                }
            }
            else{
                
                LORA_DEBUG(self->app, "unexpected frame type")
            }
        }
        break;
        
    case LORA_OP_DATA_UNCONFIRMED:
    case LORA_OP_DATA_CONFIRMED:
        
        if(LDL_Frame_decode(f, in, len)){
        
            switch(f->type){
            default:
            case FRAME_TYPE_JOIN_ACCEPT:
            
                LORA_DEBUG(self->app, "unexpected frame type")
                break;
                
            case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
            case FRAME_TYPE_DATA_CONFIRMED_DOWN:
            
                if(self->ctx.devAddr == f->devAddr){
                    
                    counter = deriveDownCounter(self, f->port, f->counter);
                    
                    if((self->ctx.version == 1U) && f->ack){
                    
                        hdrDataDown2(&hdr, (self->ctx.up-1U), f->devAddr, counter, len);
                    }                    
                    else{
                        
                        hdrDataDown(&hdr, f->devAddr, counter, len);
                    }
                    
                    mic = LDL_SM_mic(self->sm, LORA_SM_KEY_SNWKSINT, hdr.value, sizeof(hdr.value), in, len-sizeof(mic));
        
                    if(mic == f->mic){
                        
                        dataIV(&hdr, f->devAddr, false, f->counter);
                        
                        /* V1.1 seems to have snuck this breaking change in */
                        if((self->ctx.version == 1U) && (f->optsLen > 0)){
                            
                            LDL_SM_ctr(self->sm, LORA_SM_KEY_NWKSENC, &hdr, f->opts, f->optsLen);    
                        }
                        
                        LDL_SM_ctr(self->sm, (f->port == 0U) ? LORA_SM_KEY_NWKSENC : LORA_SM_KEY_APPS, &hdr, f->data, f->dataLen);
                       
                        retval = true;
                    }
                    else{
                                            
                        LORA_DEBUG(self->app, "mic failed")
                    }                           
                }
                else{
                    
                    LORA_DEBUG(self->app, "devaddr mismatch")        
                }                                         
                break;    
            }
        } 
        break;               
    }
    
    return retval;
}

/* static functions ***************************************************/

static void hdrDataUp(struct lora_block *iv, uint32_t devAddr, uint32_t upCounter, uint8_t len)
{
    hdrDataUp2(iv, 0U, 0U, 0U, devAddr, upCounter, len);    
}

static void hdrDataUp2(struct lora_block *iv, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, uint8_t len)
{    
    iv->value[0] = 0x49;
    iv->value[1] = confirmCounter;
    iv->value[2] = confirmCounter >> 8;
    iv->value[3] = rate;
    iv->value[4] = chIndex;
    iv->value[5] = 0U;          /* direction */
    iv->value[6] = devAddr;
    iv->value[7] = devAddr >> 8;
    iv->value[8] = devAddr >> 16;
    iv->value[9] = devAddr >> 24;
    iv->value[10] = upCounter;
    iv->value[11] = upCounter >> 8;
    iv->value[12] = upCounter >> 16;
    iv->value[13] = upCounter >> 24;
    iv->value[14] = 0U;
    iv->value[15] = len;    
}

static void hdrDataDown(struct lora_block *iv, uint32_t devAddr, uint32_t downCounter, uint8_t len)
{
    hdrDataUp2(iv, 0U, 0U, 0U, devAddr, downCounter, len);
    iv->value[5] = 1U;
}

static void hdrDataDown2(struct lora_block *iv, uint16_t confirmCounter, uint32_t devAddr, uint32_t downCounter, uint8_t len)
{
    hdrDataUp2(iv, confirmCounter, 0U, 0U, devAddr, downCounter, len);
    iv->value[5] = 1U;
}

static void dataIV(struct lora_block *iv, uint32_t devAddr, bool upstream, uint32_t counter)
{
    iv->value[0] = 1U;
    iv->value[1] = 0U;
    iv->value[2] = 0U;
    iv->value[3] = 0U;
    iv->value[4] = 0U;    
    iv->value[5] = upstream ? 0U : 1U;
    iv->value[6] = devAddr;
    iv->value[7] = devAddr >> 8;
    iv->value[8] = devAddr >> 16;
    iv->value[9] = devAddr >> 24;
    iv->value[10] = counter;
    iv->value[11] = counter >> 8;
    iv->value[12] = counter >> 16;
    iv->value[13] = counter >> 24;
    iv->value[14] = 0U;
    iv->value[15] = 0U;
}

static uint32_t deriveDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t mine = ((self->ctx.version > 0U) && (port == 0U)) ? (uint32_t)self->ctx.nwkDown : (uint32_t)self->ctx.appDown;
    
    mine = mine << 16;
    
    if((uint32_t)counter < mine){
        
        mine = mine + 0x10000UL + (uint32_t)counter;
    }
    else{
        
        mine = mine + (uint32_t)counter;
    }    
    
    return mine;
}

static uint32_t micDataUp(struct lora_mac *self, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len)
{    
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block hdr;
    hdrDataUp(&hdr, devAddr, upCounter, len);
    
    return LDL_SM_mic(self->sm, LORA_SM_KEY_FNWKSINT, hdr.value, sizeof(hdr.value), data, len);
}

static uint32_t micDataUp2(struct lora_mac *self, uint16_t confirmCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t micS;
    uint32_t micF;
    struct lora_block hdr;
    hdrDataUp2(&hdr, confirmCounter, rate, chIndex, devAddr, upCounter, len);
    
    micS = LDL_SM_mic(self->sm, LORA_SM_KEY_FNWKSINT, hdr.value, sizeof(hdr.value), data, len);
    micF = micDataUp(self, devAddr, upCounter, data, len);
    
    return (micS << 16) | (micF & 0xffffUL);
}
