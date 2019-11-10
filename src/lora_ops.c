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

static void hdrDataDown2(struct lora_block *iv, uint16_t upCounter, uint32_t devAddr, uint32_t downCounter, uint8_t len);
static void hdrDataDown(struct lora_block *iv, uint32_t devAddr, uint32_t downCounter, uint8_t len);
static void hdrDataUp2(struct lora_block *iv, uint16_t downCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, uint8_t len);
static void hdrDataUp(struct lora_block *iv, uint32_t devAddr, uint32_t upCounter, uint8_t len);

/* functions **********************************************************/

uint32_t LDL_OPS_deriveDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter)
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

void LDL_OPS_syncDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t derived = LDL_OPS_deriveDownCounter(self, port, counter);
    
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
        iv.value[0] = (uint8_t)LORA_SM_KEY_APPS;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_APPS, LORA_SM_KEY_NWK, &iv);
        
        iv.value[0] = (uint8_t)LORA_SM_KEY_FNWKSINT;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_FNWKSINT, LORA_SM_KEY_NWK, &iv);    
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_SNWKSINT, LORA_SM_KEY_NWK, &iv);
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_NWKSENC, LORA_SM_KEY_NWK, &iv);
    }
    LDL_SM_endUpdateSessionKey(self->sm);    
}

void LDL_OPS_deriveKeys2(struct lora_mac *self, uint32_t joinNonce, const uint8_t *joinEUI, uint16_t devNonce)
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
        iv.value[0] = (uint8_t)LORA_SM_KEY_APPS;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_APPS, LORA_SM_KEY_NWK, &iv);
        
        iv.value[0] = (uint8_t)LORA_SM_KEY_FNWKSINT;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_FNWKSINT, LORA_SM_KEY_NWK, &iv);    
        
        iv.value[0] = (uint8_t)LORA_SM_KEY_SNWKSINT;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_SNWKSINT, LORA_SM_KEY_NWK, &iv);
        
        iv.value[0] = (uint8_t)LORA_SM_KEY_SNWKSINT;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_SNWKSINT, LORA_SM_KEY_NWK, &iv);
        
        iv.value[0] = (uint8_t)LORA_SM_KEY_NWKSENC;
        LDL_SM_updateSessionKey(self->sm, LORA_SM_KEY_NWKSENC, LORA_SM_KEY_NWK, &iv);                
    }    
    LDL_SM_endUpdateSessionKey(self->sm);    
}

bool LDL_OPS_receiveFrame(struct lora_mac *self, struct lora_frame_down *f, uint8_t *in, uint8_t len)
{
    bool retval = false;
    enum lora_sm_key key;
    uint32_t mic;
    enum lora_frame_type type;
    struct lora_system_identity id;
    
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
                        
                        mic = LDL_OPS_micJoinAccept2(self, joinReqType, id.joinEUI, self->devNonce, in, len);
                    }
                    else{
                        
                        mic = LDL_OPS_micJoinAccept(self, in, len);
                    }
                    
                    if(f->mic == mic){
                        
                        retval = true;                    
                    }
                    else{
                        
                        LORA_INFO(self->app, "joinAccept MIC failed")
                    }                
                }
            }
            else{
                
                LORA_INFO(NULL, "unexpected frame type")
            }
        }
        break;
        
    case LORA_OP_DATA_UNCONFIRMED:
    case LORA_OP_DATA_CONFIRMED:
        
        if(LDL_Frame_decode(f, in, len)){
        
            switch(f->type){
            default:
            case FRAME_TYPE_JOIN_ACCEPT:
            
                LORA_INFO(NULL, "unexpected frame type")
                break;
                
            case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
            case FRAME_TYPE_DATA_CONFIRMED_DOWN:
            
                if(self->ctx.devAddr == f->devAddr){
                    
                    uint32_t counter = LDL_OPS_deriveDownCounter(self, f->port, f->counter);
            
                    if((self->ctx.version == 1U) && f->ack){
                    
                        mic = LDL_OPS_micDataDown2(self, self->ctx.devAddr, counter, self->ctx.up, in, len-sizeof(mic));
                    }
                    
                    else{
                        
                        mic = LDL_OPS_micDataDown(self, self->ctx.devAddr, counter, in, len-sizeof(mic));
                    }
        
                    if(mic == f->mic){
                        
                        LDL_OPS_syncDownCounter(self, f->port, f->counter);    
                        
                        LDL_OPS_decryptData(self->app, f);                            
                        
                        retval = true;
                    }
                    else{
                                            
                        LORA_INFO(NULL, "mic failed")
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

void LDL_OPS_encryptData(struct lora_mac *self, void *buf, uint8_t bufLen)
{
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block iv;
    struct lora_frame_down f;
    
    if(LDL_Frame_decode(&f, buf, bufLen)){
    
        iv.value[0] = 1U;
        iv.value[1] = 0U;
        iv.value[2] = 0U;
        iv.value[3] = 0U;
        iv.value[4] = 0U;    
        iv.value[5] = 0U;
        iv.value[6] = f.devAddr;
        iv.value[7] = f.devAddr >> 8;
        iv.value[8] = f.devAddr >> 16;
        iv.value[9] = f.devAddr >> 24;
        iv.value[10] = f.counter;
        iv.value[11] = f.counter >> 8;
        iv.value[12] = f.counter >> 16;
        iv.value[13] = f.counter >> 24;
        iv.value[14] = 0U;
        iv.value[15] = 0U;
        
        /* V1.1 seems to have snuck this breaking change in */
        if((self->ctx.version == 1U) && (f.optsLen > 0)){
            
            LDL_SM_ctr(self->sm, LORA_SM_KEY_NWKSENC, &iv, f.opts, f.optsLen);    
        }
        
        LDL_SM_ctr(self->sm, (f.port == 0U) ? LORA_SM_KEY_NWKSENC : LORA_SM_KEY_APPS, &iv, f.data, f.dataLen);
    }
}

void LDL_OPS_decryptData(struct lora_mac *self, struct lora_frame_down *f)
{
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block iv;
    
    iv.value[0] = 1U;
    iv.value[1] = 0U;
    iv.value[2] = 0U;
    iv.value[3] = 0U;
    iv.value[4] = 0U;    
    iv.value[5] = 1U;
    iv.value[6] = f->devAddr;
    iv.value[7] = f->devAddr >> 8;
    iv.value[8] = f->devAddr >> 16;
    iv.value[9] = f->devAddr >> 24;
    iv.value[10] = f->counter;
    iv.value[11] = f->counter >> 8;
    iv.value[12] = f->counter >> 16;
    iv.value[13] = f->counter >> 24;
    iv.value[14] = 0U;
    iv.value[15] = 0U;
    
    /* V1.1 seems to have snuck this breaking change in */
    if((self->ctx.version == 1U) && (f->optsLen > 0)){
        
        LDL_SM_ctr(self->sm, LORA_SM_KEY_NWKSENC, &iv, f->opts, f->optsLen);    
    }
    
    LDL_SM_ctr(self->sm, (f->port == 0U) ? LORA_SM_KEY_NWKSENC : LORA_SM_KEY_APPS, &iv, f->data, f->dataLen);
}

uint32_t LDL_OPS_micDataUp(struct lora_mac *self, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len)
{    
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block hdr;
    hdrDataUp(&hdr, devAddr, upCounter, len);
    
    return LDL_SM_mic(self->sm, LORA_SM_KEY_FNWKSINT, hdr.value, sizeof(hdr.value), data, len);
}

uint32_t LDL_OPS_micDataUp2(struct lora_mac *self, uint16_t downCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t micS;
    uint32_t micF;
    struct lora_block hdr;
    hdrDataUp2(&hdr, downCounter, rate, chIndex, devAddr, upCounter, len);
    
    micS = LDL_SM_mic(self->sm, LORA_SM_KEY_FNWKSINT, hdr.value, sizeof(hdr.value), data, len);
    micF = LDL_OPS_micDataUp(self, devAddr, upCounter, data, len);
    
    return (micS << 16) | (micF & 0xffffUL);
}

uint32_t LDL_OPS_micDataDown(struct lora_mac *self, uint32_t devAddr, uint32_t downCounter, const void *data, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block hdr;
    hdrDataDown(&hdr, devAddr, downCounter, len);
    
    return LDL_SM_mic(self->sm, LORA_SM_KEY_SNWKSINT, hdr.value, sizeof(hdr.value), data, len);    
}

uint32_t LDL_OPS_micDataDown2(struct lora_mac *self, uint16_t upCounter, uint32_t devAddr, uint32_t downCounter, const void *data, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    
    struct lora_block hdr;
    hdrDataDown2(&hdr, devAddr, upCounter, downCounter, len);
    
    return LDL_SM_mic(self->sm, LORA_SM_KEY_SNWKSINT, hdr.value, sizeof(hdr.value), data, len);    
}

uint32_t LDL_OPS_micJoinAccept(struct lora_mac *self, const void *joinAccept, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    
    return LDL_SM_mic(self->sm, LORA_SM_KEY_NWK, NULL, 0U, joinAccept, len);
}

uint32_t LDL_OPS_micJoinAccept2(struct lora_mac *self, uint8_t joinReqType, const uint8_t *joinEUI, uint16_t devNonce, const void *joinAccept, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t hdr[] = {
        joinReqType,
        joinEUI[7],
        joinEUI[6],
        joinEUI[5],
        joinEUI[4],
        joinEUI[3],
        joinEUI[2],
        joinEUI[1],
        joinEUI[0],
        devNonce,
        devNonce >> 8        
    };
    
    return LDL_SM_mic(self->sm, LORA_SM_KEY_JSINT, hdr, sizeof(hdr), joinAccept, len);
}

uint32_t LDL_OPS_micJoinRequest(struct lora_mac *self, const void *joinRequest, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
        
    return LDL_SM_mic(self->sm, LORA_SM_KEY_NWK, NULL, 0U, joinRequest, len);
}

/* static functions ***************************************************/

static void hdrDataUp(struct lora_block *iv, uint32_t devAddr, uint32_t upCounter, uint8_t len)
{
    hdrDataUp2(iv, 0U, 0U, 0U, devAddr, upCounter, len);    
}

static void hdrDataUp2(struct lora_block *iv, uint16_t downCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, uint8_t len)
{    
    iv->value[0] = 0x49;
    iv->value[1] = downCounter;
    iv->value[2] = downCounter >> 8;
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

static void hdrDataDown2(struct lora_block *iv, uint16_t upCounter, uint32_t devAddr, uint32_t downCounter, uint8_t len)
{
    hdrDataUp2(iv, upCounter, 0U, 0U, devAddr, downCounter, len);
    iv->value[5] = 1U;
}
