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

SPI spi(D11, D12, D13);
LDL::SX1272 radio(spi, A0, D10, D2, D3);
LDL::DefaultSM sm(app_key, nwk_key);
LDL::DefaultStore store(dev_eui, join_eui);
LDL::MAC mac(store, sm, radio);

void handle_entropy(unsigned int value)
{
    srand(value);
}

int main()
{
    radio.set_pa(LDL_RADIO_PA_RFO);

    /* print all the events to the terminal */
    mac.set_event_cb(callback(&mac, &LDL::MAC::print_event));

    mac.set_entropy_cb(callback(handle_entropy));
    
    mac.start(LDL_EU_863_870);

    for(;;){

        if(mac.ready()){

            mac.otaa();

            while(mac.joined()){    

                const char msg[] = "hello world";

                mac.unconfirmed(1, msg, strlen(msg));

                ThisThread::sleep_for(5000);
            }
        }

        ThisThread::sleep_for(5000);
    }
    
    return 0;
}
