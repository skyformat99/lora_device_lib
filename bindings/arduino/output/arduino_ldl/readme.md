arduino_ldl
===========

An Arduino library that wraps [LoraDeviceLib](https://github.com/cjhdev/lora_device_lib).

## Supported Targets

- ATMEGA328p based boards with ceramic resonators and crystal oscillators

## Optimisation

ArduinoLDL can be optimised and adapted by changing the definitions in [platform.h](platform.h).

## Interface

See [arduino_ldl.h](arduino_ldl.h).

Note that `DEBUG_LEVEL` is optional.

- `DEBUG_LEVEL 1` will print summary status information to Serial
- `DEBUG_LEVEL 2` will print all status information to Serial

Reset, select, and dio pins can be moved to suit your hardware. All
of these connections are required for correct operation.

## Examples

Send an empty data message as often as the TTN fair access policy allows:

~~~
#include <arduino_ldl.h>

static void get_identity(struct lora_system_identity *id)
{       
    static const struct lora_system_identity _id = {
        .appEUI = {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U},
        .devEUI = {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x01U},
        .appKey = {0x2bU,0x7eU,0x15U,0x16U,0x28U,0xaeU,0xd2U,0xa6U,0xabU,0xf7U,0x15U,0x88U,0x09U,0xcfU,0x4fU,0x3cU}
    };

    memcpy(id, &_id, sizeof(*id));
}

ArduinoLDL& get_ldl()
{
    static ArduinoLDL ldl(
        get_identity,       /* specify name of function that returns euis and key */
        EU_863_870,         /* specify region */
        LORA_RADIO_SX1272,  /* specify radio */    
        LORA_RADIO_PA_RFO,  /* specify radio power amplifier */
        A0,                 /* radio reset pin */
        10,                 /* radio select pin */
        2,                  /* radio dio0 pin */
        3                   /* radio dio1 pin */
    );
    
    return ldl;
}

void setup() 
{
    ArduinoLDL& ldl = get_ldl();
}

void loop() 
{ 
    ArduinoLDL& ldl = get_ldl();
    
    if(ldl.ready()){
    
        if(ldl.joined()){
        
            ldl.unconfirmedData(1U, NULL, 0U);                 
        }
        else{
         
            ldl.otaa();
        }
    }    
    
    ldl.process();        
}
~~~

More [examples](examples).

## License

MIT for the wrapper, example sketches, and core LoraDeviceLib code.

Some of the examples depend on third-party libraries. These libraries are 
kept in separate folders and have their own license T&Cs.
