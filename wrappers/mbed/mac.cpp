#include "mac.h"

#include <inttypes.h>

using namespace LDL;

Timer MAC::timer;

/* constructors *******************************************************/

MAC::MAC(Store &store, SM &sm, Radio &radio) : 
    radio(radio),
    sm(sm),
    store(store)
{
    timer.start();
}

/* functions  *********************************************************/

uint32_t LDL_System_ticks(void *app)
{
    return MAC::ticks(app);
}

uint32_t LDL_System_tps(void)
{
    return 1000UL;    
}

uint32_t LDL_System_advance(void)
{
    return 0U;
}

uint32_t LDL_System_eps(void)
{
    return 1U;    
}

/* protected static  **************************************************/

MAC *
MAC::to_obj(void *self)
{
    return static_cast<MAC *>(self);
}

void
MAC::app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    MAC *self = to_obj(app);

    if((type == LDL_MAC_STARTUP) && (self->entropy_cb)){

        self->entropy_cb(arg->startup.entropy);                   
    }

    if((type == LDL_MAC_RX) && (self->data_cb)){

        self->data_cb(arg->rx.port, arg->rx.data, arg->rx.size);
    }

    if(self->event_cb){

        self->event_cb(type, arg);
    }
}

/* protected **********************************************************/

void
MAC::do_process()
{
    LDL_MAC_process(&mac);
    uint32_t next = LDL_MAC_ticksUntilNextEvent(&mac);
    if(next < UINT32_MAX){
        events.cancel(next_event_handler);
        next_event_handler = events.call_in(std::chrono::duration<uint32_t>(next), callback(this, &MAC::do_process));
    }
}

void
MAC::do_handle_radio_event(enum ldl_radio_event event)
{
    LDL_MAC_radioEvent(&mac, event);
    do_process();
}

void
MAC::handle_radio_event(enum ldl_radio_event event)
{
    events.call(callback(this, &MAC::do_handle_radio_event), event);
    do_process();
}

void
MAC::do_unconfirmed(bool *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    *retval = LDL_MAC_unconfirmedData(&mac, port, data, len, opts);
    do_process();
    notify_api();
}

void
MAC::do_confirmed(bool *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    *retval = LDL_MAC_unconfirmedData(&mac, port, data, len, opts);
    do_process();
    notify_api();
}

void
MAC::do_otaa(bool *retval)
{
    *retval = LDL_MAC_otaa(&mac);
    do_process();
    notify_api();
}

void
MAC::do_forget()
{
    LDL_MAC_forget(&mac);
    do_process();
    notify_api();
}

void
MAC::do_set_rate(bool *retval, uint8_t value)
{
    *retval = LDL_MAC_setRate(&mac, value);
    notify_api();
}

void
MAC::do_get_rate(uint8_t *retval)
{
    *retval = LDL_MAC_getRate(&mac);
    notify_api();
}

void
MAC::do_set_power(bool *retval, uint8_t value)
{
    *retval = LDL_MAC_setRate(&mac, value);
    notify_api();
}

void
MAC::do_get_power(uint8_t *retval)
{
    *retval = LDL_MAC_getPower(&mac);
    notify_api();
}

void
MAC::do_enable_adr()
{
    LDL_MAC_enableADR(&mac);
    notify_api();
}

void
MAC::do_disable_adr()
{
    LDL_MAC_disableADR(&mac);
    notify_api();
}

void
MAC::do_adr(bool *retval)
{
    *retval = LDL_MAC_adr(&mac);
    notify_api();
}

void
MAC::do_get_errno(enum ldl_mac_errno *retval)
{
    *retval = LDL_MAC_errno(&mac);
    notify_api();
}

void
MAC::do_joined(bool *retval)
{
    *retval = LDL_MAC_joined(&mac);
    notify_api();
}

void
MAC::do_ready(bool *retval)
{
    *retval = LDL_MAC_ready(&mac);
    notify_api();
}

void
MAC::do_get_op(enum ldl_mac_operation *retval)
{
    *retval = LDL_MAC_op(&mac);
    notify_api();
}

void
MAC::do_get_state(enum ldl_mac_state *retval)
{
    *retval = LDL_MAC_state(&mac);
    notify_api();
}

void
MAC::do_set_max_dcycle(uint8_t value)
{
    LDL_MAC_setMaxDCycle(&mac, value);
    notify_api();
}

void
MAC::do_get_max_dcycle(uint8_t *retval)
{
    *retval = LDL_MAC_getMaxDCycle(&mac);
    notify_api();
}

void
MAC::begin_api()
{
    mutex.lock();
    done = false;
}

void
MAC::wait_until_api_done()
{
    while(!done){

        api.acquire();
    }

    mutex.unlock();
}

void
MAC::notify_api()
{
    done = true;
    api.release();
}

/* public methods *****************************************************/

bool
MAC::start(enum ldl_region region)
{
    mutex.lock();

    if(run_state == ON){

        return false;
    }

    struct ldl_mac_init_arg arg = {0};
    
    uint8_t dev_eui[8];
    uint8_t join_eui[8];
    
    store.get_dev_eui(dev_eui);
    store.get_join_eui(join_eui);
    
    arg.app = this;
    arg.handler = app_handler;
    
    arg.radio = (struct ldl_radio *)(&radio);
    arg.radio_adapter = &radio.adapter;

    arg.sm = (struct ldl_sm *)(&sm);
    arg.sm_adapter = &sm.adapter;

    arg.devEUI = dev_eui;   
    arg.joinEUI = join_eui;

    radio.set_event_handler(callback(this, &MAC::handle_radio_event));
    
    LDL_MAC_init(&mac, region, &arg);
    
    /* apply TTN fair access policy 
     * 
     * ~30s per day
     * 
     * 30 / (60*60*24)  = 0.000347222
     * 
     * 1 / (2 ^ 11)     = 0.000488281
     * 1 / (2 ^ 12)     = 0.000244141
     * 
     * */
    LDL_MAC_setMaxDCycle(&mac, 12U);

    event_thread.start(callback(&events, &EventQueue::dispatch_forever));

    run_state = ON;

    mutex.unlock();

    return true;
}

void
MAC::stop()
{
    mutex.lock();

    events.break_dispatch();

    event_thread.join();

    run_state = OFF;

    mutex.unlock();
}

bool
MAC::unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_unconfirmed), &retval, port, data, len, opts);

    wait_until_api_done();

    return retval;
}

bool
MAC::confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts)
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_confirmed), &retval, port, data, len, opts);

    wait_until_api_done();

    return retval;
}

bool
MAC::otaa()
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_otaa), &retval);

    wait_until_api_done();

    return retval;
}

void
MAC::forget()
{
    begin_api();

    events.call(callback(this, &MAC::do_forget));

    wait_until_api_done();
}

bool
MAC::set_rate(uint8_t value)
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_set_rate), &retval, value);

    wait_until_api_done();
    
    return retval;
}

uint8_t
MAC::get_rate()
{
    uint8_t retval;

    begin_api();

    events.call(callback(this, &MAC::do_get_rate), &retval);

    wait_until_api_done();
    
    return retval;
}

bool
MAC::set_power(uint8_t value)
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_set_power), &retval, value);
    
    wait_until_api_done();
    
    return retval;
}

uint8_t
MAC::get_power()
{
    uint8_t retval;

    begin_api();

    events.call(callback(this, &MAC::do_get_power), &retval);
    
    wait_until_api_done();
    
    return retval;    
}

enum ldl_mac_errno
MAC::get_errno()
{
    enum ldl_mac_errno retval;

    begin_api();

    events.call(callback(this, &MAC::do_get_errno), &retval);
    
    wait_until_api_done();
    
    return retval;
}    

bool
MAC::joined()
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_joined), &retval);
    
    wait_until_api_done();
    
    return retval;
}

bool
MAC::ready()
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_ready), &retval);

    wait_until_api_done();
    
    return retval;
}

enum ldl_mac_operation
MAC::get_op()
{
    enum ldl_mac_operation retval;

    begin_api();

    events.call(callback(this, &MAC::do_get_op), &retval);
    
    wait_until_api_done();
    
    return retval;
}

enum ldl_mac_state
MAC::get_state()
{
    enum ldl_mac_state retval;

    begin_api();

    events.call(callback(this, &MAC::do_get_state), &retval);
    
    wait_until_api_done();
    
    return retval;    
}

void
MAC::enable_adr()
{
    begin_api();

    events.call(callback(this, &MAC::do_enable_adr));

    wait_until_api_done();
}

void MAC::disable_adr()
{
    begin_api();

    events.call(callback(this, &MAC::do_disable_adr));
    
    wait_until_api_done();
}

bool
MAC::adr()
{
    bool retval;

    begin_api();

    events.call(callback(this, &MAC::do_adr), &retval);
    
    wait_until_api_done();

    return retval;
}

void
MAC::set_max_dcycle(uint8_t value)
{
    begin_api();

    events.call(callback(this, &MAC::do_set_max_dcycle), value);
    
    wait_until_api_done();
}

uint8_t
MAC::get_max_dcycle()
{
    uint8_t retval;

    begin_api();

    events.call(callback(this, &MAC::do_get_max_dcycle), &retval);

    wait_until_api_done();

    return retval;    
}

uint32_t
MAC::ticks(void *self)
{
    return (uint32_t)timer.elapsed_time().count();
}

void
MAC::print_event(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    static const char *bw[] = {
        "125",
        "250",
        "500"
    };

    printf("[%" PRIu32 "]", (uint32_t)timer.elapsed_time().count());

    switch(type){
    case LDL_MAC_STARTUP:
        printf("STARTUP: ENTROPY=%u",
            arg->startup.entropy
        );
        break;
    case LDL_MAC_LINK_STATUS:
        printf("LINK_STATUS: M=%u GW=%u",
            arg->link_status.margin,
            arg->link_status.gwCount
        );
        break;
    case LDL_MAC_CHIP_ERROR:
        printf("CHIP_ERROR");
        break;            
    case LDL_MAC_RESET:
        printf("RESET\n");
        break;            
    case LDL_MAC_TX_BEGIN:
        printf("TX_BEGIN: SZ=%u F=%" PRIu32 " SF=%u BW=%s P=%u",
            arg->tx_begin.size,
            arg->tx_begin.freq,
            arg->tx_begin.sf,
            bw[arg->tx_begin.bw],
            arg->tx_begin.power
        );        
        break;
    case LDL_MAC_TX_COMPLETE:
        printf("TX_COMPLETE\n");
        break;
    case LDL_MAC_RX1_SLOT:
    case LDL_MAC_RX2_SLOT:
        printf("%s: F=%" PRIu32 " SF=%u BW=%s E=%" PRIu32 " M=%" PRIu32,
            (type == LDL_MAC_RX1_SLOT) ? "RX1_SLOT" : "RX2_SLOT",
            arg->rx_slot.freq,
            arg->rx_slot.sf,
            bw[arg->rx_slot.bw],
            arg->rx_slot.error,
            arg->rx_slot.margin
        );
        break;
    case LDL_MAC_DOWNSTREAM:
        printf("DOWNSTREAM: SZ=%u RSSI=%" PRIu16 " SNR=%" PRIu16,
            arg->downstream.size,
            arg->downstream.rssi,
            arg->downstream.snr
        );
        break;
    case LDL_MAC_JOIN_COMPLETE:
        printf("JOIN_COMPLETE: JN=%" PRIu32 " NDN=%" PRIu16 " NETID=%" PRIu32 " DEVADDR=%" PRIu32,
            arg->join_complete.joinNonce,
            arg->join_complete.nextDevNonce,
            arg->join_complete.netID,
            arg->join_complete.devAddr
        );
        break;
    case LDL_MAC_JOIN_TIMEOUT:
        printf("JOIN_TIMEOUT");
        break;
    case LDL_MAC_RX:
        printf("RX: PORT=%u COUNT=%" PRIu16 " SIZE=%u",
            arg->rx.port,
            arg->rx.counter,
            arg->rx.size
        );
        break;
    case LDL_MAC_DATA_COMPLETE:
        printf("DATA_COMPLETE");
        break;
    case LDL_MAC_DATA_TIMEOUT:
        printf("DATA_TIMEOUT");
        break;
    case LDL_MAC_DATA_NAK:
        printf("DATA_NAK");
        break;
    case LDL_MAC_SESSION_UPDATED:
        printf("SESSION_UPDATED");
        break;    
    case LDL_MAC_DEVICE_TIME:
        printf("DEVICE_TIME_ANS: SEC=%" PRIu32 " FRAC=%u",
            arg->device_time.seconds,
            arg->device_time.fractions
        );
        break;
    default:
        break;
    }

    printf("\n");
}
