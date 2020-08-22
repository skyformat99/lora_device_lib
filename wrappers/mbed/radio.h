#ifndef MBED_LDL_RADIO_H
#define MBED_LDL_RADIO_H

#include "mbed.h"
#include "ldl_radio.h"

namespace LDL {

    class Radio {

        protected:

            SPI &spi;
            DigitalInOut nreset;
            InterruptIn dio0;
            InterruptIn dio1;                                    
            Callback<void(enum ldl_radio_event)> event_cb;

            struct ldl_radio state;

            static Radio *to_obj(void *self);            
            static struct ldl_radio *to_state(void *self);            
            static void entropy_begin(struct ldl_radio *self);
            static unsigned int entropy_end(struct ldl_radio *self);
            static void reset(struct ldl_radio *self, bool state);
            static uint8_t collect(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max);
            static void sleep(struct ldl_radio *self);
            static void transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len);
            static void receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings);
            static void clear_interrupt(struct ldl_radio *self);
            static int16_t min_snr(struct ldl_radio *self, enum ldl_spreading_factor sf);

            static void interrupt_handler(void *self, enum ldl_radio_event event);

            void dio0_handler();
            void dio1_handler();

            static void chip_reset(void *self, bool state);
            static void chip_write(void *self, uint8_t addr, const void *data, uint8_t size);
            static void chip_read(void *self, uint8_t addr, void *data, uint8_t size);

            const struct ldl_chip_adapter chip_adapter = {

                .reset = chip_reset,
                .write = chip_write,
                .read = chip_read
            };

        public:

            const struct ldl_radio_adapter adapter = {
                .entropy_begin = entropy_begin,
                .entropy_end = entropy_end,
                .reset = reset,
                .collect = collect,
                .sleep = sleep,
                .transmit = transmit,
                .receive = receive,
                .clear_interrupt = clear_interrupt,
                .min_snr = min_snr 
            };

            Radio(enum ldl_radio_type type, SPI &spi, PinName nreset, PinName dio0, PinName dio1);
              
            void set_pa(enum ldl_radio_pa setting);
            void set_event_handler(Callback<void(enum ldl_radio_event)> handler);
    };

    class SX1272 : public Radio {
        
        public:

            SX1272(SPI &spi, PinName nreset, PinName dio0, PinName dio1)
                : Radio(LDL_RADIO_SX1272, spi, nreset, dio0, dio1)        
            {}
    };
    
    class SX1276 : public Radio {

        public:

            SX1276(SPI &spi, PinName nreset, PinName dio0, PinName dio1)
                : Radio(LDL_RADIO_SX1276, spi, nreset, dio0, dio1)        
            {}            
    };
};

#endif
