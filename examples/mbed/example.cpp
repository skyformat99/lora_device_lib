/* This is an example of the RTOS version of LDL::MAC in operation.
 *
 * The RTOS version runs LDL inside it's own thread and provides
 * thread-safe interfaces. This will appeal to users with more than 20K
 * of RAM. This will still work with devices that have 20K of RAM, but only
 * just.
 *
 * */

#include "mbed_ldl.h"

/* you will need to change these values
 *
 * - keys must be 16 bytes
 * - EUIs must be 8 bytes
 * 
 * */
const uint8_t app_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const uint8_t nwk_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const uint8_t dev_eui[] = {0,0,0,0,0,0,0,1};
const uint8_t join_eui[] = {0,0,0,0,0,0,0,2};

/* these may need to change according to your pin mapping and
 * radio chip */
SPI spi(D11, D12, D13);
LDL::SX1272 radio(spi, A0, D10, D2, D3);

LDL::DefaultSM sm(app_key, nwk_key);
LDL::DefaultStore store(dev_eui, join_eui);
LDL::MAC mac(store, sm, radio);

int main()
{
    radio.set_pa(LDL_RADIO_PA_RFO);

    /* print all the events to the terminal */
    mac.set_event_cb(callback(&mac, &LDL::MAC::print_event));

    mac.start(LDL_EU_863_870);

    for(;;){

        if(mac.ready()){

            if(mac.joined()){

                const char msg[] = "hello world";

                mac.unconfirmed(1, msg, strlen(msg));
            }
            else{
            
                mac.otaa();
            }
        }
        
        ThisThread::sleep_for(1000);
    }
    
    return 0;
}
