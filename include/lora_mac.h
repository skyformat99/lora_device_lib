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

#ifndef __LORA_MAC_H
#define __LORA_MAC_H

/** @file */

/**
 * @defgroup ldl LDL
 * 
 * # LDL Interface Documentation
 * 
 * Interface documentation for [LoRa Device Library](https://github.com/cjhdev/lora_device_lib).
 * 
 * - @ref ldl_mac MAC layer interface
 * - @ref ldl_radio radio driver interface
 * - @ref ldl_system portable system interface
 * - @ref ldl_radio_connector portable connector between radio driver and transceiver digital interface
 * - @ref ldl_build_options portable build options
 * - @ref ldl_tsm portable security module interface
 * - @ref ldl_crypto cryptographic implementations used by the default security module
 * 
 * ## Usage
 * 
 * Below is an example of how to use LDL to join and then send data. 
 * 
 * What isn't shown:
 * 
 * - complete implementation of @ref ldl_radio_connector
 * - implementation of @ref ldl_system
 * - implementation of functions marked as "extern"
 * 
 * This example would need the following @ref ldl_build_options to be defined:
 * 
 * - #LORA_ENABLE_SX1272
 * - #LORA_ENABLE_EU_863_870
 * 
 * @include examples/doxygen/example.c
 * 
 * ## Examples
 * 
 *  - Arduino (AVR)
 *      - [arduino_ldl.cpp](https://github.com/cjhdev/lora_device_lib/tree/master/bindings/arduino/output/arduino_ldl/arduino_ldl.cpp)
 *      - [arduino_ldl.h](https://github.com/cjhdev/lora_device_lib/tree/master/bindings/arduino/output/arduino_ldl/arduino_ldl.h)
 *      - [platform.h](https://github.com/cjhdev/lora_device_lib/tree/master/bindings/arduino/output/arduino_ldl/platform.h)
 * 
 * */

/**
 * @defgroup ldl_mac MAC
 * @ingroup ldl
 * 
 * # MAC Interface
 * 
 * Before accessing any of the interfaces #lora_mac must be initialised
 * by calling LDL_MAC_init().
 * 
 * Initialisation will take order of milliseconds but LDL_MAC_init() does 
 * not block, instead it schedules actions to run in the future.
 * It is then the responsibility of the application to poll LDL_MAC_process() 
 * from a loop in order to ensure the schedule is processed. LDL_MAC_ready() will return true
 * when initialisation is complete.
 * 
 * On systems that sleep, LDL_MAC_ticksUntilNextEvent() can be used in combination
 * with a wakeup timer to ensure that LDL_MAC_process() is called only when 
 * necessary. Note that the counter behind LDL_System_ticks() must continue
 * to increment during sleep.
 * 
 * Events (#lora_mac_response_type) that occur within LDL_MAC_process() are pushed back to the 
 * application using the #lora_mac_response_fn function pointer. 
 * This includes everything from state change notifications
 * to data received from the network.
 * 
 * The application is free to apply settings while waiting for LDL_MAC_ready() to become
 * true. Setting interfaces are:
 * 
 * - LDL_MAC_setRate()
 * - LDL_MAC_setPower()
 * - LDL_MAC_enableADR()
 * - LDL_MAC_disableADR()
 * - LDL_MAC_setMaxDCycle()
 * - LDL_MAC_setNbTrans()
 * 
 * Data services are not available until #lora_mac is joined to a network.
 * The join procedure is initiated by calling LDL_MAC_otaa(). LDL_MAC_otaa() will return false if the join procedure cannot be initiated. The application
 * can use LDL_MAC_errno() to discover the reason for failure.
 * 
 * The join procedure will run indefinitely until either the join succeeds, or the application
 * calls LDL_MAC_cancel() or LDL_MAC_forget(). The application can check on the join status
 * by calling LDL_MAC_joined().
 * 
 * Once joined the application can use the data services:
 * 
 * - LDL_MAC_unconfirmedData()
 * - LDL_MAC_confirmedData()
 * 
 * If the application wishes to un-join it can call LDL_MAC_forget().
 * 
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include "lora_region.h"
#include "lora_stream.h"
#include "lora_radio.h"

#include <stdint.h>
#include <stdbool.h>
 
struct lora_mac;
struct lora_sm;

/** Event types pushed to application
 * 
 * @see lora_mac_response_arg
 * 
 *  */
enum lora_mac_response_type {
        
    /** diagnostic event: radio chip did not respond as expected and will now be reset
     * 
     * */
    LORA_MAC_CHIP_ERROR,
    
    /** diagnostic event: radio chip is in process of being reset
     * 
     * MAC will send #LORA_MAC_STARTUP when it is ready again
     * 
     * */
    LORA_MAC_RESET,
    
    /** MAC has started and is now ready for commands
     * 
     * */
    LORA_MAC_STARTUP,
    
    /** join request was answered and MAC is now joined */
    LORA_MAC_JOIN_COMPLETE,
    
    /** join request was not answered (MAC will try again) */
    LORA_MAC_JOIN_TIMEOUT,
    
    /** data request (confirmed or unconfirmed) completed successfully */
    LORA_MAC_DATA_COMPLETE,
    
    /** confirmed data request was not answered */
    LORA_MAC_DATA_TIMEOUT,
    
    /** confirmed data request was answered but the ACK bit wasn't set */
    LORA_MAC_DATA_NAK,
    
    /** data receieved */
    LORA_MAC_RX,
    
    /** LinkCheckAns */
    LORA_MAC_LINK_STATUS,
    
    /** diagnostic event: RX1 window opened */
    LORA_MAC_RX1_SLOT,
    
    /** diagnostic event: RX2 window opened */
    LORA_MAC_RX2_SLOT,
    
    /** diagnostic event: a frame has been receieved in an RX window */
    LORA_MAC_DOWNSTREAM,
        
    /** diagnostic event: transmit complete */
    LORA_MAC_TX_COMPLETE,
    
    /** diagnostic event: transmit begin */
    LORA_MAC_TX_BEGIN,
    
    /** #lora_mac_session has changed 
     * 
     * The application can choose to save the session at this point
     * 
     * */
    LORA_MAC_SESSION_UPDATED
};

struct lora_mac_session;

/** Event arguments sent to application
 * 
 * @see lora_mac_response_type
 * 
 *  */
union lora_mac_response_arg {

    /** #LORA_MAC_DOWNSTREAM argument */
    struct {
        
        int16_t rssi;   /**< rssi of frame */
        int16_t snr;    /**< snr of frame */
        uint8_t size;   /**< size of frame */
        
    } downstream;
    
    /** #LORA_MAC_RX argument */
    struct {
        
        const uint8_t *data;    /**< message data */
        uint16_t counter;       /**< frame counter */  
        uint8_t port;           /**< lorawan application port */  
        uint8_t size;           /**< size of message */
        
    } rx;     
    
    /** #LORA_MAC_LINK_STATUS argument */
    struct {
        
        bool inFOpt;        /**< link status was transported in Fopt field */
        int8_t margin;      /**< SNR margin */                     
        uint8_t gwCount;    /**< number of gateways in range */
        
    } link_status;
    
    /** #LORA_MAC_RX1_SLOT and #LORA_MAC_RX2_SLOT argument */
    struct {
    
        uint32_t margin;                /**< allowed error margin */
        uint32_t error;                 /**< ticks passed since scheduled event */
        uint32_t freq;                  /**< frequency */
        enum lora_signal_bandwidth bw;  /**< bandwidth */
        enum lora_spreading_factor sf;  /**< spreading factor */
        uint8_t timeout;                /**< symbol timeout */
        
    } rx_slot;    
    
    /** #LORA_MAC_TX_BEGIN argument */
    struct {
        
        uint32_t freq;                      /**< frequency */    
        enum lora_spreading_factor sf;      /**< spreading factor */
        enum lora_signal_bandwidth bw;      /**< bandwidth */
        uint8_t power;                      /**< power setting @warning this is not dBm */
        uint8_t size;                       /**< message size */
        
    } tx_begin;
    
    /** #LORA_MAC_STARTUP argument */
    struct {
        
        unsigned int entropy;               /**< srand seed from radio driver */
        
    } startup;
    
    /** #LORA_MAC_SESSION_UPDATED argument */
    struct { 
        
        const struct lora_mac_session *session;
        
    } session_updated;
};

/** LDL calls this function pointer to notify application of events
 * 
 * @param[in] app   app from LDL_MAC_init()
 * @param[in] type  event type (#lora_mac_response_type)  
 * @param[in] arg   **OPTIONAL** depending on #lora_mac_response_type
 * 
 * */
typedef void (*lora_mac_response_fn)(void *app, enum lora_mac_response_type type, const union lora_mac_response_arg *arg);

/** MAC state */
enum lora_mac_state {

    LORA_STATE_INIT,            /**< stack has been memset to zero */

    LORA_STATE_INIT_RESET,      /**< holding reset after _INIT */
    LORA_STATE_INIT_LOCKOUT,    /**< waiting after _RESET */
    
    LORA_STATE_RECOVERY_RESET,   /**< holding reset after chip error */
    LORA_STATE_RECOVERY_LOCKOUT, /**< waiting after chip error reset */

    LORA_STATE_ENTROPY,         /**< sample entropy */

    LORA_STATE_IDLE,        /**< ready for operations */
    
    LORA_STATE_WAIT_TX,     /**< waiting for channel to become available */
    LORA_STATE_TX,          /**< radio is TX */
    LORA_STATE_WAIT_RX1,    /**< waiting for first RX window */
    LORA_STATE_RX1,         /**< first RX window */
    LORA_STATE_WAIT_RX2,    /**< waiting for second RX window */
    LORA_STATE_RX2,         /**< second RX window */    
    
    LORA_STATE_RX2_LOCKOUT, /**< used to ensure an out of range RX2 window is not clobbered */
    
    LORA_STATE_WAIT_RETRY,  /**< wait to retransmit / retry */
    
};

/** MAC operations */
enum lora_mac_operation {
  
    LORA_OP_NONE,                   /**< no active operation */
    LORA_OP_JOINING,                /**< MAC is performing a join */
    LORA_OP_REJOINING,              /**< MAC is performing a rejoin */
    LORA_OP_DATA_UNCONFIRMED,       /**< MAC is sending unconfirmed data */
    LORA_OP_DATA_CONFIRMED,         /**< MAC is sending confirmed data */    
    LORA_OP_RESET,                  /**< MAC is performing radio reset */
};

/** MAC error modes */
enum lora_mac_errno {
    
    LORA_ERRNO_NONE,
    LORA_ERRNO_NOCHANNEL,   /**< upstream channel not available */
    LORA_ERRNO_SIZE,        /**< message too large to send */
    LORA_ERRNO_RATE,        /**< data rate setting not valid for region */
    LORA_ERRNO_PORT,        /**< port not valid for upstream message */
    LORA_ERRNO_BUSY,        /**< stack is busy; cannot process request */
    LORA_ERRNO_NOTJOINED,   /**< stack is not joined; cannot process request */
    LORA_ERRNO_POWER,       /**< power setting not valid for region */
    LORA_ERRNO_INTERNAL     /**< implementation fault */
};

/* band array indices */
enum lora_band_index {
    
    LORA_BAND_1,
    LORA_BAND_2,
    LORA_BAND_3,
    LORA_BAND_4,
    LORA_BAND_5,
    LORA_BAND_GLOBAL,
    LORA_BAND_RETRY,
    LORA_BAND_MAX
};

enum lora_timer_inst {
    
    LORA_TIMER_WAITA,
    LORA_TIMER_WAITB,
    LORA_TIMER_BAND,
    LORA_TIMER_MAX
};

struct lora_timer {
    
    uint32_t time;    
    bool armed;
};

enum lora_input_type {
  
    LORA_INPUT_TX_COMPLETE,
    LORA_INPUT_RX_READY,
    LORA_INPUT_RX_TIMEOUT
};

struct lora_input {
    
    uint8_t armed;
    uint8_t state;
    uint32_t time;
};

struct lora_mac_channel {
    
    uint32_t freqAndRate;
    uint32_t dlFreq;
};

/** session cache */
struct lora_mac_session {
    
    /* frame counters */
    uint32_t up;
    uint16_t appDown;
    uint16_t nwkDown;
    
    uint32_t devAddr;
    
    uint8_t netID;
    
    struct lora_mac_channel chConfig[16U];
    
    uint8_t chMask[72U / 8U];
    
    uint8_t rate;
    uint8_t power;
    
    uint8_t maxDutyCycle;    
    uint8_t nbTrans;
    
    uint8_t rx1DROffset;    
    uint8_t rx1Delay;        
    uint8_t rx2DataRate;    
    uint8_t rx2Rate;   
    
    uint32_t rx2Freq;
    
    bool joined;
    bool adr;
    
    uint8_t version;
};

/** data service invocation options */
struct lora_mac_data_opts {
    
    uint8_t nbTrans;        /**< redundancy (0..LORA_REDUNDANCY_MAX) */    
    bool check;             /**< piggy-back a LinkCheckReq */
    uint8_t dither;         /**< seconds of dither to add to the transmit schedule (0..60) */
};

/** MAC layer data */
struct lora_mac {

    enum lora_mac_state state;
    enum lora_mac_operation op;
    enum lora_mac_errno errno;
    
#ifdef LORA_ENABLE_STATIC_RX_BUFFER
    uint8_t rx_buffer[LORA_MAX_PACKET];
#endif    
    uint8_t buffer[LORA_MAX_PACKET];
    uint8_t bufferLen;
    
    /* off-time in ms per band */    
    uint32_t band[LORA_BAND_MAX];
    
    uint32_t polled_band_ticks;
    
    uint16_t devNonce;
    
    int16_t margin;

    /* time of the last valid downlink in seconds */
    uint32_t last_valid_downlink;
    
    /* the settings currently being used to TX */
    struct {
        
        uint8_t chIndex;
        uint32_t freq;
        uint8_t rate;
        uint8_t power;
        
    } tx;
    
    uint32_t rx1_margin;
    uint32_t rx2_margin;
    
    uint8_t rx1_symbols;    
    uint8_t rx2_symbols;
    
    struct lora_mac_session ctx;
    
    struct lora_sm *sm;
    struct lora_radio *radio;
    struct lora_input inputs;
    struct lora_timer timers[LORA_TIMER_MAX];
    
    enum lora_region region;
    
    lora_mac_response_fn handler;
    void *app;
    
    bool linkCheckReq_pending;
    bool rxParamSetupAns_pending;    
    bool dlChannelAns_pending;
    bool rxtimingSetupAns_pending;
    
    uint8_t adrAckCounter;
    bool adrAckReq;
    
    uint32_t time;
    uint32_t polled_time_ticks;
    
    uint32_t service_start_time;
    
    /* number of join/data trials */
    uint32_t trials;
  
    /* options and overrides applicable to current data service */
    struct lora_mac_data_opts opts;
};

/** passed as an argument to LDL_MAC_init() */
struct lora_mac_init_arg {
    
    /** pointer passed to #lora_mac_response_fn and @ref ldl_system functions */
    void *app;      
    
    /** initialised radio object */
    struct lora_radio *radio;
    
    /** security module object */
    struct lora_sm *sm;
    
    /** application callback #lora_mac_response_fn */
    lora_mac_response_fn handler;
    
    /** #lora_mac_session to load (NULL if not available) */
    const struct lora_mac_session *session;    
};


/** Initialise #lora_mac 
 * 
 * @param[in] self      #lora_mac
 * @param[in] region    lorawan region id #lora_region
 * @param[in] arg       #lora_mac_init_arg
 * 
 * @see LDL_Radio_init()
 * 
 * */
void LDL_MAC_init(struct lora_mac *self, enum lora_region region, const struct lora_mac_init_arg *arg);

/** Send data without confirmation
 * 
 * Once initiated MAC will send at most nbTrans times until a valid downlink is received. NbTrans may be set:
 * 
 * - globally by the network (via LinkADRReq)
 * - globally by the application (via LDL_MAC_setNbTrans())
 * - per invocation by #lora_mac_data_opts
 *
 * The application can cancel the service while it is in progress by calling LDL_MAC_cancel().
 * 
 * #lora_mac_response_fn will push #LORA_MAC_DATA_COMPLETE on completion.
 * 
 * @param[in] self  #lora_mac
 * @param[in] port  lorawan port (must be >0)
 * @param[in] data  pointer to message to send
 * @param[in] len   byte length of data
 * @param[in] opts  #lora_mac_data_opts (may be NULL)
 * 
 * @retval true     pending
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_unconfirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len, const struct lora_mac_data_opts *opts);

/** Send data with confirmation
 * 
 * Once initiated MAC will send at most nbTrans times until a confirmation is received. NbTrans may be set:
 * 
 * - globally by the network (via LinkADRReq)
 * - globally by the application (via LDL_MAC_setNbTrans())
 * - per invocation by #lora_mac_data_opts
 * 
 * The application can cancel the service while it is in progress by calling LDL_MAC_cancel().
 * 
 * #lora_mac_response_fn will push #LORA_MAC_DATA_TIMEOUT on every timeout
 * #lora_mac_response_fn will push #LORA_MAC_DATA_COMPLETE on completion
 * 
 * 
 * @param[in] self  #lora_mac
 * @param[in] port  lorawan port (must be >0)
 * @param[in] data  pointer to message to send
 * @param[in] len   byte length of data
 * @param[in] opts  #lora_mac_data_opts (may be NULL)
 * 
 * @retval true     pending
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_confirmedData(struct lora_mac *self, uint8_t port, const void *data, uint8_t len, const struct lora_mac_data_opts *opts);

/** Initiate Over The Air Activation
 * 
 * Once initiated MAC will keep trying to join forever.
 * 
 * - Application can cancel by calling LDL_MAC_cancel()
 * - #lora_mac_response_fn will push #LORA_MAC_JOIN_TIMEOUT on every timeout
 * - #lora_mac_response_fn will push #LORA_MAC_JOIN_COMPLETE on completion
 * 
 * @param[in] self  #lora_mac
 * 
 * @retval true     pending
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_otaa(struct lora_mac *self);

/** Forget network
 * 
 * @param[in] self  #lora_mac
 * 
 * */
void LDL_MAC_forget(struct lora_mac *self);

/** Return state to #LORA_STATE_IDLE
 * 
 * @note has no immediate effect if MAC is already in the process of resetting
 * the radio
 * 
 * @param[in] self  #lora_mac
 * 
 * */
void LDL_MAC_cancel(struct lora_mac *self);

/** Call to process next events
 * 
 * @param[in] self  #lora_mac
 * 
 * */
void LDL_MAC_process(struct lora_mac *self);

/** Get number of ticks until the next event
 * 
 * Aside from the passage of time, calls to the following functions may 
 * cause the return value of this function to be change:
 * 
 * - LDL_MAC_process()
 * - LDL_MAC_otaa()
 * - LDL_MAC_unconfirmedData()
 * - LDL_MAC_confirmedData()
 * - LDL_Radio_interrupt()
 * 
 * @param[in] self  #lora_mac
 * 
 * @return system ticks
 * 
 * @retval UINT32_MAX   there are no future events at this time
 * 
 * @note interrupt safe if LORA_SYSTEM_ENTER_CRITICAL() and LORA_SYSTEM_ENTER_CRITICAL() have been defined
 * 
 * */
uint32_t LDL_MAC_ticksUntilNextEvent(const struct lora_mac *self);

/** Set the transmit data rate
 * 
 * @param[in] self  #lora_mac
 * @param[in] rate
 * 
 * @retval true     applied
 * @retval false    error 
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_setRate(struct lora_mac *self, uint8_t rate);

/** Get the current transmit data rate
 * 
 * @param[in] self  #lora_mac
 * 
 * @return transmit data rate setting
 * 
 * */
uint8_t LDL_MAC_getRate(const struct lora_mac *self);

/** Set the transmit power
 * 
 * @param[in] self  #lora_mac
 * @param[in] power
 * 
 * @retval true     applied
 * @retval false    error
 * 
 * @see LDL_MAC_errno()
 * 
 * */
bool LDL_MAC_setPower(struct lora_mac *self, uint8_t power);

/** Get the current transmit power
 * 
 * @param[in] self  #lora_mac
 * 
 * @return transmit power setting
 * 
 * */
uint8_t LDL_MAC_getPower(const struct lora_mac *self);

/** Enable ADR mode
 * 
 * @param[in] self  #lora_mac
 * 
 * */
void LDL_MAC_enableADR(struct lora_mac *self);

/** Disable ADR mode
 * 
 * @param[in] self  #lora_mac
 * 
 * */
void LDL_MAC_disableADR(struct lora_mac *self);

/** Is ADR mode enabled?
 * 
 * @param[in] self  #lora_mac
 * 
 * @retval true     enabled
 * @retval false    not enabled
 * 
 * */
bool LDL_MAC_adr(const struct lora_mac *self);

/** Read the last error
 * 
 * The following functions will set the errno when they fail:
 * 
 * - LDL_MAC_otaa()
 * - LDL_MAC_unconfirmedData()
 * - LDL_MAC_confirmedData()
 * - LDL_MAC_setRate()
 * - LDL_MAC_setPower()
 * 
 * @param[in] self  #lora_mac
 * 
 * @return #lora_mac_errno
 * 
 * */
enum lora_mac_errno LDL_MAC_errno(const struct lora_mac *self);

/** Read the current operation
 * 
 * @param[in] self  #lora_mac
 * 
 * @return #lora_mac_operation
 * 
 * */
enum lora_mac_operation LDL_MAC_op(const struct lora_mac *self);

/** Read the current state
 * 
 * @param[in] self  #lora_mac
 * 
 * @return #lora_mac_state
 * 
 * */
enum lora_mac_state LDL_MAC_state(const struct lora_mac *self);

/** Is MAC joined?
 * 
 * @param[in] self  #lora_mac
 * 
 * @retval true     joined
 * @retval false    not joined
 * 
 * */
bool LDL_MAC_joined(const struct lora_mac *self);

/** Is MAC ready to send?
 * 
 * @param[in] self  #lora_mac
 * 
 * @retval true     ready
 * @retval false    not ready
 *
 * */
bool LDL_MAC_ready(const struct lora_mac *self);

/** Calculate transmit time of message sent from node
 * 
 * @param[in] bw    bandwidth
 * @param[in] sf    spreading factor
 * @param[in] size  size of message
 * 
 * @return system ticks
 * 
 * */
uint32_t LDL_MAC_transmitTimeUp(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size);

/** Calculate transmit time of message sent from gateway
 * 
 * @param[in] bw    bandwidth
 * @param[in] sf    spreading factor
 * @param[in] size  size of message
 * 
 * @return system ticks
 * 
 * */
uint32_t LDL_MAC_transmitTimeDown(enum lora_signal_bandwidth bw, enum lora_spreading_factor sf, uint8_t size);

/** Convert bandwidth enumeration to Hz
 * 
 * @param[in] bw bandwidth
 * @return Hz
 * 
 * */
uint32_t LDL_MAC_bwToNumber(enum lora_signal_bandwidth bw);



/** Get the maximum transfer unit in bytes
 * 
 * This number changes depending on:
 * 
 * - region
 * - rate
 * - pending mac commands
 * 
 * @param[in] self  #lora_mac
 * @retval mtu
 * 
 * */
uint8_t LDL_MAC_mtu(const struct lora_mac *self);

/** Seconds since last valid downlink message
 * 
 * A valid downlink is one that is:
 * 
 * - expected type for current operation
 * - well formed
 * - able to be decrypted
 * 
 * @param[in] self  #lora_mac
 * @return seconds since last valid downlink
 * 
 * @retval UINT32_MAX no valid downlink received
 * 
 * */
uint32_t LDL_MAC_timeSinceValidDownlink(struct lora_mac *self);

/** Set the aggregated duty cycle limit
 * 
 * duty cycle limit = 1 / (2 ^ limit)
 * 
 * @param[in] self  #lora_mac
 * @param[in] maxDCycle
 * 
 * @see LoRaWAN Specification: DutyCycleReq.DutyCyclePL.MaxDCycle
 * 
 * */
void LDL_MAC_setMaxDCycle(struct lora_mac *self, uint8_t maxDCycle);

/** Get aggregated duty cycle limit
 * 
 * @param[in] self  #lora_mac
 * 
 * @return maxDCycle
 * 
 * @see LoRaWAN Specification: DutyCycleReq.DutyCyclePL.MaxDCycle
 * 
 * */
uint8_t LDL_MAC_getMaxDCycle(const struct lora_mac *self);

/** Set transmission redundancy
 *
 * - confirmed and unconfirmed uplink frames are sent nbTrans times (or until acknowledgement is received)
 * - a value of zero will leave the setting unchanged
 * - limited to 15 or LORA_REDUNDANCY_MAX (whichever is lower)
 * 
 * @param[in] self  #lora_mac
 * @param[in] nbTrans
 * 
 * @see LoRaWAN Specification: LinkADRReq.Redundancy.NbTrans
 * 
 * */
void LDL_MAC_setNbTrans(struct lora_mac *self, uint8_t nbTrans);

/** Get transmission redundancy
 * 
 * @param[in] self  #lora_mac
 * 
 * @return nbTrans
 * 
 * @see LoRaWAN Specification: LinkADRReq.Redundancy.NbTrans
 * 
 * */
uint8_t LDL_MAC_getNbTrans(const struct lora_mac *self);

/** Return true to indicate that LDL is expecting to handle
 * a time sensitive event in the next interval.
 * 
 * This can be used by an application to ensure long-running tasks
 * do not cause LDL to miss important events.
 * 
 * @param[in] self      #lora_mac
 * @param[in] interval  seconds
 * 
 * 
 * @retval true    
 * @retval false    
 * 
 * */
bool LDL_MAC_priority(const struct lora_mac *self, uint8_t interval);


/* for internal use only */
void LDL_MAC_radioEvent(struct lora_mac *self, enum lora_radio_event event);
bool LDL_MAC_addChannel(struct lora_mac *self, uint8_t chIndex, uint32_t freq, uint8_t minRate, uint8_t maxRate);
void LDL_MAC_removeChannel(struct lora_mac *self, uint8_t chIndex);
bool LDL_MAC_maskChannel(struct lora_mac *self, uint8_t chIndex);
bool LDL_MAC_unmaskChannel(struct lora_mac *self, uint8_t chIndex);

void LDL_MAC_timerSet(struct lora_mac *self, enum lora_timer_inst timer, uint32_t timeout);
bool LDL_MAC_timerCheck(struct lora_mac *self, enum lora_timer_inst timer, uint32_t *error);
void LDL_MAC_timerClear(struct lora_mac *self, enum lora_timer_inst timer);
uint32_t LDL_MAC_timerTicksUntilNext(const struct lora_mac *self);
uint32_t LDL_MAC_timerTicksUntil(const struct lora_mac *self, enum lora_timer_inst timer, uint32_t *error);

void LDL_MAC_inputArm(struct lora_mac *self, enum lora_input_type type);
bool LDL_MAC_inputCheck(const struct lora_mac *self, enum lora_input_type type, uint32_t *error);
void LDL_MAC_inputClear(struct lora_mac *self);
void LDL_MAC_inputSignal(struct lora_mac *self, enum lora_input_type type);
bool LDL_MAC_inputPending(const struct lora_mac *self);

#ifdef __cplusplus
}
#endif

/** @} */
#endif
