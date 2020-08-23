#include "radio.h"

using namespace LDL;

/* constructors *******************************************************/

Radio::Radio(enum ldl_radio_type type, SPI &spi, PinName nreset, PinName nselect, PinName dio0, PinName dio1)
    :
    spi(spi),
    nreset(nreset),
    nselect(nselect),
    dio0(dio0),
    dio1(dio1)
{
    struct ldl_radio_init_arg arg = {
        .type = type,
        .chip = this,
        .chip_interface = &chip_interface        
    };

    LDL_Radio_init(&state, &arg);
    this->dio0.rise(callback(this, &Radio::dio0_handler));
    this->dio1.rise(callback(this, &Radio::dio1_handler));
    LDL_Radio_setHandler(&state, (struct ldl_mac *)this, &Radio::interrupt_handler);
}

/* static protected ***************************************************/

Radio *
Radio::to_obj(void *self)
{
    return static_cast<Radio *>(self);
}

struct ldl_radio *
Radio::to_state(void *self)
{
    return &to_obj(self)->state;
}

void
Radio::chip_reset(void *self, bool state)
{
    Radio *obj = to_obj(self);

    if(state){

        obj->nreset.output();
        obj->nreset = 0;
    }
    else{

        obj->nreset.input();
    }
}

void
Radio::chip_write(void *self, uint8_t addr, const void *data, uint8_t size)
{
    Radio *obj = to_obj(self);

    obj->chip_select(true);

    obj->spi.write(addr | 0x80U);
    obj->spi.write((const char *)data, size, nullptr, 0);

    obj->chip_select(false);
}

void
Radio::chip_read(void *self, uint8_t addr, void *data, uint8_t size)
{
    Radio *obj = to_obj(self);

    obj->chip_select(true);

    obj->spi.write(addr & 0x7eU);
    obj->spi.write(nullptr, 0, (char *)data, size);

    obj->chip_select(false);
}

void
Radio::interrupt_handler(void *self, enum ldl_radio_event event)
{
    if(to_obj(self)->event_cb){

        to_obj(self)->event_cb(event);
    }
}

void
Radio::entropy_begin(struct ldl_radio *self)
{
    LDL_Radio_entropyBegin(to_state(self));
}

unsigned int
Radio::entropy_end(struct ldl_radio *self)
{
    return LDL_Radio_entropyEnd(to_state(self));
}

void
Radio::reset(struct ldl_radio *self, bool state)
{
    LDL_Radio_reset(to_state(self), state);
}

uint8_t
Radio::collect(struct ldl_radio *self, struct ldl_radio_packet_metadata *meta, void *data, uint8_t max)
{
    return LDL_Radio_collect(to_state(self), meta, data, max);
}

void
Radio::sleep(struct ldl_radio *self)
{   
    LDL_Radio_sleep(to_state(self));
}

void
Radio::transmit(struct ldl_radio *self, const struct ldl_radio_tx_setting *settings, const void *data, uint8_t len)
{
    LDL_Radio_transmit(to_state(self), settings, data, len);
}

void
Radio::receive(struct ldl_radio *self, const struct ldl_radio_rx_setting *settings)
{
    LDL_Radio_receive(to_state(self), settings);
}

void
Radio::clear_interrupt(struct ldl_radio *self)
{
    LDL_Radio_clearInterrupt(to_state(self));
}

int16_t
Radio::min_snr(struct ldl_radio *self, enum ldl_spreading_factor sf)
{
    return LDL_Radio_minSNR(to_state(self), sf);
}

/* protected **********************************************************/

void
Radio::dio0_handler()
{
    LDL_Radio_interrupt(&state, 0);
}

void
Radio::dio1_handler()
{
    LDL_Radio_interrupt(&state, 1);
}

void
Radio::chip_select(bool state)
{
    if(state){

        if(nselect.is_connected() < 0){

            spi.select();
            spi.frequency();
            spi.format(8U, 0U);            
        }
        else{

            spi.lock();
            spi.frequency();
            spi.format(8U, 0U);            
            nselect = 0;
        }
    }
    else{

        if(nselect.is_connected() < 0){

            spi.deselect();            
        }
        else{

            nselect = 1;
            spi.unlock();
        }
    }
}

/* public *************************************************************/

void
Radio::set_pa(enum ldl_radio_pa setting)
{
    LDL_Radio_setPA(&state, setting);
}

void
Radio::set_event_handler(Callback<void(enum ldl_radio_event)> handler)
{
    event_cb = handler;
}
