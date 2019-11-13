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

#include "lora_mac.h"
#include "lora_radio.h"
#include "lora_frame.h"
#include "lora_debug.h"
#include "lora_system.h"
#include "lora_mac_commands.h"
#include "lora_stream.h"
#include "lora_sm_internal.h"
#include "lora_ops.h"
#include <string.h>

enum {
    
    ADRAckLimit = 64U,
    ADRAckDelay = 32U,
    ADRAckTimeout = 2U
};

/* static function prototypes *****************************************/

static uint8_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period);
static bool externalDataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len, const struct lora_mac_data_opts *opts);
static bool dataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len);
static uint8_t processCommands(struct lora_mac *self, const uint8_t *in, uint8_t len, bool inFopts, uint8_t *out, uint8_t max);
static bool selectChannel(const struct lora_mac *self, uint8_t rate, uint8_t prevChIndex, uint32_t limit, uint8_t *chIndex, uint32_t *freq);
static void registerTime(struct lora_mac *self, uint32_t freq, uint32_t airTime);
static bool getChannel(const struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate);
static bool isAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate, uint32_t limit);
static uint32_t transmitTime(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size, bool crc);
static void restoreDefaults(struct lora_mac *self, bool keep);
static bool setChannel(struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
static bool maskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex);
static bool unmaskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex);
static void unmaskAllChannels(uint8_t *self, enum lora_region region);
static bool channelIsMasked(const uint8_t *self, enum lora_region region, uint8_t chIndex);
static uint32_t symbolPeriod(enum lora_spreading_factor sf, enum lora_signal_bandwidth bw);
static bool msUntilAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate, uint32_t *ms);
static bool rateSettingIsValid(enum lora_region region, uint8_t rate);
static void adaptRate(struct lora_mac *self);
static uint32_t timeNow(struct lora_mac *self);
static void processBands(struct lora_mac *self);
static uint32_t nextBandEvent(const struct lora_mac *self);
static void downlinkMissingHandler(struct lora_mac *self);
static uint32_t ticksToMS(uint32_t ticks);
static uint32_t ticksToMSCoarse(uint32_t ticks);
static uint32_t msUntilNextChannel(const struct lora_mac *self, uint8_t rate);
static uint32_t rand32(void *app);
static uint32_t getRetryDuty(uint32_t seconds_since);
static uint32_t timerDelta(uint32_t timeout, uint32_t time);
#ifndef LORA_DISABLE_SESSION_UPDATE
static void pushSessionUpdate(struct lora_mac *self);
#endif
static void dummyResponseHandler(void *app, enum lora_mac_response_type type, const union lora_mac_response_arg *arg);

/* functions **********************************************************/

void LDL_MAC_init(struct lora_mac *self, enum lora_region region, const struct lora_mac_init_arg *arg)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC(LDL_System_tps() >= 1000UL)
    
    (void)memset(self, 0, sizeof(*self));
    
    self->tx.chIndex = UINT8_MAX;
    
    self->region = region;
    
    if(arg != NULL){
    
        self->app = arg->app;    
        self->handler = arg->handler ? arg->handler : dummyResponseHandler;
        self->radio = arg->radio;
        self->sm = arg->sm;        
    }
    else{
        
        self->handler = dummyResponseHandler;
        
        LORA_INFO(self->app, "arg is undefined")
    }
        
    if(self->radio != NULL){
        
        LDL_Radio_setHandler(self->radio, self, LDL_MAC_radioEvent);
    }
    else{
        
        LORA_INFO(self->app, "radio is undefined")
    }
    
    if(LDL_SM_restore(self->sm) && (arg->session != NULL)){
        
        (void)memcpy(&self->ctx, arg->session, sizeof(self->ctx));
    }
    else{
        
        restoreDefaults(self, false);
    }
     
#ifndef LORA_STARTUP_DELAY
#   define LORA_STARTUP_DELAY 0UL
#endif
    
    self->band[LORA_BAND_GLOBAL] = (uint32_t)LORA_STARTUP_DELAY;
    
    self->polled_band_ticks = LDL_System_ticks(self->app);
    self->polled_time_ticks = self->polled_band_ticks;
    
    LDL_Radio_reset(self->radio, false);

    /* leave reset line alone for 10ms */
    LDL_MAC_timerSet(self, LORA_TIMER_WAITA, (LDL_System_tps() + LDL_System_eps())/100UL);
    
    /* self->state is LORA_STATE_INIT */
}

enum lora_mac_errno LDL_MAC_errno(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->errno;
}

enum lora_mac_operation LDL_MAC_op(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->op;
}

enum lora_mac_state LDL_MAC_state(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->state;
}

bool LDL_MAC_unconfirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len, const struct lora_mac_data_opts *opts)
{    
    return externalDataCommand(self, false, port, data, len, opts);
}

bool LDL_MAC_confirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len, const struct lora_mac_data_opts *opts)
{    
    return externalDataCommand(self, true, port, data, len, opts);
}

bool LDL_MAC_otaa(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t delay;
    struct lora_system_identity identity;
    struct lora_frame_join_request f;
    
    bool retval = false;
    
    self->errno = LORA_ERRNO_NONE;
    
    if(self->state == LORA_STATE_IDLE){
        
        if(self->ctx.joined){
            
            LDL_MAC_forget(self);
        }
        
        self->trials = 0U;
        
        self->tx.rate = LDL_Region_getJoinRate(self->region, self->trials);
        self->band[LORA_BAND_RETRY] = 0U;
        self->tx.power = 0U;
        
        if(self->band[LORA_BAND_GLOBAL] == 0UL){
        
            if(selectChannel(self, self->tx.rate, self->tx.chIndex, 0UL, &self->tx.chIndex, &self->tx.freq)){
            
                LDL_System_getIdentity(self->app, &identity);
                
                (void)memcpy(f.joinEUI, identity.joinEUI, sizeof(f.joinEUI));
                (void)memcpy(f.devEUI, identity.devEUI, sizeof(f.devEUI));
                
                self->devNonce = rand32(self->app);
                
                f.devNonce = self->devNonce;

                self->bufferLen = LDL_OPS_prepareJoinRequest(self, &f, self->buffer, sizeof(self->buffer));

                delay = rand32(self->app) % (60UL*LDL_System_tps());
                
                LORA_DEBUG(self->app, "sending join in %"PRIu32" ticks", delay)
                            
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, delay);
                
                self->state = LORA_STATE_WAIT_TX;
                self->op = LORA_OP_JOINING;            
                self->service_start_time = timeNow(self) + (delay / LDL_System_tps());            
                retval = true;        
            }
            else{
                
                self->errno = LORA_ERRNO_NOCHANNEL;
            }
        }
        else{
            
            self->errno = LORA_ERRNO_NOCHANNEL;
        }
    }
    else{
        
        self->errno = LORA_ERRNO_BUSY;
    }
    
    return retval;
}

bool LDL_MAC_joined(const struct lora_mac *self)
{
    return self->ctx.joined;
}

void LDL_MAC_forget(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)    
    
    LDL_MAC_cancel(self);    
    
    if(self->ctx.joined){
    
        restoreDefaults(self, true);    
        
#ifndef LORA_DISABLE_SESSION_UPDATE        
        pushSessionUpdate(self);   
#endif
    }
}

void LDL_MAC_cancel(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    switch(self->state){
    case LORA_STATE_IDLE:
    case LORA_STATE_INIT_RESET:
    case LORA_STATE_INIT_LOCKOUT:
    case LORA_STATE_RECOVERY_RESET:    
    case LORA_STATE_RECOVERY_LOCKOUT:    
    case LORA_STATE_ENTROPY:    
        break;
    default:
        self->state = LORA_STATE_IDLE;
        LDL_Radio_sleep(self->radio);    
        break;
    }   
}

uint32_t LDL_MAC_transmitTimeUp(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size)
{
    return transmitTime(bw, sf, size, true);
}

uint32_t LDL_MAC_transmitTimeDown(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size)
{
    return transmitTime(bw, sf, size, false);
}

void LDL_MAC_process(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t error;    
    union lora_mac_response_arg arg;
    struct lora_system_identity identity;

    (void)timeNow(self);    
    
    processBands(self);
    
    switch(self->state){
    default:
    case LORA_STATE_IDLE:
        /* do nothing */
        break;    
    case LORA_STATE_INIT:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
    
            LDL_Radio_reset(self->radio, true);
        
            self->state = LORA_STATE_INIT_RESET;
            self->op = LORA_OP_RESET;
            
            /* hold reset for at least 100us */
            LDL_MAC_timerSet(self, LORA_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
            
#ifndef LORA_DISABLE_MAC_RESET_EVENT         
            self->handler(self->app, LORA_MAC_RESET, NULL); 
#endif                    
        }
        break;
        
    case LORA_STATE_INIT_RESET:
    case LORA_STATE_RECOVERY_RESET:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
        
            LDL_Radio_reset(self->radio, false);
            
            self->op = LORA_OP_RESET;
            
            switch(self->state){
            default:
            case LORA_STATE_INIT_RESET:
                self->state = LORA_STATE_INIT_LOCKOUT;                
                /* 10ms */
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/100UL) + 1UL); 
                break;            
            case LORA_STATE_RECOVERY_RESET:
                self->state = LORA_STATE_RECOVERY_LOCKOUT;
                /* 60s */
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, (LDL_System_tps() + LDL_System_eps()) * 60UL);
                break;
            }            
        }    
        break;
        
    case LORA_STATE_INIT_LOCKOUT:
    case LORA_STATE_RECOVERY_LOCKOUT:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
            
            self->op = LORA_OP_RESET;
            self->state = LORA_STATE_ENTROPY;
            
            LDL_Radio_entropyBegin(self->radio);                 
            
            /* 100us */
            LDL_MAC_timerSet(self, LORA_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
        }
        break;
            
    case LORA_STATE_ENTROPY:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
    
            self->op = LORA_OP_RESET;
            
            arg.startup.entropy = LDL_Radio_entropyEnd(self->radio);                    
                
            self->state = LORA_STATE_IDLE;
            self->op = LORA_OP_NONE;
            
#ifndef LORA_DISABLE_MAC_STARTUP_EVENT            
            self->handler(self->app, LORA_MAC_STARTUP, &arg);        
#endif                                
        }
        break;
    
    case LORA_STATE_WAIT_TX:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
            
            struct lora_radio_tx_setting radio_setting;
            uint32_t tx_time;
            uint8_t mtu;
            
            LDL_Region_convertRate(self->region, self->tx.rate, &radio_setting.sf, &radio_setting.bw, &mtu);
            
            radio_setting.dbm = LDL_Region_getTXPower(self->region, self->tx.power);
            
            radio_setting.freq = self->tx.freq;
            
            tx_time = transmitTime(radio_setting.bw, radio_setting.sf, self->bufferLen, true);
            
            LDL_MAC_inputClear(self);  
            LDL_MAC_inputArm(self, LORA_INPUT_TX_COMPLETE);  
            
            LDL_Radio_transmit(self->radio, &radio_setting, self->buffer, self->bufferLen);

            registerTime(self, self->tx.freq, tx_time);
            
            self->state = LORA_STATE_TX;
            
            LORA_PEDANTIC((tx_time & 0x80000000UL) != 0x80000000UL)
            
            /* reset the radio if the tx complete interrupt doesn't appear after double the expected time */      
            LDL_MAC_timerSet(self, LORA_TIMER_WAITA, tx_time << 1UL);    
            
#ifndef LORA_DISABLE_TX_BEGIN_EVENT            
            arg.tx_begin.freq = self->tx.freq;
            arg.tx_begin.power = self->tx.power;
            arg.tx_begin.sf = radio_setting.sf;
            arg.tx_begin.bw = radio_setting.bw;
            arg.tx_begin.size = self->bufferLen;
            
            self->handler(self->app, LORA_MAC_TX_BEGIN, &arg);
#endif            
        }
        break;
        
    case LORA_STATE_TX:
    
        if(LDL_MAC_inputCheck(self, LORA_INPUT_TX_COMPLETE, &error)){
        
            LDL_MAC_inputClear(self);
        
            uint32_t waitSeconds;
            uint32_t waitTicks;    
            uint32_t advance;
            uint32_t advanceA;
            uint32_t advanceB;
            enum lora_spreading_factor sf;
            enum lora_signal_bandwidth bw;
            uint8_t rate;
            uint8_t extra_symbols;
            uint32_t xtal_error;
            uint8_t mtu;
            
            /* the wait interval is always measured in whole seconds */
            waitSeconds = (self->op == LORA_OP_JOINING) ? LDL_Region_getJA1Delay(self->region) : self->ctx.rx1Delay;    
            
            /* add xtal error to ensure the fastest clock will not open before the earliest start time */
            waitTicks = (waitSeconds * LDL_System_tps()) + (waitSeconds * LDL_System_eps());
            
            /* sources of timing advance common to both slots are:
             * 
             * - LDL_System_advance(): interrupt response time + radio RX ramp up
             * - error: ticks since tx complete event
             * 
             * */
            advance = LDL_System_advance() + error;    
            
            /* RX1 */
            {
                LDL_Region_getRX1DataRate(self->region, self->tx.rate, self->ctx.rx1DROffset, &rate);
                LDL_Region_convertRate(self->region, rate, &sf, &bw, &mtu);
                
                xtal_error = (waitSeconds * LDL_System_eps() * 2U);
                
                extra_symbols = extraSymbols(xtal_error, symbolPeriod(sf, bw));
                
                self->rx1_margin = (((3U + extra_symbols) * symbolPeriod(sf, bw)));
                self->rx1_symbols = 8U + extra_symbols;
            
                /* advance timer by time required for extra symbols */
                advanceA = advance + (extra_symbols * symbolPeriod(sf, bw));
            }
            
            /* RX2 */
            {
                LDL_Region_convertRate(self->region, self->ctx.rx2Rate, &sf, &bw, &mtu);
                
                xtal_error = ((waitSeconds + 1UL) * LDL_System_eps() * 2UL);
                
                extra_symbols = extraSymbols(xtal_error, symbolPeriod(sf, bw));
                
                self->rx2_margin = (((3U + extra_symbols) * symbolPeriod(sf, bw)));
                self->rx2_symbols = 8U + extra_symbols;
                
                /* advance timer by time required for extra symbols */
                advanceB = advance + (extra_symbols * symbolPeriod(sf, bw));
            }
                
            if(advanceB <= (waitTicks + (LDL_System_tps() + LDL_System_eps()))){
                
                LDL_MAC_timerSet(self, LORA_TIMER_WAITB, waitTicks + (LDL_System_tps() + LDL_System_eps()) - advanceB);
                
                if(advanceA <= waitTicks){
                
                    LDL_MAC_timerSet(self, LORA_TIMER_WAITA, waitTicks - advanceA);
                    self->state = LORA_STATE_WAIT_RX1;
                }
                else{
                    
                    LDL_MAC_timerClear(self, LORA_TIMER_WAITA);
                    self->state = LORA_STATE_WAIT_RX2;
                }
            }
            else{
                
                self->state = LORA_STATE_WAIT_RX2;
                LDL_MAC_timerClear(self, LORA_TIMER_WAITA);
                LDL_MAC_timerSet(self, LORA_TIMER_WAITB, 0U);
                self->state = LORA_STATE_WAIT_RX2;                
            }    
            
            LDL_Radio_clearInterrupt(self->radio);
            
#ifndef LORA_DISABLE_TX_COMPLETE_EVENT            
            self->handler(self->app, LORA_MAC_TX_COMPLETE, NULL);                        
#endif            
        }
        else{
            
            if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
                
#ifndef LORA_DISABLE_CHIP_ERROR_EVENT                
                self->handler(self->app, LORA_MAC_CHIP_ERROR, NULL);                
#endif                
                LDL_MAC_inputClear(self);
                
                self->state = LORA_STATE_RECOVERY_RESET;
                self->op = LORA_OP_RESET;
                
                LDL_Radio_reset(self->radio, true);
                
                /* hold reset for at least 100us */
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1UL);
            }
        }
        break;
        
    case LORA_STATE_WAIT_RX1:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
    
            struct lora_radio_rx_setting radio_setting;
            uint32_t freq;    
            uint8_t rate;
            
            LDL_Region_getRX1DataRate(self->region, self->tx.rate, self->ctx.rx1DROffset, &rate);
            LDL_Region_getRX1Freq(self->region, self->tx.freq, self->tx.chIndex, &freq);    
                                    
            LDL_Region_convertRate(self->region, rate, &radio_setting.sf, &radio_setting.bw, &radio_setting.max);
            
            radio_setting.max += LDL_Frame_phyOverhead();
            
            self->state = LORA_STATE_RX1;
            
            if(error <= self->rx1_margin){
                
                radio_setting.freq = freq;
                radio_setting.timeout = self->rx1_symbols;
                
                LDL_MAC_inputClear(self);
                LDL_MAC_inputArm(self, LORA_INPUT_RX_READY);
                LDL_MAC_inputArm(self, LORA_INPUT_RX_TIMEOUT);
                
                LDL_Radio_receive(self->radio, &radio_setting);
                
                /* use waitA as a guard */
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, (LDL_System_tps()) << 4U);                                
            }
            else{
                
                self->state = LORA_STATE_WAIT_RX2;
            }
                
#ifndef LORA_DISABLE_SLOT_EVENT                           
            arg.rx_slot.margin = self->rx1_margin;                
            arg.rx_slot.timeout = self->rx1_symbols;                
            arg.rx_slot.error = error;
            arg.rx_slot.freq = freq;
            arg.rx_slot.bw = radio_setting.bw;
            arg.rx_slot.sf = radio_setting.sf;
                            
            self->handler(self->app, LORA_MAC_RX1_SLOT, &arg);                    
#endif                
        }
        break;
            
    case LORA_STATE_WAIT_RX2:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITB, &error)){
            
            struct lora_radio_rx_setting radio_setting;
            
            LDL_Region_convertRate(self->region, self->ctx.rx2DataRate, &radio_setting.sf, &radio_setting.bw, &radio_setting.max);
            
            radio_setting.max += LDL_Frame_phyOverhead();
            
            self->state = LORA_STATE_RX2;
            
            if(error <= self->rx2_margin){
                
                radio_setting.freq = self->ctx.rx2Freq;
                radio_setting.timeout = self->rx2_symbols;
                
                LDL_MAC_inputClear(self);
                LDL_MAC_inputArm(self, LORA_INPUT_RX_READY);
                LDL_MAC_inputArm(self, LORA_INPUT_RX_TIMEOUT);
                
                LDL_Radio_receive(self->radio, &radio_setting);
                
                /* use waitA as a guard */
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, (LDL_System_tps()) << 4U);
            }
            else{
                
                adaptRate(self);
                
                self->state = LORA_STATE_IDLE;
            }
                
#ifndef LORA_DISABLE_SLOT_EVENT                                    
            arg.rx_slot.margin = self->rx2_margin;                
            arg.rx_slot.timeout = self->rx2_symbols;                
            arg.rx_slot.error = error;
            arg.rx_slot.freq = self->ctx.rx2Freq;
            arg.rx_slot.bw = radio_setting.bw;
            arg.rx_slot.sf = radio_setting.sf;           
                 
            self->handler(self->app, LORA_MAC_RX2_SLOT, &arg);                    
#endif                
        }
        break;
        
    case LORA_STATE_RX1:    
    case LORA_STATE_RX2:

        if(LDL_MAC_inputCheck(self, LORA_INPUT_RX_READY, &error)){
        
            LDL_MAC_inputClear(self);
    
            struct lora_frame_down frame;
#ifdef LORA_ENABLE_STATIC_RX_BUFFER         
            uint8_t *buffer = self->rx_buffer;
#else   
            uint8_t buffer[LORA_MAX_PACKET];
#endif            
            uint8_t len;
            
            struct lora_radio_packet_metadata meta;            
            uint8_t cmd_len = 0U;
            
            LDL_MAC_timerClear(self, LORA_TIMER_WAITA);
            LDL_MAC_timerClear(self, LORA_TIMER_WAITB);
            
            len = LDL_Radio_collect(self->radio, &meta, buffer, LORA_MAX_PACKET);        
            
            LDL_Radio_clearInterrupt(self->radio);
            
            /* notify of a downstream message */
#ifndef LORA_DISABLE_DOWNSTREAM_EVENT            
            arg.downstream.rssi = meta.rssi;
            arg.downstream.snr = meta.snr;
            arg.downstream.size = len;
            
            self->handler(self->app, LORA_MAC_DOWNSTREAM, &arg);      
#endif  
            self->margin = meta.snr;
                      
            LDL_System_getIdentity(self->app, &identity);
            
            if(LDL_OPS_receiveFrame(self, &frame, buffer, len)){
                
                self->last_valid_downlink = timeNow(self);
                
                switch(frame.type){
                default:
                case FRAME_TYPE_JOIN_ACCEPT:

                    restoreDefaults(self, true);
                    
                    self->ctx.joined = true;
                    
                    if(self->ctx.adr){
                        
                        /* keep the joining rate */
                        self->ctx.rate = self->tx.rate;
                    }                
                    
                    self->ctx.rx1DROffset = frame.rx1DataRateOffset;
                    self->ctx.rx2DataRate = frame.rx2DataRate;
                    self->ctx.rx1Delay = frame.rxDelay;
                    
                    if(frame.cfList != NULL){
                        
                        LDL_Region_processCFList(self->region, self, frame.cfList, frame.cfListLen);                        
                    }
                    
                    if(frame.optNeg){
                    
                        LDL_OPS_deriveKeys2(
                            self, 
                            frame.joinNonce, 
                            identity.joinEUI, 
                            identity.devEUI,
                            self->devNonce
                        );
                        
                        self->ctx.version = 1U;
                    }
                    else{
                        
                        LDL_OPS_deriveKeys(
                            self, 
                            frame.joinNonce, 
                            frame.netID, 
                            self->devNonce
                        );
                        
                        self->ctx.version = 0U;
                    }
                    
                    self->ctx.devAddr = frame.devAddr;    
                    self->ctx.netID = frame.netID;
                    
#ifndef LORA_DISABLE_JOIN_COMPLETE_EVENT                    
                    self->handler(self->app, LORA_MAC_JOIN_COMPLETE, NULL);                    
#endif                                      
                    self->state = LORA_STATE_IDLE;           
                    self->op = LORA_OP_NONE;                                
                    break;
                
                case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
                case FRAME_TYPE_DATA_CONFIRMED_DOWN:
                                
                    
                    LDL_OPS_syncDownCounter(self, frame.port, frame.counter);
                        
                    self->adrAckCounter = 0U;
                    self->rxParamSetupAns_pending = false;    
                    self->dlChannelAns_pending = false;
                    self->rxtimingSetupAns_pending = false;
                    self->adrAckReq = false;
                                        
                    if(frame.data != NULL){
            
                        if(frame.port > 0U){

                            cmd_len = processCommands(self, frame.opts, frame.optsLen, true, buffer, LORA_MAX_PACKET);      
#ifndef LORA_DISABLE_RX_EVENT                                            
                            arg.rx.counter = frame.counter;
                            arg.rx.port = frame.port;
                            arg.rx.data = frame.data;
                            arg.rx.size = frame.dataLen;                                            
                            
                            self->handler(self->app, LORA_MAC_RX, &arg);                                        
#endif                                                                                        
                        }
                        else{
                        
                            cmd_len = processCommands(self, frame.data, frame.dataLen, false, buffer, LORA_MAX_PACKET);                                              
                        }
                    }
                    else{
                        
                        cmd_len = processCommands(self, frame.opts, frame.optsLen, true, buffer, LORA_MAX_PACKET);      
                    }
                    
#ifndef LORA_DISABLE_DATA_COMPLETE_EVENT
                    self->handler(self->app, LORA_MAC_DATA_COMPLETE, NULL);
#endif                
                    /* respond to MAC command */
                    if(cmd_len > 0U){

                        LORA_DEBUG(self->app, "sending mac response")
                        
                        self->tx.rate = self->ctx.rate;
                        self->tx.power = self->ctx.power;
                    
                        uint32_t ms_until_next = msUntilNextChannel(self, self->tx.rate);
                    
                        /* MAC command may have masked everything... */
                        if(ms_until_next != UINT32_MAX){
                    
                            struct lora_frame_data f;
                            
                            (void)memset(&f, 0, sizeof(f));
                        
                            f.devAddr = self->ctx.devAddr;
                            f.counter = self->ctx.up;
                            f.adr = self->ctx.adr;
                            f.adrAckReq = self->adrAckReq;
                            f.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
                            
                            if(cmd_len <= 15U){
                                
                                f.opts = buffer;
                                f.optsLen = cmd_len;
                            }
                            else{
                                
                                f.data = buffer;
                                f.dataLen = cmd_len;
                            }
                            
                            self->bufferLen = LDL_OPS_prepareData(self, &f, self->buffer, sizeof(self->buffer));
                            
                            self->ctx.up++;
                            
                            self->op = LORA_OP_DATA_UNCONFIRMED;
                            self->state = LORA_STATE_WAIT_RETRY;
                            self->band[LORA_BAND_RETRY] = ms_until_next;                        
                        }
                        else{
                            
                            LORA_DEBUG(self->app, "cannot send, all channels are masked!")
                            
                            self->state = LORA_STATE_IDLE;           
                            self->op = LORA_OP_NONE;
                        }
                    }
                    else{
                        
                        self->state = LORA_STATE_IDLE;           
                        self->op = LORA_OP_NONE;
                    }
                    
                    
                    break;
                }
                
#ifndef LORA_DISABLE_SESSION_UPDATE
                pushSessionUpdate(self);    
#endif                
            }
            else{
                
                downlinkMissingHandler(self);
            }
        }
        else if(LDL_MAC_inputCheck(self, LORA_INPUT_RX_TIMEOUT, &error)){
            
            LDL_MAC_inputClear(self);
            
            LDL_Radio_clearInterrupt(self->radio);
            
            if(self->state == LORA_STATE_RX2){
            
                LDL_MAC_timerClear(self, LORA_TIMER_WAITB);
                
                uint8_t mtu;
                enum lora_spreading_factor sf;
                enum lora_signal_bandwidth bw;
                
                LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &mtu);                        
                
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, transmitTime(bw, sf, mtu, false));
                
                self->state = LORA_STATE_RX2_LOCKOUT;
            }
            else{
                
                LDL_MAC_timerClear(self, LORA_TIMER_WAITA);
                
                self->state = LORA_STATE_WAIT_RX2;
            }   
        }
        else{
            
            /* this is a hardware failure condition */
            if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error) || LDL_MAC_timerCheck(self, LORA_TIMER_WAITB, &error)){
                
#ifndef LORA_DISABLE_CHIP_ERROR_EVENT                
                self->handler(self->app, LORA_MAC_CHIP_ERROR, NULL); 
#endif                
                LDL_MAC_inputClear(self);
                LDL_MAC_timerClear(self, LORA_TIMER_WAITA);
                LDL_MAC_timerClear(self, LORA_TIMER_WAITB);
                
                self->state = LORA_STATE_RECOVERY_RESET;
                self->op = LORA_OP_RESET;
                
                LDL_Radio_reset(self->radio, true);
                
                /* hold reset for at least 100us */
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, ((LDL_System_tps() + LDL_System_eps())/10000UL) + 1U);                     
            }
        }
        break;
    
    case LORA_STATE_RX2_LOCKOUT:
    
        if(LDL_MAC_timerCheck(self, LORA_TIMER_WAITA, &error)){
            
            downlinkMissingHandler(self);  
            
#ifndef LORA_DISABLE_SESSION_UPDATE              
            pushSessionUpdate(self);
#endif            
        }
        break;
    
    case LORA_STATE_WAIT_RETRY:
        
        if(self->band[LORA_BAND_RETRY] == 0U){
            
            if(msUntilNextChannel(self, self->tx.rate) != UINT32_MAX){
            
                if(self->band[LORA_BAND_GLOBAL] == 0UL){
            
                    if(selectChannel(self, self->tx.rate, self->tx.chIndex, 0UL, &self->tx.chIndex, &self->tx.freq)){
                                
                        uint32_t delay = rand32(self->app) % (LDL_System_tps()*30UL);
                                
                        LORA_DEBUG(self->app, "dither retry by %"PRIu32" ticks", delay)
                                
                        LDL_MAC_timerSet(self, LORA_TIMER_WAITA, delay);
                        self->state = LORA_STATE_WAIT_TX;
                    }            
                }
            }
            else{
                
                LORA_DEBUG(self->app, "no channels for retry")
                
                self->op = LORA_OP_NONE;
                self->state = LORA_STATE_IDLE;
            }
        }
        break;    
    }
    
    {
        uint32_t next = nextBandEvent(self);     
            
        if(next < ticksToMSCoarse(60UL*LDL_System_tps())){
            
            LDL_MAC_timerSet(self, LORA_TIMER_BAND, LDL_System_tps() / 1000UL * (next+1U));                    
        }
        else{
        
            LDL_MAC_timerSet(self, LORA_TIMER_BAND, 60UL*LDL_System_tps());                    
        }
    }       
}

uint32_t LDL_MAC_ticksUntilNextEvent(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    uint32_t retval = 0UL;
    
    if(!LDL_MAC_inputPending(self)){
     
        retval = LDL_MAC_timerTicksUntilNext(self);    
    }
    
    return retval;
}

bool LDL_MAC_setRate(struct lora_mac *self, uint8_t rate)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;    
    self->errno = LORA_ERRNO_NONE;
    
    if(rateSettingIsValid(self->region, rate)){
        
        self->ctx.rate = rate;
        
#ifndef LORA_DISABLE_SESSION_UPDATE        
        pushSessionUpdate(self);
#endif        
        retval = true;        
    }
    else{
        
        self->errno = LORA_ERRNO_RATE;
    }
    
    return retval;
}

uint8_t LDL_MAC_getRate(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.rate;
}

bool LDL_MAC_setPower(struct lora_mac *self, uint8_t power)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    self->errno = LORA_ERRNO_NONE;
        
    if(LDL_Region_validateTXPower(self->region, power)){
        
        self->ctx.power = power;
#ifndef LORA_DISABLE_SESSION_UPDATE        
        pushSessionUpdate(self);        
#endif        
        retval = true;
    }
    else{
     
        self->errno = LORA_ERRNO_POWER;
    }        
    
    return retval;
}

uint8_t LDL_MAC_getPower(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.power;
}

void LDL_MAC_enableADR(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    self->ctx.adr = true;
    
#ifndef LORA_DISABLE_SESSION_UPDATE    
    pushSessionUpdate(self);       
#endif    
}

bool LDL_MAC_adr(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.adr;
}

void LDL_MAC_disableADR(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    self->ctx.adr = false;
    
#ifndef LORA_DISABLE_SESSION_UPDATE    
    pushSessionUpdate(self);       
#endif     
}

bool LDL_MAC_ready(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    
    if(self->state == LORA_STATE_IDLE){
        
        retval = (msUntilNextChannel(self, self->ctx.rate) == 0UL);
    }
    
    return retval;
}

uint32_t LDL_MAC_bwToNumber(enum lora_signal_bandwidth bw)
{
    uint32_t retval;
    
    switch(bw){
    default:
    case BW_125:
        retval = 125000UL;
        break;        
    case BW_250:
        retval = 250000UL;
        break;            
    case BW_500:
        retval = 500000UL;
        break;                
    }
    
    return retval;
}

void LDL_MAC_radioEvent(struct lora_mac *self, enum lora_radio_event event)
{
    LORA_PEDANTIC(self != NULL)
    
    switch(event){
    case LORA_RADIO_EVENT_TX_COMPLETE:
        LDL_MAC_inputSignal(self, LORA_INPUT_TX_COMPLETE);
        break;
    case LORA_RADIO_EVENT_RX_READY:
        LDL_MAC_inputSignal(self, LORA_INPUT_RX_READY);
        break;
    case LORA_RADIO_EVENT_RX_TIMEOUT:
        LDL_MAC_inputSignal(self, LORA_INPUT_RX_TIMEOUT);        
        break;
    case LORA_RADIO_EVENT_NONE:
    default:
        break;
    }     
}

uint8_t LDL_MAC_mtu(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    uint8_t max = 0U;
    uint8_t overhead = LDL_Frame_dataOverhead();    
    
    /* which rate?? */
    LDL_Region_convertRate(self->region, self->ctx.rate, &sf, &bw, &max);
    
    LORA_PEDANTIC(LDL_Frame_dataOverhead() < max)
    
    if(self->dlChannelAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(DL_CHANNEL);        
    }
    
    if(self->rxtimingSetupAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(RX_TIMING_SETUP);                       
    }
    
    if(self->rxParamSetupAns_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(RX_PARAM_SETUP);
    }
    
    if(self->linkCheckReq_pending){
        
        overhead += LDL_MAC_sizeofCommandUp(LINK_CHECK);
    }
    
    return (overhead > max) ? 0U : (max - overhead);    
}

uint32_t LDL_MAC_timeSinceValidDownlink(struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return (self->last_valid_downlink == 0) ? UINT32_MAX : (timeNow(self) - self->last_valid_downlink);
}

void LDL_MAC_setMaxDCycle(struct lora_mac *self, uint8_t maxDCycle)
{
    LORA_PEDANTIC(self != NULL)
    
    self->ctx.maxDutyCycle = maxDCycle & 0xfU;
    
#ifndef LORA_DISABLE_SESSION_UPDATE    
    pushSessionUpdate(self);        
#endif    
}

uint8_t LDL_MAC_getMaxDCycle(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.maxDutyCycle;
}

void LDL_MAC_setNbTrans(struct lora_mac *self, uint8_t nbTrans)
{
    LORA_PEDANTIC(self != NULL)
    
    self->ctx.nbTrans = nbTrans & 0xfU;
    
#ifndef LORA_DISABLE_SESSION_UPDATE
    pushSessionUpdate(self);     
#endif        
}

uint8_t LDL_MAC_getNbTrans(const struct lora_mac *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->ctx.nbTrans;        
}

bool LDL_MAC_addChannel(struct lora_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    LORA_PEDANTIC(self != NULL)
    
    return setChannel(self->ctx.chConfig, self->region, chIndex, freq, minRate, maxRate);
}

bool LDL_MAC_maskChannel(struct lora_mac *self, uint8_t chIndex)
{
    LORA_PEDANTIC(self != NULL)
    
    return maskChannel(self->ctx.chMask, self->region, chIndex);
}

bool LDL_MAC_unmaskChannel(struct lora_mac *self, uint8_t chIndex)
{
    LORA_PEDANTIC(self != NULL)
    
    return unmaskChannel(self->ctx.chMask, self->region, chIndex);
}

void LDL_MAC_timerSet(struct lora_mac *self, enum lora_timer_inst timer, uint32_t timeout)
{
    LORA_SYSTEM_ENTER_CRITICAL(self->app)
   
    self->timers[timer].time = LDL_System_ticks(self->app) + (timeout & INT32_MAX);
    self->timers[timer].armed = true;
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)
}

void LDL_MAC_timerIncrement(struct lora_mac *self, enum lora_timer_inst timer, uint32_t timeout)
{
    LORA_SYSTEM_ENTER_CRITICAL(self->app)
   
    self->timers[timer].time += (timeout & INT32_MAX);
    self->timers[timer].armed = true;
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)
}

bool LDL_MAC_timerCheck(struct lora_mac *self, enum lora_timer_inst timer, uint32_t *error)
{
    bool retval = false;
    uint32_t time;
    
    LORA_SYSTEM_ENTER_CRITICAL(self->app)
        
    if(self->timers[timer].armed){
        
        time = LDL_System_ticks(self->app);
        
        if(timerDelta(self->timers[timer].time, time) < INT32_MAX){
    
            self->timers[timer].armed = false;            
            *error = timerDelta(self->timers[timer].time, time);
            retval = true;
        }
    }    
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)
    
    return retval;
}

void LDL_MAC_timerClear(struct lora_mac *self, enum lora_timer_inst timer)
{
    self->timers[timer].armed = false;
}

uint32_t LDL_MAC_timerTicksUntilNext(const struct lora_mac *self)
{
    size_t i;
    uint32_t retval = UINT32_MAX;
    uint32_t time;
    
    time = LDL_System_ticks(self->app);

    for(i=0U; i < (sizeof(self->timers)/sizeof(*self->timers)); i++){

        LORA_SYSTEM_ENTER_CRITICAL(self->app)

        if(self->timers[i].armed){
            
            if(timerDelta(self->timers[i].time, time) <= INT32_MAX){
                
                retval = 0U;
            }
            else{
                
                if(timerDelta(time, self->timers[i].time) < retval){
                    
                    retval = timerDelta(time, self->timers[i].time);
                }
            }
        }
        
        LORA_SYSTEM_LEAVE_CRITICAL(self->app)
        
        if(retval == 0U){
            
            break;
        }
    }
    
    return retval;
}

uint32_t LDL_MAC_timerTicksUntil(const struct lora_mac *self, enum lora_timer_inst timer, uint32_t *error)
{
    uint32_t retval = UINT32_MAX;
    uint32_t time;
    
    LORA_SYSTEM_ENTER_CRITICAL(self->app)
    
    if(self->timers[timer].armed){
        
        time = LDL_System_ticks(self->app);
    
        *error = timerDelta(self->timers[timer].time, time);
        
        if(*error <= INT32_MAX){
            
            retval = 0U;
        }
        else{
            
            retval = timerDelta(time, self->timers[timer].time);
        }
    }
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)
    
    return retval;
}

void LDL_MAC_inputSignal(struct lora_mac *self, enum lora_input_type type)
{
    LORA_SYSTEM_ENTER_CRITICAL(self->app)
    
    if(self->inputs.state == 0U){
    
        if((self->inputs.armed & (1U << type)) > 0U){
    
            self->inputs.time = LDL_System_ticks(self->app);
            self->inputs.state = (1U << type);
        }
    }
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)
}

void LDL_MAC_inputArm(struct lora_mac *self, enum lora_input_type type)
{
    LORA_SYSTEM_ENTER_CRITICAL(self->app) 
    
    self->inputs.armed |= (1U << type);
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)     
}

bool LDL_MAC_inputCheck(const struct lora_mac *self, enum lora_input_type type, uint32_t *error)
{
    bool retval = false;
    
    LORA_SYSTEM_ENTER_CRITICAL(self->app)     
    
    if((self->inputs.state & (1U << type)) > 0U){
        
        *error = timerDelta(self->inputs.time, LDL_System_ticks(self->app));
        retval = true;
    }
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)    
    
    return retval;
}

void LDL_MAC_inputClear(struct lora_mac *self)
{
    LORA_SYSTEM_ENTER_CRITICAL(self->app)     
    
    self->inputs.state = 0U;
    self->inputs.armed = 0U;
    
    LORA_SYSTEM_LEAVE_CRITICAL(self->app)   
}

bool LDL_MAC_inputPending(const struct lora_mac *self)
{
    return (self->inputs.state != 0U);
}

bool LDL_MAC_priority(const struct lora_mac *self, uint8_t interval)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval;
    
    uint32_t error;
    
    LDL_MAC_timerTicksUntil(self, LORA_TIMER_WAITA, &error);
    
    switch(self->state){
    default:
        retval = false;
        break;
    case LORA_STATE_TX:    
    case LORA_STATE_WAIT_RX1:
    case LORA_STATE_RX1:
    case LORA_STATE_WAIT_RX2:
    case LORA_STATE_RX2:
        retval = true;
        break;
    }
    
    return retval;
}

/* static functions ***************************************************/

static bool externalDataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len, const struct lora_mac_data_opts *opts)
{
    bool retval = false;
    uint8_t maxPayload;
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    
    self->errno = LORA_ERRNO_NONE;

    if(self->state == LORA_STATE_IDLE){
        
        if(self->ctx.joined){
        
            if((port > 0U) && (port <= 223U)){    
                
                if(self->band[LORA_BAND_GLOBAL] == 0UL){
                
                    if(selectChannel(self, self->ctx.rate, self->tx.chIndex, 0UL, &self->tx.chIndex, &self->tx.freq)){
                     
                        LDL_Region_convertRate(self->region, self->ctx.rate, &sf, &bw, &maxPayload);
                                            
                        if(opts == NULL){
                            
                            (void)memset(&self->opts, 0, sizeof(self->opts));
                        }
                        else{
                            
                            (void)memcpy(&self->opts, opts, sizeof(self->opts));
                        }

                        self->opts.nbTrans = self->opts.nbTrans & 0xfU;
                        
                        if(len <= (maxPayload - LDL_Frame_dataOverhead() - (opts->check ? 1U : 0))){
                 
                            retval = dataCommand(self, confirmed, port, data, len);
                        }
                        else{
                            
                            self->errno = LORA_ERRNO_SIZE;
                        }                                        
                    }
                    else{
                        
                        self->errno = LORA_ERRNO_NOCHANNEL;
                    }                
                }
                else{
                    
                    self->errno = LORA_ERRNO_NOCHANNEL;
                }
            }
            else{
                
                self->errno = LORA_ERRNO_PORT;
            }
        }
        else{
            
            self->errno = LORA_ERRNO_NOTJOINED;
        }
    }
    else{
        
        self->errno = LORA_ERRNO_BUSY;
    }
            
    return retval;
}

static bool dataCommand(struct lora_mac *self, bool confirmed, uint8_t port, const void *data, uint8_t len)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC((len == 0) || (data != NULL))
    
    bool retval = false;
    struct lora_frame_data f;
    struct lora_stream s;
    uint8_t maxPayload;
    uint8_t opts[15U];
    enum lora_signal_bandwidth bw;
    enum lora_spreading_factor sf;
    
    self->trials = 0U;
    
    self->tx.rate = self->ctx.rate;
    self->tx.power = self->ctx.power;
    
    /* pending MAC commands take priority over user payload */
    (void)LDL_Stream_init(&s, opts, sizeof(opts));
    
    if(self->dlChannelAns_pending){
    
        struct lora_dl_channel_ans ans;        
        (void)memset(&ans, 0, sizeof(ans));
        LDL_MAC_putDLChannelAns(&s, &ans);                                    
    }
    
    if(self->rxtimingSetupAns_pending){
        
        (void)LDL_MAC_putRXTimingSetupAns(&s);                                    
    }
    
    if(self->rxParamSetupAns_pending){
        
        struct lora_rx_param_setup_ans ans;
        (void)memset(&ans, 0, sizeof(ans));
        LDL_MAC_putRXParamSetupAns(&s, &ans);                                    
    }
    
    if(self->opts.check){

        LDL_MAC_putLinkCheckReq(&s);
    }                                
    
    LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &maxPayload);
    
    LORA_PEDANTIC(maxPayload >= LDL_Frame_dataOverhead())
    
    (void)memset(&f, 0, sizeof(f));
    
    f.type = confirmed ? FRAME_TYPE_DATA_CONFIRMED_UP : FRAME_TYPE_DATA_UNCONFIRMED_UP;
    f.devAddr = self->ctx.devAddr;
    f.counter = self->ctx.up;
    f.adr = self->ctx.adr;
    f.adrAckReq = self->adrAckReq;
    f.opts = opts;
    f.optsLen = LDL_Stream_tell(&s);
    f.port = port;
    
    self->state = LORA_STATE_WAIT_TX;    
    
    /* it's possible the user data doesn't fit after mac command priority */
    if((LDL_Stream_tell(&s) + LDL_Frame_dataOverhead() + len) <= maxPayload){
        
        f.data = data;
        f.dataLen = len;
        
        self->bufferLen = LDL_OPS_prepareData(self, &f, self->buffer, sizeof(self->buffer));
        
        self->op = confirmed ? LORA_OP_DATA_CONFIRMED : LORA_OP_DATA_UNCONFIRMED;
                
        retval = true;
    }
    /* no room for data, prioritise data */
    else{
        
        f.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
        f.port = 0U;
        f.data = opts;
        f.dataLen = f.optsLen;
        f.opts = NULL;
        f.optsLen = 0U;
        
        self->bufferLen = LDL_OPS_prepareData(self, &f, self->buffer, sizeof(self->buffer));
        
        self->op = LORA_OP_DATA_UNCONFIRMED;
        self->errno = LORA_ERRNO_SIZE;          //fixme: special error code to say goalpost moved?                
    }
    
    self->ctx.up++;
    
    /* putData must have failed for some reason */
    LORA_PEDANTIC(self->bufferLen > 0U)
    
    uint32_t send_delay = 0U;
    
    if(self->opts.dither > 0U){
        
        send_delay = (rand32(self->app) % ((uint32_t)self->opts.dither * LDL_System_tps()));
    }
    
    self->service_start_time = timeNow(self) + (send_delay / LDL_System_tps());            
                
    LDL_MAC_timerSet(self, LORA_TIMER_WAITA, send_delay);
    
    return retval;
}

static void adaptRate(struct lora_mac *self)
{
    self->adrAckReq = false;
    
    if(self->ctx.adr){
                
        if(self->adrAckCounter < UINT8_MAX){
            
            if(self->adrAckCounter >= ADRAckLimit){
                
                self->adrAckReq = true;
            
                LORA_DEBUG(self->app, "adr: adrAckCounter=%u (past ADRAckLimit)", self->adrAckCounter)
            
                if(self->adrAckCounter >= (ADRAckLimit + ADRAckDelay)){
                
                    if(((self->adrAckCounter - (ADRAckLimit + ADRAckDelay)) % ADRAckDelay) == 0U){
                
                        if(self->ctx.power == 0U){
                
                            if(self->ctx.rate > LORA_DEFAULT_RATE){
                                                            
                                self->ctx.rate--;
                                LORA_DEBUG(self->app, "adr: rate reduced to %u", self->ctx.rate)
                            }
                            else{
                                
                                LORA_DEBUG(self->app, "adr: all channels unmasked")
                                
                                unmaskAllChannels(self->ctx.chMask, self->region);
                                
                                self->adrAckCounter = UINT8_MAX;
                            }
                        }
                        else{
                            
                            LORA_DEBUG(self->app, "adr: full power enabled")
                            self->ctx.power = 0U;
                        }
                    }
                }    
            }
                
            self->adrAckCounter++;            
        }        
    }
}

static uint32_t transmitTime(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size, bool crc)
{
    /* from 4.1.1.7 of sx1272 datasheet
     *
     * Ts (symbol period)
     * Rs (symbol rate)
     * PL (payload length)
     * SF (spreading factor
     * CRC (presence of trailing CRC)
     * IH (presence of implicit header)
     * DE (presence of data rate optimize)
     * CR (coding rate 1..4)
     * 
     *
     * Ts = 1 / Rs
     * Tpreamble = ( Npreamble x 4.25 ) x Tsym
     *
     * Npayload = 8 + max( ceil[( 8PL - 4SF + 28 + 16CRC + 20IH ) / ( 4(SF - 2DE) )] x (CR + 4), 0 )
     *
     * Tpayload = Npayload x Ts
     *
     * Tpacket = Tpreamble + Tpayload
     * 
     * */

    bool header;
    bool lowDataRateOptimize;
    uint32_t Tpacket;
    uint32_t Ts;
    uint32_t Tpreamble;
    uint32_t numerator;
    uint32_t denom;
    uint32_t Npayload;
    uint32_t Tpayload;
    
    Tpacket = 0UL;
    
    /* optimise this mode according to the datasheet */
    lowDataRateOptimize = ((bw == BW_125) && ((sf == SF_11) || (sf == SF_12))) ? true : false;    
    
    /* lorawan always uses a header */
    header = true; 

    Ts = symbolPeriod(sf, bw);
    Tpreamble = (Ts * 12UL) +  (Ts / 4UL);

    numerator = (8UL * (uint32_t)size) - (4UL * (uint32_t)sf) + 28UL + ( crc ? 16UL : 0UL ) - ( header ? 20UL : 0UL );
    denom = 4UL * ((uint32_t)sf - ( lowDataRateOptimize ? 2UL : 0UL ));

    Npayload = 8UL + ((((numerator / denom) + (((numerator % denom) != 0UL) ? 1UL : 0UL)) * ((uint32_t)CR_5 + 4UL)));

    Tpayload = Npayload * Ts;

    Tpacket = Tpreamble + Tpayload;

    return Tpacket;
}

static uint32_t symbolPeriod(enum lora_spreading_factor sf, enum lora_signal_bandwidth bw)
{
    return ((((uint32_t)1U) << sf) * LDL_System_tps()) / LDL_MAC_bwToNumber(bw);
}

static uint8_t extraSymbols(uint32_t xtal_error, uint32_t symbol_period)
{
    return (xtal_error / symbol_period) + (((xtal_error % symbol_period) > 0U) ? 1U : 0U);        
}

static uint8_t processCommands(struct lora_mac *self, const uint8_t *in, uint8_t len, bool inFopts, uint8_t *out, uint8_t max)
{
    uint8_t pos = 0U;
    
    struct lora_stream s_in;
    struct lora_stream s_out;
    
    struct lora_downstream_cmd cmd;
    struct lora_mac_session shadow;
    union lora_mac_response_arg arg;
    struct lora_link_adr_ans adr_ans;
    enum lora_mac_cmd_type next_cmd;
    
    enum {
        
        _NO_ADR,
        _ADR_OK,
        _ADR_BAD
        
    } adr_state = _NO_ADR;
    
    (void)memset(&adr_ans, 0, sizeof(adr_ans));
    (void)memcpy(&shadow, &self->ctx, sizeof(shadow));

    (void)LDL_Stream_initReadOnly(&s_in, in, len);
    (void)LDL_Stream_init(&s_out, out, max);
    
    adr_ans.channelMaskOK = true;
    
    while(LDL_MAC_getDownCommand(&s_in, &cmd)){
        
        /* save the position in case the output buffer becomes
         * full partway through outputting a command */
        pos = LDL_Stream_tell(&s_out);
        
        switch(cmd.type){
        default:
            break;     
#ifndef LORA_DISABLE_CHECK                                   
        case LINK_CHECK:    
        {                
            const struct lora_link_check_ans *ans = &cmd.fields.linkCheckAns;
            
            arg.link_status.inFOpt = inFopts;
            arg.link_status.margin = ans->margin;
            arg.link_status.gwCount = ans->gwCount;            
            
            LORA_DEBUG(self->app, "link_check_ans: margin=%u gwCount=%u", 
                ans->margin,
                ans->gwCount             
            )
            
            self->handler(self->app, LORA_MAC_LINK_STATUS, &arg);                                                             
        }
            break;
#endif                            

        case LINK_ADR:              
        {
            const struct lora_link_adr_req *req = &cmd.fields.linkADRReq;
            
            LORA_DEBUG(self->app, "link_adr_req: dataRate=%u txPower=%u chMask=%04x chMaskCntl=%u nbTrans=%u",
                req->dataRate, req->txPower, req->channelMask, req->channelMaskControl, req->nbTrans)
            
            uint8_t i;
            
            if(LDL_Region_isDynamic(self->region)){
                
                switch(req->channelMaskControl){
                case 0U:
                
                    /* mask/unmask channels 0..15 */
                    for(i=0U; i < (sizeof(req->channelMask)*8U); i++){
                        
                        if((req->channelMask & (1U << i)) > 0U){
                            
                            (void)unmaskChannel(shadow.chMask, self->region, i);
                        }
                        else{
                            
                            (void)maskChannel(shadow.chMask, self->region, i);
                        }
                    }
                    break;            
                    
                case 6U:
                
                    unmaskAllChannels(shadow.chMask, self->region);
                    break;           
                     
                default:
                    adr_ans.channelMaskOK = false;
                    break;
                }
            }
            else{
                
                switch(req->channelMaskControl){
                case 6U:     /* all 125KHz on */
                case 7U:     /* all 125KHz off */
                
                    /* fixme: there is probably a more robust way to do this...right
                     * now we only support US and AU fixed channel plans so this works.
                     * */
                    for(i=0U; i < 64U; i++){
                        
                        if(req->channelMaskControl == 6U){
                            
                            (void)unmaskChannel(shadow.chMask, self->region, i);
                        }
                        else{
                            
                            (void)maskChannel(shadow.chMask, self->region, i);
                        }            
                    }                                  
                    break;
                    
                default:
                    
                    for(i=0U; i < (sizeof(req->channelMask)*8U); i++){
                        
                        if((req->channelMask & (1U << i)) > 0U){
                            
                            (void)unmaskChannel(shadow.chMask, self->region, (req->channelMaskControl * 16U) + i);
                        }
                        else{
                            
                            (void)maskChannel(shadow.chMask, self->region, (req->channelMaskControl * 16U) + i);
                        }
                    }
                    break;
                }            
            }
            
            if(!LDL_MAC_peekNextCommand(&s_in, &next_cmd) || (next_cmd != LINK_ADR)){
             
                adr_ans.dataRateOK = true;
                adr_ans.powerOK = true;
             
                /* nbTrans setting 0 means keep existing */
                if(req->nbTrans > 0U){
                
                    shadow.nbTrans = req->nbTrans & 0xfU;
                    
                    if(shadow.nbTrans > LORA_REDUNDANCY_MAX){
                        
                        shadow.nbTrans = LORA_REDUNDANCY_MAX;
                    }
                }
                
                /* ignore rate setting 16 */
                if(req->dataRate < 0xfU){            
                    
                    // todo: need to pin out of range to maximum
                    if(rateSettingIsValid(self->region, req->dataRate)){
                    
                        shadow.rate = req->dataRate;            
                    }
                    else{
                                        
                        adr_ans.dataRateOK = false;
                    }
                }
                
                /* ignore power setting 16 */
                if(req->txPower < 0xfU){            
                    
                    if(LDL_Region_validateTXPower(self->region, req->txPower)){
                    
                        shadow.power = req->txPower;        
                    }
                    else{
                     
                        adr_ans.powerOK = false;
                    }        
                }   
             
                if(adr_ans.dataRateOK && adr_ans.powerOK && adr_ans.channelMaskOK){
                    
                    adr_state = _ADR_OK;
                }
                else{
                    
                    adr_state = _ADR_BAD;
                }
               
             
                LORA_DEBUG(self->app, "link_adr_ans: powerOK=%s dataRateOK=%s channelMaskOK=%s",
                    adr_ans.dataRateOK ? "true" : "false", 
                    adr_ans.powerOK ? "true" : "false", 
                    adr_ans.channelMaskOK ? "true" : "false"
                )
             
                LDL_MAC_putLinkADRAns(&s_out, &adr_ans);
            }                
            
        }
            break;
        
        case DUTY_CYCLE:
        {    
            const struct lora_duty_cycle_req *req = &cmd.fields.dutyCycleReq;
            
            LORA_DEBUG(self->app, "duty_cycle_req: %u", req->maxDutyCycle)
            
            shadow.maxDutyCycle = req->maxDutyCycle;                        
            LDL_MAC_putDutyCycleAns(&s_out);
        }
            break;
        
        case RX_PARAM_SETUP:     
        {
            const struct lora_rx_param_setup_req *req = &cmd.fields.rxParamSetupReq;
            struct lora_rx_param_setup_ans ans;
         
            LORA_DEBUG(self->app, "rx_param_setup: rx1DROffset=%u rx2DataRate=%u freq=%"PRIu32,
                req->rx1DROffset,
                req->rx2DataRate,
                req->freq
            )
            
            // todo: validation
            
            shadow.rx1DROffset = req->rx1DROffset;
            shadow.rx2DataRate = req->rx2DataRate;
            shadow.rx2Freq = req->freq;
            
            ans.rx1DROffsetOK = true;
            ans.rx2DataRateOK = true;
            ans.channelOK = true;       
            
            LDL_MAC_putRXParamSetupAns(&s_out, &ans);
        }
            break;
        
        case DEV_STATUS:
        {
            LORA_DEBUG(self->app, "dev_status_req")
            struct lora_dev_status_ans ans;        
            
            ans.battery = LDL_System_getBatteryLevel(self->app);
            ans.margin = (int8_t)self->margin;
            
            LDL_MAC_putDevStatusAns(&s_out, &ans);
        }
            break;
            
        case NEW_CHANNEL:    
        
            LORA_DEBUG(self->app, "new_channel_req:")
            
            if(LDL_Region_isDynamic(self->region)){
            
                struct lora_new_channel_ans ans;
            
                ans.dataRateRangeOK = LDL_Region_validateRate(self->region, cmd.fields.newChannelReq.chIndex, cmd.fields.newChannelReq.minDR, cmd.fields.newChannelReq.maxDR);        
                ans.channelFrequencyOK = LDL_Region_validateFreq(self->region, cmd.fields.newChannelReq.chIndex, cmd.fields.newChannelReq.freq);
                
                if(ans.dataRateRangeOK && ans.channelFrequencyOK){
                    
                    (void)setChannel(shadow.chConfig, self->region, cmd.fields.newChannelReq.chIndex, cmd.fields.newChannelReq.freq, cmd.fields.newChannelReq.minDR, cmd.fields.newChannelReq.maxDR);                        
                }            
                
                LDL_MAC_putNewChannelAns(&s_out, &ans);
            }
            break; 
                   
        case DL_CHANNEL:            
            
            LORA_DEBUG(self->app, "dl_channel:")
            
            if(LDL_Region_isDynamic(self->region)){
                
                struct lora_dl_channel_ans ans;
                
                ans.uplinkFreqOK = true;
                ans.channelFrequencyOK = LDL_Region_validateFreq(self->region, cmd.fields.dlChannelReq.chIndex, cmd.fields.dlChannelReq.freq);
                
                LDL_MAC_putDLChannelAns(&s_out, &ans);            
            }        
            break;
        
        case RX_TIMING_SETUP:
        {        
            LORA_DEBUG(self->app, "handing rx_timing_setup")
            
            shadow.rx1Delay = cmd.fields.rxTimingSetupReq.delay;
            
            LDL_MAC_putRXTimingSetupAns(&s_out);
        }
            break;
        
        case TX_PARAM_SETUP:        
            
            LORA_DEBUG(self->app, "handing tx_param_setup")    
            break;
        }
        
        /* this ensures the output stream doesn't contain part of a MAC command */
        if(LDL_Stream_error(&s_out)){
            
            LDL_Stream_seekSet(&s_out, pos);
        }        
    }
    
    /* roll back ADR request if not successful */
    if(adr_state == _ADR_BAD){
        
        LORA_DEBUG(self->app, "bad ADR setting; rollback")
        
        (void)memcpy(shadow.chMask, self->ctx.chMask, sizeof(shadow.chMask));
        
        shadow.rate = self->ctx.rate;
        shadow.power = self->ctx.power;
        shadow.nbTrans = self->ctx.nbTrans;
    }

    /* otherwise apply changes */
    (void)memcpy(&self->ctx, &shadow, sizeof(self->ctx));  
    
    /* return the length of data written to the output buffer */
    return LDL_Stream_tell(&s_out);  
}

static void registerTime(struct lora_mac *self, uint32_t freq, uint32_t airTime)
{
    uint8_t band;
    uint32_t offtime;
    
    if(LDL_Region_getBand(self->region, freq, &band)){
    
        offtime = LDL_Region_getOffTimeFactor(self->region, band);
    
        if(offtime > 0U){
        
            LORA_PEDANTIC( band < LORA_BAND_MAX )
            
            offtime = ticksToMS(airTime) * offtime;
            
            if((self->band[band] + offtime) < self->band[band]){
                
                self->band[band] = UINT32_MAX;
            }
            else{
                
                self->band[band] += offtime; 
            }
        }
    }
    
    if((self->op != LORA_OP_JOINING) && (self->ctx.maxDutyCycle > 0U)){
        
        offtime = ticksToMS(airTime) * ( 1UL << (self->ctx.maxDutyCycle & 0xfU));
        
        if((self->band[LORA_BAND_GLOBAL] + offtime) < self->band[LORA_BAND_GLOBAL]){
            
            self->band[LORA_BAND_GLOBAL] = UINT32_MAX;
        }
        else{
            
            self->band[LORA_BAND_GLOBAL] += offtime; 
        }
    }
}    



static bool selectChannel(const struct lora_mac *self, uint8_t rate, uint8_t prevChIndex, uint32_t limit, uint8_t *chIndex, uint32_t *freq)
{
    bool retval = false;
    uint8_t i;    
    uint8_t selection;    
    uint8_t available = 0U;
    uint8_t j = 0U;
    uint8_t minRate;
    uint8_t maxRate;    
    uint8_t except = UINT8_MAX;
    
    uint8_t mask[sizeof(self->ctx.chMask)];
    
    (void)memset(mask, 0, sizeof(mask));
    
    /* count number of available channels for this rate */
    for(i=0U; i < LDL_Region_numChannels(self->region); i++){
        
        if(isAvailable(self, i, rate, limit)){
        
            if(i == prevChIndex){
                
                except = i;
            }
        
            (void)maskChannel(mask, self->region, i);        
            available++;            
        }            
    }
        
    if(available > 0U){
    
        if(except != UINT8_MAX){
    
            if(available == 1U){
                
                except = UINT8_MAX;
            }
            else{
                
                available--;
            }
        }
    
        selection = LDL_System_rand(self->app) % available;
        
        for(i=0U; i < LDL_Region_numChannels(self->region); i++){
        
            if(channelIsMasked(mask, self->region, i)){
        
                if(except != i){
            
                    if(selection == j){
                        
                        if(getChannel(self->ctx.chConfig, self->region, i, freq, &minRate, &maxRate)){
                            
                            *chIndex = i;
                            retval = true;
                            break;
                        }                        
                    }
            
                    j++;            
                }
            }            
        }        
    }
    
    return retval;
}

static bool isAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate, uint32_t limit)
{
    bool retval = false;
    uint32_t freq;
    uint8_t minRate;    
    uint8_t maxRate;    
    uint8_t band;
    
    if(!channelIsMasked(self->ctx.chMask, self->region, chIndex)){
    
        if(getChannel(self->ctx.chConfig, self->region, chIndex, &freq, &minRate, &maxRate)){
            
            if((rate >= minRate) && (rate <= maxRate)){
            
                if(LDL_Region_getBand(self->region, freq, &band)){
                
                    LORA_PEDANTIC( band < LORA_BAND_MAX )
                
                    if(self->band[band] <= limit){
                
                        retval = true;              
                    }
                }
            }
        }
    }
    
    return retval;
}

static bool msUntilAvailable(const struct lora_mac *self, uint8_t chIndex, uint8_t rate, uint32_t *ms)
{
    bool retval = false;
    uint32_t freq;
    uint8_t minRate;    
    uint8_t maxRate;    
    uint8_t band;
    
    if(!channelIsMasked(self->ctx.chMask, self->region, chIndex)){
    
        if(getChannel(self->ctx.chConfig, self->region, chIndex, &freq, &minRate, &maxRate)){
            
            if((rate >= minRate) && (rate <= maxRate)){
            
                if(LDL_Region_getBand(self->region, freq, &band)){
                
                    LORA_PEDANTIC( band < LORA_BAND_MAX )
                
                    *ms = (self->band[band] > self->band[LORA_BAND_GLOBAL]) ? self->band[band] : self->band[LORA_BAND_GLOBAL];
                    
                    retval = true;                
                }
            }
        }
    }
    
    return retval;
}

static void restoreDefaults(struct lora_mac *self, bool keep)
{
    if(!keep){
        
        (void)memset(&self->ctx, 0, sizeof(self->ctx));
        self->ctx.rate = LORA_DEFAULT_RATE;    
        self->ctx.adr = true;        
    }
    else{
        
        self->ctx.up = 0U;
        self->ctx.nwkDown = 0U;
        self->ctx.appDown = 0U;
        
        (void)memset(self->ctx.chConfig, 0, sizeof(self->ctx.chConfig));
        (void)memset(self->ctx.chMask, 0, sizeof(self->ctx.chMask));        
        self->ctx.joined = false;        
    }
    
    LDL_Region_getDefaultChannels(self->region, self);    
    
    self->ctx.rx1DROffset = LDL_Region_getRX1Offset(self->region);
    self->ctx.rx1Delay = LDL_Region_getRX1Delay(self->region);
    self->ctx.rx2DataRate = LDL_Region_getRX2Rate(self->region);
    self->ctx.rx2Freq = LDL_Region_getRX2Freq(self->region);    
    self->ctx.version = 0U;
}

static bool getChannel(const struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t *freq, uint8_t *minRate, uint8_t *maxRate)
{
    bool retval = false;
    
    if(LDL_Region_isDynamic(region)){
        
        if(chIndex < LDL_Region_numChannels(region)){
            
            *freq = (self[chIndex].freqAndRate >> 8) * 100U;
            *minRate = (self[chIndex].freqAndRate >> 4) & 0xfU;
            *maxRate = self[chIndex].freqAndRate & 0xfU;
            
            retval = true;
        }        
    }
    else{
        
        retval = LDL_Region_getChannel(region, chIndex, freq, minRate, maxRate);                        
    }
     
    return retval;
}

static bool setChannel(struct lora_mac_channel *self, enum lora_region region, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
        
        self[chIndex].freqAndRate = ((freq/100U) << 8) | ((minRate << 4) & 0xfU) | (maxRate & 0xfU);
        
        retval = true;
    }
    
    return retval;
}

static bool maskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
    
        self[chIndex / 8U] |= (1U << (chIndex % 8U));
        retval = true;
    }
    
    return retval;
}

static bool unmaskChannel(uint8_t *self, enum lora_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
    
        self[chIndex / 8U] &= ~(1U << (chIndex % 8U));
        retval = true;
    }
    
    return retval;
}

static void unmaskAllChannels(uint8_t *self, enum lora_region region)
{
    uint8_t i;
    
    for(i=0U; i < LDL_Region_numChannels(region); i++){
        
        (void)unmaskChannel(self, region, i);
    }    
}

static bool channelIsMasked(const uint8_t *self, enum lora_region region, uint8_t chIndex)
{
    bool retval = false;
    
    if(chIndex < LDL_Region_numChannels(region)){
    
        retval = ((self[chIndex / 8U] & (1U << (chIndex % 8U))) > 0U);        
    }
    
    return retval;    
}

static bool rateSettingIsValid(enum lora_region region, uint8_t rate)
{
    bool retval = false;
    uint8_t i;
    
    for(i=0U; i < LDL_Region_numChannels(region); i++){
        
        if(LDL_Region_validateRate(region, i, rate, rate)){
            
            retval = true;
            break;
        }
    }
    
    return retval;
}

static uint32_t timeNow(struct lora_mac *self)
{
    uint32_t seconds;
    uint32_t ticks;
    uint32_t since;
    uint32_t part;
    
    ticks = LDL_System_ticks(self->app);
    since = timerDelta(self->polled_time_ticks, ticks);
    
    seconds = since / LDL_System_tps();
    
    if(seconds > 0U){
    
        part = since % LDL_System_tps();        
        self->polled_time_ticks = (ticks - part);    
        self->time += seconds;
    }
    
    return self->time;        
}

static uint32_t getRetryDuty(uint32_t seconds_since)
{
    /* reset after one day */
    uint32_t delta = seconds_since % (60UL*60UL*24UL);
    
    /* 36/3600 (0.01) */
    if(delta < (60UL*60UL)){
        
        return 100UL;
    }
    /* 36/36000 (0.001) */
    else if(delta < (11UL*60UL*60UL)){
     
        return 1000UL;
    }
    /* 8.7/86400 (0.0001) */
    else{
        
        return 10000UL;    
    }
}

static uint32_t timerDelta(uint32_t timeout, uint32_t time)
{
    return (timeout <= time) ? (time - timeout) : (UINT32_MAX - timeout + time);
}

static void processBands(struct lora_mac *self)
{
    uint32_t since;
    uint32_t ticks;
    uint32_t diff;
    size_t i;
    
    ticks = LDL_System_ticks(self->app);
    diff = timerDelta(self->polled_band_ticks, ticks);    
    since = diff / LDL_System_tps() * 1000UL;
    
    if(since > 0U){
    
        self->polled_band_ticks += LDL_System_tps() / 1000U * since;
        
        for(i=0U; i < (sizeof(self->band)/sizeof(*self->band)); i++){

            if(self->band[i] > 0U){
            
                if(self->band[i] < since){
                   
                  self->band[i] = 0U;  
                }
                else{
                   
                   self->band[i] -= since;
                }                                    
            }            
        }        
    }
}

static uint32_t nextBandEvent(const struct lora_mac *self)
{
    uint32_t retval = UINT32_MAX;
    size_t i;
    
    for(i=0U; i < (sizeof(self->band)/sizeof(*self->band)); i++){
        
        if(self->band[i] > 0U){
        
            if(self->band[i] < retval){
                
                retval = self->band[i];
            }        
        }
    }
    
    return retval;
}

static void downlinkMissingHandler(struct lora_mac *self)
{
    union lora_mac_response_arg arg;
    uint8_t nbTrans;
    enum lora_spreading_factor sf;
    enum lora_signal_bandwidth bw;
    uint32_t delta;
    uint32_t tx_time;
    uint8_t mtu;
    
    if(self->opts.nbTrans > 0U){
        
        nbTrans = self->opts.nbTrans;
    }
    else{
        
        nbTrans = (self->ctx.nbTrans > 0) ? self->ctx.nbTrans : 1U;
    }
    
    self->trials++;
    
    delta = (timeNow(self) - self->service_start_time);
    
    LDL_Region_convertRate(self->region, self->tx.rate, &sf, &bw, &mtu);
    
    tx_time = ticksToMS(transmitTime(bw, sf, self->bufferLen, true));
        
    switch(self->op){
    default:
    case LORA_OP_NONE:
        break;
    case LORA_OP_DATA_CONFIRMED:

        if(self->trials < nbTrans){

            self->band[LORA_BAND_RETRY] = tx_time * getRetryDuty(delta);
            self->state = LORA_STATE_WAIT_RETRY;            
        }
        else{
            
            adaptRate(self);
         
            self->tx.rate = self->ctx.rate;
            self->tx.power = self->ctx.power;
                
#ifndef LORA_DISABLE_DATA_CONFIRMED_EVENT                
            self->handler(self->app, LORA_MAC_DATA_TIMEOUT, NULL);
#endif                  
            self->state = LORA_STATE_IDLE;
            self->op = LORA_OP_NONE;
        }
        break;
        
    case LORA_OP_DATA_UNCONFIRMED:
    
        if(self->trials < nbTrans){
            
            if((self->band[LORA_BAND_GLOBAL] < LDL_Region_getMaxDCycleOffLimit(self->region)) && selectChannel(self, self->tx.rate, self->tx.chIndex, LDL_Region_getMaxDCycleOffLimit(self->region), &self->tx.chIndex, &self->tx.freq)){
            
                LDL_MAC_timerSet(self, LORA_TIMER_WAITA, 0U);
                self->state = LORA_STATE_WAIT_TX;
            }
            else{
                
                LORA_DEBUG(self->app, "no channel available for retry")
                
#ifndef LORA_DISABLE_DATA_COMPLETE_EVENT                    
                self->handler(self->app, LORA_MAC_DATA_COMPLETE, NULL);
#endif                    
                self->state = LORA_STATE_IDLE;
                self->op = LORA_OP_NONE;                      
            }            
        }
        else{
        
            adaptRate(self);
         
            self->tx.rate = self->ctx.rate;
            self->tx.power = self->ctx.power;
                                
#ifndef LORA_DISABLE_DATA_COMPLETE_EVENT                    
            self->handler(self->app, LORA_MAC_DATA_COMPLETE, NULL);
#endif  
            self->state = LORA_STATE_IDLE;
            self->op = LORA_OP_NONE;                      
        }
        break;

    case LORA_OP_JOINING:
        
        self->band[LORA_BAND_RETRY] = tx_time * getRetryDuty(delta);
        
        self->tx.rate = LDL_Region_getJoinRate(self->region, self->trials);
        
#ifndef LORA_DISABLE_JOIN_TIMEOUT_EVENT                               
        self->handler(self->app, LORA_MAC_JOIN_TIMEOUT, &arg);                    
#endif                            
        self->state = LORA_STATE_WAIT_RETRY;        
        break;                    
    }        
}

static uint32_t ticksToMS(uint32_t ticks)
{
    return ticks * 1000UL / LDL_System_tps();
}

static uint32_t ticksToMSCoarse(uint32_t ticks)
{
    return ticks / LDL_System_tps() * 1000UL;
}

static uint32_t msUntilNextChannel(const struct lora_mac *self, uint8_t rate)
{
    uint8_t i;
    uint32_t min = UINT32_MAX;
    uint32_t ms;
    
    for(i=0U; i < LDL_Region_numChannels(self->region); i++){
        
        if(msUntilAvailable(self, i, rate, &ms)){
            
            if(ms < min){
                
                min = ms;
            }
        }
    }
    
    return min;
}

static uint32_t rand32(void *app)
{
    uint32_t retval;
    
    retval = LDL_System_rand(app);
    retval <<= 8;
    retval |= LDL_System_rand(app);
    retval <<= 8;
    retval |= LDL_System_rand(app);
    retval <<= 8;
    retval |= LDL_System_rand(app);
    
    return retval;
}

#ifndef LORA_DISABLE_SESSION_UPDATE
static void pushSessionUpdate(struct lora_mac *self)
{
    union lora_mac_response_arg arg;     
    arg.session_updated.session = &self->ctx;
    self->handler(self->app, LORA_MAC_SESSION_UPDATED, &arg);        
}
#endif

static void dummyResponseHandler(void *app, enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
}
