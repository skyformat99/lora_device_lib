/** @file */

#include <stdlib.h>
#include "lora_radio.h"
#include "lora_mac.h"

/* state for the radio and MAC layer */
struct lora_radio radio;
struct lora_mac mac;

/* set a timer event that will ensure a wakeup so many ticks in future */
extern void wakeup_after(uint32_t ticks);

/* activate sleep mode */
extern void sleep(void);

/* This function will be called when the radio signals an interrupt. 
 * How this happens is platform specific. 
 * 
 * */
void handle_radio_interrupt(uint8_t n)
{
    /* safe to call from interrupt or mainloop */
    LDL_MAC_interrupt(&mac, n, LDL_System_ticks(&mac));
}

/* This will be called from within LDL_MAC_process() to pass events back to
 * the application.
 * 
 * */
void app_handler(void *app, enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
    switch(type){
        
    /* LoRaWAN needs a little bit of random for correct operation. 
     * 
     * Targets that have no better source of entropy will use this
     * event to seed the stdlib random generator.
     * 
     * */
    case LORA_MAC_STARTUP:
    
        srand(arg->startup.entropy);
        break;
    
    
    /* this is downstream data */
    case LORA_MAC_RX:    
        /* do something */
        break;
        
    /* Ignore all other events */
    case LORA_MAC_CHIP_ERROR:
    case LORA_MAC_RESET:
    case LORA_MAC_JOIN_COMPLETE:
    case LORA_MAC_JOIN_TIMEOUT:
    case LORA_MAC_DATA_COMPLETE:
    case LORA_MAC_DATA_TIMEOUT:
    case LORA_MAC_DATA_NAK:
    case LORA_MAC_LINK_STATUS:
    case LORA_MAC_RX1_SLOT:
    case LORA_MAC_RX2_SLOT:
    case LORA_MAC_TX_COMPLETE:
    case LORA_MAC_TX_BEGIN:
    default:
        break;    
    }
}

int main(void)
{
    /* To initialise the radio driver you need:
     * 
     * - radio type
     * - optional board specific state
     *
     * */ 
    LDL_Radio_init(&radio, LORA_RADIO_SX1272, NULL);
    
    /* This radio has two power amplifiers. The amplifier in use
     * depends on the hardware (i.e. which pin the PCB traces connect).
     * 
     * You have to tell the driver which amplifier is connected:
     * 
     * - The Semtech MBED SX1272 shield uses LORA_RADIO_PA_RFO
     * - The HopeRF RFM95 SX1276 module uses LORA_RADIO_PA_BOOST
     * 
     * */
    LDL_Radio_setPA(&radio, LORA_RADIO_PA_RFO);

    /* To initialise the mac layer you need:
     * 
     * - initialised radio
     * - region code
     * - application callback
     * 
     * */
    LDL_MAC_init(&mac, NULL, EU_863_870, &radio, app_handler);

    /* 
     * - wait until MAC is ready to send
     * - if not joined, initiate the join
     * - if joined, send a message
     * - run the MAC by polling LDL_MAC_process()
     * - sleep until next MAC event
     * 
     * */
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
        
        /* safe to call from interrupt or mainloop */
        uint32_t ticks_until_next_event = LDL_MAC_ticksUntilNextEvent(&mac);
        
        if(ticks_until_next_event > 0UL){
            
            wakeup_after(ticks_until_next_event);
            sleep();
        }
    }
}
