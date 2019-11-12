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

#include "lora_mac_commands.h"
#include "lora_stream.h"
#include "lora_debug.h"

struct type_to_tag {
  
    uint8_t tag;    
    enum lora_mac_cmd_type type;    
};

static uint8_t typeToTag(enum lora_mac_cmd_type type);    
static bool tagToType(uint8_t tag, enum lora_mac_cmd_type *type);

static bool getLinkCheckAns(struct lora_stream *s, struct lora_link_check_ans *value);
static bool getLinkADRReq(struct lora_stream *s, struct lora_link_adr_req *value);
static bool getRXParamSetupReq(struct lora_stream *s, struct lora_rx_param_setup_req *value);
static bool getNewChannelReq(struct lora_stream *s, struct lora_new_channel_req *value);
static bool getDLChannelReq(struct lora_stream *s, struct lora_dl_channel_req *value);
static bool getRXTimingSetupReq(struct lora_stream *s, struct lora_rx_timing_setup_req *value);
static bool getTXParamSetupReq(struct lora_stream *s, struct lora_tx_param_setup_req *value);
static bool getDutyCycleReq(struct lora_stream *s, struct lora_duty_cycle_req *value);

static const struct type_to_tag tags[] = {
    {2U, LINK_CHECK},
    {3U, LINK_ADR},
    {4U, DUTY_CYCLE},
    {5U, RX_PARAM_SETUP},
    {6U, DEV_STATUS},
    {7U, NEW_CHANNEL},
    {8U, RX_TIMING_SETUP},
    {9U, TX_PARAM_SETUP},
    {10U, DL_CHANNEL},
    {16U, PING_SLOT_INFO},
    {17U, PING_SLOT_CHANNEL},
    {18U, BEACON_TIMING},
    {19U, BEACON_FREQ}
};

/* functions **********************************************************/

bool LDL_MAC_peekNextCommand(struct lora_stream *s, enum lora_mac_cmd_type *type)
{
    uint8_t tag;
    bool retval = false;
    
    if(LDL_Stream_peek(s, &tag)){
        
        retval = tagToType(tag, type);            
    }
    
    return retval;
}

uint8_t LDL_MAC_sizeofCommandUp(enum lora_mac_cmd_type type)
{
    uint8_t retval = 0U;
    
    switch(type){
    case LINK_CHECK:
        retval = 1U;
        break;
    case LINK_ADR:
    case DUTY_CYCLE:
    case RX_PARAM_SETUP:
    case DEV_STATUS:
    case NEW_CHANNEL:
    case RX_TIMING_SETUP:
    case TX_PARAM_SETUP:
    case DL_CHANNEL:
    case PING_SLOT_INFO:
    case PING_SLOT_CHANNEL:
    case PING_SLOT_FREQ:
    case BEACON_TIMING:
    case BEACON_FREQ:
    default:      
        break;
    }
    
    return retval;
}

void LDL_MAC_putLinkCheckReq(struct lora_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(LINK_CHECK));
}

void LDL_MAC_putLinkADRAns(struct lora_stream *s, const struct lora_link_adr_ans *value)
{
    uint8_t buf;
    
    buf = (value->powerOK ? 4U : 0U) | (value->dataRateOK ? 2U : 0U) | (value->channelMaskOK ? 1U : 0U);
    
    (void)LDL_Stream_putU8(s, typeToTag(LINK_ADR));        
    (void)LDL_Stream_putU8(s, buf);
}

void LDL_MAC_putDutyCycleAns(struct lora_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(DUTY_CYCLE));
}

void LDL_MAC_putRXParamSetupAns(struct lora_stream *s, const struct lora_rx_param_setup_ans *value)
{
    uint8_t buf;
    
    buf = (value->rx1DROffsetOK ? 4U : 0U) | (value->rx2DataRateOK ? 2U : 0U) | (value->channelOK ? 1U : 0U);
    
    (void)LDL_Stream_putU8(s, typeToTag(RX_PARAM_SETUP));
    (void)LDL_Stream_putU8(s, buf);
}

void LDL_MAC_putDevStatusAns(struct lora_stream *s, const struct lora_dev_status_ans *value)
{
    (void)LDL_Stream_putU8(s, typeToTag(DEV_STATUS));
    (void)LDL_Stream_putU8(s, value->battery);
    (void)LDL_Stream_putU8(s, value->margin);           
}


void LDL_MAC_putNewChannelAns(struct lora_stream *s, const struct lora_new_channel_ans *value)
{
    uint8_t buf;
    
    buf = (value->dataRateRangeOK ? 2U : 0U) | (value->channelFrequencyOK ? 1U : 0U);
    
    (void)LDL_Stream_putU8(s, typeToTag(NEW_CHANNEL));
    (void)LDL_Stream_putU8(s, buf);           
}


void LDL_MAC_putDLChannelAns(struct lora_stream *s, const struct lora_dl_channel_ans *value)
{
    uint8_t buf;
    
    buf = (value->uplinkFreqOK ? 2U : 0U) | (value->channelFrequencyOK ? 1U : 0U);
    
    (void)LDL_Stream_putU8(s, typeToTag(DL_CHANNEL));
    (void)LDL_Stream_putU8(s, buf);    
}

void LDL_MAC_putRXTimingSetupAns(struct lora_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(RX_TIMING_SETUP));
}

void LDL_MAC_putTXParamSetupAns(struct lora_stream *s)
{
    (void)LDL_Stream_putU8(s, typeToTag(TX_PARAM_SETUP));
}

bool LDL_MAC_getDownCommand(struct lora_stream *s, struct lora_downstream_cmd *cmd)
{
    uint8_t tag;
    bool retval = false;
    
    if(LDL_Stream_getU8(s, &tag)){
        
        if(tagToType(tag, &cmd->type)){
            
            switch(cmd->type){
            default:
            case LINK_CHECK:
            
                retval = getLinkCheckAns(s, &cmd->fields.linkCheckAns);
                break;
                
            case LINK_ADR:                    
            
                retval = getLinkADRReq(s, &cmd->fields.linkADRReq);
                break;
            
            case DUTY_CYCLE:                
            
                retval = getDutyCycleReq(s, &cmd->fields.dutyCycleReq);
                break;
            
            case RX_PARAM_SETUP:
            
                retval = getRXParamSetupReq(s, &cmd->fields.rxParamSetupReq);
                break;
            
            case DEV_STATUS:
            
                retval = true;
                break;
            
            case NEW_CHANNEL:
            
                retval = getNewChannelReq(s, &cmd->fields.newChannelReq);
                break;
                
            case DL_CHANNEL:
            
                retval = getDLChannelReq(s, &cmd->fields.dlChannelReq);
                break;
            
            case RX_TIMING_SETUP:
            
                retval = getRXTimingSetupReq(s, &cmd->fields.rxTimingSetupReq);
                break;
            
            case TX_PARAM_SETUP:
            
                retval = getTXParamSetupReq(s, &cmd->fields.txParamSetupReq);
                break;
            }
        }
    }
    
    return retval;
}

/* static functions ***************************************************/

static uint8_t typeToTag(enum lora_mac_cmd_type type)
{
    return tags[type].tag;
}

static bool tagToType(uint8_t tag, enum lora_mac_cmd_type *type)
{
    bool retval = false;
    uint8_t i;
    
    for(i=0U; i < (sizeof(tags)/sizeof(*tags)); i++){
        
        if(tags[i].tag == tag){
            
            *type = tags[i].type;
            retval = true;
            break;
        }
    }
    
    return retval;
}

static bool getLinkCheckAns(struct lora_stream *s, struct lora_link_check_ans *value)
{
    bool retval;
    
    (void)LDL_Stream_getU8(s, &value->margin);
    retval = LDL_Stream_getU8(s, &value->gwCount);    
    
    return retval;
}

static bool getLinkADRReq(struct lora_stream *s, struct lora_link_adr_req *value)
{
    uint8_t buf;
    bool retval;
    
    (void)LDL_Stream_getU8(s, &buf);
    
    value->dataRate = buf >> 4;
    value->txPower = buf & 0xfU;
    
    (void)LDL_Stream_getU16(s, &value->channelMask);
            
    retval = LDL_Stream_getU8(s, &buf);
            
    value->channelMaskControl = (buf >> 4) & 0x7U;
    value->nbTrans = buf & 0xfU;
    
    return retval;
}

static bool getDutyCycleReq(struct lora_stream *s, struct lora_duty_cycle_req *value)
{
    bool retval;
    
    retval = LDL_Stream_getU8(s, &value->maxDutyCycle);
    
    value->maxDutyCycle = value->maxDutyCycle & 0xfU;        
    
    return retval;
}

static bool getRXParamSetupReq(struct lora_stream *s, struct lora_rx_param_setup_req *value)
{
    bool retval;
    
    (void)LDL_Stream_getU8(s, &value->rx1DROffset);
    retval = LDL_Stream_getU24(s, &value->freq);        
    
    return retval;
}

static bool getNewChannelReq(struct lora_stream *s, struct lora_new_channel_req *value)
{
    bool retval;
    uint8_t buf;
    
    (void)LDL_Stream_getU8(s, &value->chIndex);        
    (void)LDL_Stream_getU24(s, &value->freq);
    retval = LDL_Stream_getU8(s, &buf);

    value->maxDR = buf >> 4;
    value->minDR = buf & 0xfU;                        
    
    return retval;
}

static bool getDLChannelReq(struct lora_stream *s, struct lora_dl_channel_req *value)
{
    bool retval;
    
    (void)LDL_Stream_getU8(s, &value->chIndex);
    retval = LDL_Stream_getU24(s, &value->freq);        
    
    return retval;
}

static bool getRXTimingSetupReq(struct lora_stream *s, struct lora_rx_timing_setup_req *value)
{
    bool retval;
    
    retval = LDL_Stream_getU8(s, &value->delay);
    value->delay &= 0xfU;        
    
    return retval;
}

static bool getTXParamSetupReq(struct lora_stream *s, struct lora_tx_param_setup_req *value)
{
    bool retval;
    uint8_t buf;
    
    retval = LDL_Stream_getU8(s, &buf);
            
    value->downlinkDwell = ((buf & 0x20U) == 0x20U); 
    value->uplinkDwell = ((buf & 0x10U) == 0x10U); 
    value->maxEIRP = buf & 0xfU; 

    return retval;
}

#if 0

static bool getPingSlotInfoReq(struct lora_stream *s, struct lora_ping_slot_info_req *value)
{
    bool retval = false;
    uint8_t buf;
    
    if(LDL_Stream_getU8(s, &buf)){
        
        value->periodicity = (buf >> 4U) & 0x7U;
        value->dataRate = buf & 0xfU;
        
        retval = true;
    }
    
    return retval
}

static bool getPingSlotChannelReq(struct lora_stream *s, struct lora_ping_slot_channel_req *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getPingSlotFreqAns(struct lora_stream *s, struct lora_ping_slot_freq_ans *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconTimingReq(struct lora_stream *s, struct lora_beacon_timing_req *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconTimingAns(struct lora_stream *s, struct lora_beacon_timing_ans *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconFreqReq(struct lora_stream *s, struct lora_beacon_freq_req *value)
{
    bool retval = false;
    
    
    return retval
}

static bool getBeaconFreqAns(struct lora_stream *s, struct lora_beacon_freq_ans *value)
{
    bool retval = false;
    
    
    return retval
}

#endif
