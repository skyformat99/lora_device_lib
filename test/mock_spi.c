#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include "cmocka.h"
#include "lora_radio.h"
#include "mock_board.h"
#include "lora_spi.h"

void LDL_SPI_select(void *self, bool state)
{
    struct mock_chip *_self = (struct mock_chip *)self;
    
    if(state){
        
        assert_false(_self->select);  //double select        
    }
    else{
        
        assert_true(_self->select);  //double select        
    }
    
    check_expected(state);
    
    _self->select = state;    
}

void LDL_SPI_reset(void *self, bool state)
{
    struct mock_chip *_self = (struct mock_chip *)_self;
    
    if(state){
        
        assert_false(_self->reset);  //double reset        
    }
    else{
        
        assert_true(_self->reset);  //double reset        
    }
    
    check_expected(state);
    
    _self->reset = state;    
}

void LDL_SPI_write(void *self, uint8_t data)
{
    struct mock_chip *_self = (struct mock_chip *)self;
    
    assert_true(_self->select);
    assert_false(_self->reset);    
    
    check_expected(data);
}

uint8_t LDL_SPI_read(void *self)
{
    struct mock_chip *_self = (struct mock_chip *)self;
    
    assert_true(_self->select);
    assert_false(_self->reset);
    
    return mock();    
}