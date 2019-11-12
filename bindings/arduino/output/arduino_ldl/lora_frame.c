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

#include "lora_frame.h"
#include "lora_debug.h"
#include "lora_stream.h"
#include <stddef.h>
#include <string.h>

/* static function prototypes *****************************************/

static bool getFrameType(uint8_t tag, enum lora_frame_type *type);

/* functions **********************************************************/

void LDL_Frame_updateMIC(void *msg, uint8_t len, uint32_t mic)
{
    struct lora_stream s;    
    
    if(len > sizeof(mic)){
    
        LDL_Stream_init(&s, msg, len);
        
        LDL_Stream_seekSet(&s, len - sizeof(mic));
        
        LDL_Stream_putU32(&s, mic);    
    }
}

uint8_t LDL_Frame_putData(const struct lora_frame_data *f, void *out, uint8_t max, struct lora_frame_data_offset *off)
{
    struct lora_stream s;    
    
    LDL_Stream_init(&s, out, max);
    
    LDL_Stream_putU8(&s, ((uint8_t)f->type) << 5);
    LDL_Stream_putU32(&s, f->devAddr);
    LDL_Stream_putU8(&s, (f->adr ? 0x80U : 0U) | (f->adrAckReq ? 0x40U : 0U) | (f->ack ? 0x20U : 0U) | (f->pending ? 0x10U : 0U) | (f->optsLen & 0xfU));
    LDL_Stream_putU16(&s, f->counter);
    
    off->opts = LDL_Stream_tell(&s);
    LDL_Stream_write(&s, f->opts, f->optsLen & 0xfU);
    
    if(f->data != NULL){

        LDL_Stream_putU8(&s, f->port);            
        off->data = LDL_Stream_tell(&s);            
        LDL_Stream_write(&s, f->data, f->dataLen);        
    }
    
    LDL_Stream_putU32(&s, f->mic);                
    
    return LDL_Stream_tell(&s);
}

uint8_t LDL_Frame_putJoinRequest(const struct lora_frame_join_request *f, void *out, uint8_t max)
{
    struct lora_stream s;    
    
    LDL_Stream_init(&s, out, max);
    
    LDL_Stream_putU8(&s, ((uint8_t)FRAME_TYPE_JOIN_REQ) << 5);
    LDL_Stream_putEUI(&s, f->joinEUI);
    LDL_Stream_putEUI(&s, f->devEUI);
    LDL_Stream_putU16(&s, f->devNonce);
    LDL_Stream_putU32(&s, f->mic);

    return LDL_Stream_tell(&s);
}

uint8_t LDL_Frame_putRejoinRequest(const struct lora_frame_rejoin_request *f, void *out, uint8_t max)
{
    struct lora_stream s;    
    
    LDL_Stream_init(&s, out, max);
        
    LDL_Stream_putU8(&s, ((uint8_t)FRAME_TYPE_REJOIN_REQ) << 5);
    LDL_Stream_putU8(&s, f->type);
    LDL_Stream_putU24(&s, f->netID);
    LDL_Stream_putEUI(&s, f->devEUI);
    LDL_Stream_putU16(&s, f->rjCount);
    LDL_Stream_putU32(&s, f->mic);    
    
    return LDL_Stream_tell(&s);
}

bool LDL_Frame_peek(const void *in, uint8_t len, enum lora_frame_type *type)
{
    bool retval = false;
    
    if(len > 0){
        
        retval = getFrameType(*(uint8_t *)in, type);        
    }
    
    return retval;
}

uint8_t LDL_Frame_sizeofJoinAccept(bool withCFList)
{
    return 17U + (withCFList ? 16U : 0U);
}

bool LDL_Frame_decode(struct lora_frame_down *f, void *in, uint8_t len)
{
    uint8_t *ptr = (uint8_t *)in;
    bool retval = false;    
    uint8_t fhdr = 0U;
    uint8_t tag;
    uint8_t dlSettings = 0U;
    struct lora_stream s;    
    
    (void)memset(f, 0, sizeof(*f));    
    
    LDL_Stream_initReadOnly(&s, in, len);

    if(LDL_Stream_getU8(&s, &tag)){
    
        if(getFrameType(tag, &f->type)){
    
            switch(f->type){
            default:  
            case FRAME_TYPE_REJOIN_REQ:            
            case FRAME_TYPE_JOIN_REQ:
            case FRAME_TYPE_DATA_UNCONFIRMED_UP:
            case FRAME_TYPE_DATA_CONFIRMED_UP:
                break;
                
            case FRAME_TYPE_JOIN_ACCEPT:
            
                LDL_Stream_getU24(&s, &f->joinNonce);
                LDL_Stream_getU24(&s, &f->netID);
                LDL_Stream_getU32(&s, &f->devAddr);
                LDL_Stream_getU8(&s, &dlSettings);
                LDL_Stream_getU8(&s, &f->rxDelay);
                
                f->optNeg =             ((dlSettings & 0x80U) != 0);
                f->rx1DataRateOffset =  (dlSettings >> 4) & 0x7U;
                f->rx2DataRate =        dlSettings & 0xfU;                    
                
                f->rxDelay = ( f->rxDelay == 0U) ? 1U : f->rxDelay;
                
                /* cflist is included */
                if(LDL_Stream_remaining(&s) > sizeof(f->mic)){
                
                    f->cfList = &ptr[LDL_Stream_tell(&s)];
                    f->cfListLen = 16U;
                    LDL_Stream_seekCur(&s, f->cfListLen);
                }
                
                LDL_Stream_getU32(&s, &f->mic);
                    
                if(!LDL_Stream_error(&s)){
                    
                    /* buffer should only be this size */
                    if(LDL_Stream_remaining(&s) == 0U){
                        
                        retval = true;                    
                    }                    
                }
                break;

            case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:            
            case FRAME_TYPE_DATA_CONFIRMED_DOWN:

                LDL_Stream_getU32(&s, &f->devAddr);
                LDL_Stream_getU8(&s, &fhdr);
                
                f->adr =        ((fhdr & 0x80U) > 0U) ? true : false;
                f->adrAckReq =  ((fhdr & 0x40U) > 0U) ? true : false;
                f->ack =        ((fhdr & 0x20U) > 0U) ? true : false;
                f->pending =    ((fhdr & 0x10U) > 0U) ? true : false;
                f->optsLen =    fhdr & 0xfU;
                
                LDL_Stream_getU16(&s, &f->counter);
                
                f->opts = (f->optsLen > 0U) ? &ptr[LDL_Stream_tell(&s)] : NULL; 
                LDL_Stream_seekCur(&s, f->optsLen);
                
                if(LDL_Stream_remaining(&s) > sizeof(f->mic)){
                    
                    f->dataPresent = true;
                    
                    LDL_Stream_getU8(&s, &f->port);
                    f->dataLen = LDL_Stream_remaining(&s) - sizeof(f->mic);                                                
                    f->data = (f->dataLen == 0U) ? NULL : &ptr[LDL_Stream_tell(&s)];                    
                    LDL_Stream_seekCur(&s, f->dataLen);
                }
                
                LDL_Stream_getU32(&s, &f->mic);
                
                if(!LDL_Stream_error(&s)){
                    
                    /* cannot have fopts when data is present and port == 0 */
                    if(!(f->dataPresent && (f->optsLen > 0) && (f->port == 0U))){
                        
                        retval = true;
                    }                                
                }
                break;
            }
        }
    }
    
    return retval;
}

uint8_t LDL_Frame_dataOverhead(void)
{
    /* DevAddr + FCtrl + FCnt + FOpts + FPort */
    return (4U + 1U + 2U) + (1U);
}

uint8_t LDL_Frame_phyOverhead(void)
{
    /* MHDR + MIC */
    return 1U + 4U;
}


/* static functions ***************************************************/

static bool getFrameType(uint8_t tag, enum lora_frame_type *type)
{
    bool retval = false;
    
    if((tag & 0x1fU) == 0U){
        
        retval = true;
        
        switch((tag >> 5)){
        case FRAME_TYPE_JOIN_REQ:
            *type = FRAME_TYPE_JOIN_REQ;
            break;
        case FRAME_TYPE_JOIN_ACCEPT:
            *type = FRAME_TYPE_JOIN_ACCEPT;
            break;
        case FRAME_TYPE_DATA_UNCONFIRMED_UP:
            *type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
            break;
        case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
            *type = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
            break;
        case FRAME_TYPE_DATA_CONFIRMED_UP:
            *type = FRAME_TYPE_DATA_CONFIRMED_UP;
            break;
        case FRAME_TYPE_DATA_CONFIRMED_DOWN:
            *type = FRAME_TYPE_DATA_CONFIRMED_DOWN;
            break;
        case FRAME_TYPE_REJOIN_REQ:
            *type = FRAME_TYPE_REJOIN_REQ;
            break;
        default:
            retval = false;
            break;
        }
    }
    
    return retval;
}
