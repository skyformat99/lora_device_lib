/** @file */

#include <stdlib.h>
#include "ldl_radio.h"
#include "ldl_mac.h"
#include "ldl_sm.h"

struct ldl_radio radio;
struct ldl_mac mac;
struct ldl_sm sm;

/* a pointer to be passed back to the application (anything you like) */
void *app_pointer;

/* a pointer to be passed to the SPI module (anything you like) */
void *radio_connector_pointer;

/* somehow set a timer event that will ensure a wakeup so many ticks in future */
extern void wakeup_after(uint32_t ticks);

/* somehow activate sleep mode */
extern void sleep(void);

/* somehow enable interrupts */
extern void enable_interrupts(void);

/* somehow connect DIOx interrupt lines back to the radio driver */
void handle_radio_interrupt_dio0(void)
{
    LDL_MAC_interrupt(&mac, 0);
}
void handle_radio_interrupt_dio1(void)
{
    LDL_MAC_interrupt(&mac, 1);
}
void handle_radio_interrupt_dio2(void)
{
    LDL_MAC_interrupt(&mac, 2);
}
void handle_radio_interrupt_dio3(void)
{
    LDL_MAC_interrupt(&mac, 3);
}

/* Called from within LDL_MAC_process() to pass events back to the application */
void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

int main(void)
{
    LDL_Radio_init(&radio, LDL_RADIO_SX1272, radio_connector_pointer);
    
    /* This radio has two power amplifiers. The amplifier in use
     * depends on the hardware (i.e. which pin the PCB traces connect).
     * 
     * You have to tell the driver which amplifier is connected:
     * 
     * - The Semtech MBED SX1272 shield uses LDL_RADIO_PA_RFO
     * - The HopeRF RFM95 SX1276 module uses LDL_RADIO_PA_BOOST
     * 
     * */
    LDL_Radio_setPA(&radio, LDL_RADIO_PA_RFO);
    
    
    
    struct ldl_mac_init_arg arg = {
        
        .radio = &radio,
        .handler = app_handler,
        .app = NULL, 
        .sm = &sm
    };

    LDL_MAC_init(&mac, EU_863_870, &arg);

    /* Ensure a maximum aggregated duty cycle of ~1%
     * 
     * EU_863_870 already includes duty cycle limits. This is to safeguard
     * the example if the region is changed to US_902_928 or AU_915_928.
     * 
     * Aggregated Duty Cycle Limit = 1 / (2 ^ setting)
     * 
     * */
    LDL_MAC_setMaxDCycle(&mac, 7);

    enable_interrupts();
    
    for(;;){
        
        if(LDL_MAC_ready(&mac)){
           
            if(LDL_MAC_joined(&mac)){
                
                const char msg[] = "hello world";                
                                
                /* final argument is NULL since we don't have any specific invocation options */
                (void)LDL_MAC_unconfirmedData(&mac, 1, msg, sizeof(msg), NULL);
            }
            else{
                
                (void)LDL_MAC_otaa(&mac);
            }            
        }
        
        LDL_MAC_process(&mac);        
        
        uint32_t ticks_until_next_event = LDL_MAC_ticksUntilNextEvent(&mac);
        
        if(ticks_until_next_event > 0UL){
            
            wakeup_after(ticks_until_next_event);
            sleep();
        }
    }
}

void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg)
{
    switch(type){
        
    /* LoRaWAN needs a little bit of random for correct operation. 
     * 
     * Targets that have no better source of entropy will use this
     * event to seed the stdlib random generator.
     * 
     * */
    case LDL_MAC_STARTUP:
    
        srand(arg->startup.entropy);
        break;
    
    
    /* this is downstream data */
    case LDL_MAC_RX:    
        /* do something */
        break;
        
    case LDL_MAC_CHIP_ERROR:
    case LDL_MAC_RESET:
    case LDL_MAC_JOIN_COMPLETE:
    case LDL_MAC_JOIN_TIMEOUT:
    case LDL_MAC_DATA_COMPLETE:
    case LDL_MAC_DATA_TIMEOUT:
    case LDL_MAC_DATA_NAK:
    case LDL_MAC_LINK_STATUS:
    case LDL_MAC_RX1_SLOT:
    case LDL_MAC_RX2_SLOT:
    case LDL_MAC_TX_COMPLETE:
    case LDL_MAC_TX_BEGIN:
    default:
        break;    
    }
}
