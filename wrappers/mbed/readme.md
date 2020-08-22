LDL MBED Wrapper
================

This wrapper exists for quick evaluation and regression testing. 

It's more compact and composable than the native MBED lorawan feature.
At time of writing it requires the multithreading profile, since these features
are part of what makes this wrapper easy to use. 

Usage:

~~~ c++

#include "mbed_ldl.h"

const uint8_t app_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const uint8_t nwk_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const uint8_t dev_eui[] = {0,0,0,0,0,0,0,1};
const uint8_t join_eui[] = {0,0,0,0,0,0,0,2};

SPI spi(D11, D12, D13, D10);
LDL::SX1272 radio(spi, A0, D2, D3);
LDL::DefaultSM sm(app_key, nwk_key);
LDL::DefaultStore store(dev_eui, join_eui);
LDL::MAC mac(store, sm, radio);

int main()
{
    radio.set_pa(LDL_RADIO_PA_RFO);

    mac.start(LDL_EU_863_870);

    while(!mac.ready());

    mac.otaa();

    for(;;){

        const char msg[] = "hello world";

        mac.unconfirmed(1, msg, sizeof(msg));

        ThisThread::sleep_for(5000);
    }

    return 0;
}
~~~
