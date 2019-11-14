#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"

#include <string.h>


#include "lora_radio.h"

void LDL_Radio_init(struct ldl_radio *self, enum ldl_radio_type type, void *board)
{    
}

void LDL_Radio_setPA(struct ldl_radio *self, enum ldl_radio_pa pa)
{
}

void LDL_Radio_reset(struct ldl_radio *self, bool state)
{
}

void LDL_Radio_transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
}

void LDL_Radio_receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
}

void LDL_Radio_sleep(struct ldl_radio *self)
{
}

void LDL_Radio_clearInterrupt(struct ldl_radio *self)
{
}

/* to setup:
 * 
 * will_return( <size of data> )
 * will_return( <pointer to data> )
 * 
 * */
uint8_t LDL_Radio_collect(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    uint8_t data_size = mock_type(uint8_t);    
    (void)memcpy(data, mock_ptr_type(uint8_t *), (data_size > max) ? max : data_size);
    return data_size;
}

enum ldl_radio_event LDL_Radio_signal(struct ldl_radio *self, uint8_t n)
{
    enum ldl_radio_event retval = LDL_RADIO_NONE;
    
    switch(n){
    case (uint8_t)LDL_RADIO_EVENT_RX_READY:
        retval = LDL_RADIO_EVENT_RX_READY;
        break;
    case (uint8_t)LDL_RADIO_EVENT_RX_TIMEOUT:
        retval = LDL_RADIO_EVENT_RX_TIMEOUT;
        break;    
    case (uint8_t)LDL_RADIO_EVENT_TX_COMPLETE:
        retval = LDL_RADIO_EVENT_TX_COMPLETE;
        break;    
    default:
        break;
    }
    
    return retval;
}

void LDL_Radio_entropyBegin(struct ldl_radio *self)
{    
}

unsigned int LDL_Radio_entropyEnd(struct ldl_radio *self)
{
    return 0U;
}
