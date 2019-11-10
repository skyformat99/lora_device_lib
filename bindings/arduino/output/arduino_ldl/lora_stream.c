/* Copyright (c) 2019 Cameron Harper
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */

#include "lora_stream.h"
#include "lora_debug.h"

#include <string.h>

struct lora_stream * LDL_Stream_init(struct lora_stream *self, void *buf, uint8_t size)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC((buf != NULL) || (size == 0U))
    
    self->write = buf;
    self->read = buf;
    self->size = size;
    self->pos = 0U;
    self->error = false;
    
    return self;
}

struct lora_stream * LDL_Stream_initReadOnly(struct lora_stream *self, const void *buf, uint8_t size)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC((buf != NULL) || (size == 0U))
    
    self->write = NULL;
    self->read = buf;
    self->size = size;
    self->pos = 0U;
    self->error = false;
    
    return self;
}

bool LDL_Stream_read(struct lora_stream *self, void *buf, uint8_t count)
{
    LORA_PEDANTIC(self != NULL)
    LORA_PEDANTIC(buf != NULL)
    
    bool retval = false;
    
    if(!self->error){
    
        if((self->size - self->pos) >= count){

            (void)memcpy(buf, &self->read[self->pos], count);
            self->pos += count;
            retval = true;
        }    
        else{
            
            self->error = true;
        }
    }
    
    return retval;
}

bool LDL_Stream_write(struct lora_stream *self, const void *buf, uint8_t count)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    
    if(self->write != NULL){
        
        if(!self->error){
        
            if((self->size - self->pos) >= count){
                
                (void)memcpy(&self->write[self->pos], buf, count);
                self->pos += count;
                retval = true;
            }
            else{
                
                self->error = true;
            }
        }
    }
    
    return retval;
}

uint8_t LDL_Stream_tell(const struct lora_stream *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->pos;
}

uint8_t LDL_Stream_remaining(const struct lora_stream *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return (self->size - self->pos);
}

bool LDL_Stream_peek(const struct lora_stream *self, void *out)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    
    if(LDL_Stream_remaining(self) > 0U){
        
        (void)memcpy(out, &self->read[self->pos], 1U);
        retval = true;
    }
    
    return retval;
}

bool LDL_Stream_seekSet(struct lora_stream *self, uint8_t offset)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;

    if(self->size >= offset){

        self->pos = offset;        
        retval = true;
    }    

    return retval;
}

bool LDL_Stream_seekCur(struct lora_stream *self, int16_t offset)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    
    int32_t pos = ((int32_t)self->pos) + ((int32_t)offset);
    int32_t size = self->size;

    if((pos >= 0) && (pos <= size)){

        if(offset >= 0){
            
            self->pos += (uint8_t)offset;   
        }
        else{
            
            self->pos -= (uint8_t)offset; 
        }

        retval = true;
    }

    return retval;
}

bool LDL_Stream_putU8(struct lora_stream *self, uint8_t value)
{
    LORA_PEDANTIC(self != NULL)
    
    return LDL_Stream_write(self, &value, sizeof(value));
}

bool LDL_Stream_putU16(struct lora_stream *self, uint16_t value)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t out[] = {
        value, 
        value >> 8        
    };
    
    return LDL_Stream_write(self, out, sizeof(out));
}

bool LDL_Stream_putU24(struct lora_stream *self, uint32_t value)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t out[] = {
        value, 
        value >> 8,        
        value >> 16        
    };
    
    return LDL_Stream_write(self, out, sizeof(out));
}

bool LDL_Stream_putU32(struct lora_stream *self, uint32_t value)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t out[] = {
        value, 
        value >> 8,        
        value >> 16,        
        value >> 24        
    };
    
    return LDL_Stream_write(self, out, sizeof(out));
}

bool LDL_Stream_putEUI(struct lora_stream *self, const uint8_t *value)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t out[] = {
        value[7],
        value[6],
        value[5],
        value[4],
        value[3],
        value[2],
        value[1],
        value[0],
    };
    
    return LDL_Stream_write(self, out, sizeof(out));    
}

bool LDL_Stream_getU8(struct lora_stream *self, uint8_t *value)
{
    LORA_PEDANTIC(self != NULL)
    
    return LDL_Stream_read(self, value, sizeof(*value));
}

bool LDL_Stream_getU16(struct lora_stream *self, uint16_t *value)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t buf[2U];
    bool retval;
    
    *value = 0U;
    
    retval = LDL_Stream_read(self, buf, sizeof(buf));
        
    *value |= buf[1];
    *value <<= 8;
    *value |= buf[0];
        
    return retval;
}

bool LDL_Stream_getU24(struct lora_stream *self, uint32_t *value)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t buf[3U];
    bool retval;
    
    *value = 0U;
    
    retval = LDL_Stream_read(self, buf, sizeof(buf));
        
    *value |= buf[2];
    *value <<= 8;
    *value |= buf[1];
    *value <<= 8;
    *value |= buf[0];
    
    return retval;
}

bool LDL_Stream_getU32(struct lora_stream *self, uint32_t *value)
{
    LORA_PEDANTIC(self != NULL)
    
    uint8_t buf[4U];
    bool retval;
    
    *value = 0U;
    
    retval = LDL_Stream_read(self, buf, sizeof(buf));
        
    *value |= buf[3];
    *value <<= 8;
    *value |= buf[2];
    *value <<= 8;
    *value |= buf[1];
    *value <<= 8;
    *value |= buf[0];
    
    return retval;
}

bool LDL_Stream_getEUI(struct lora_stream *self, uint8_t *value)
{
    LORA_PEDANTIC(self != NULL)
    
    bool retval = false;
    uint8_t out[8];
    
    if(LDL_Stream_read(self, out, sizeof(out))){
        
        value[0] = out[7];
        value[1] = out[6];
        value[2] = out[5];
        value[3] = out[4];
        value[4] = out[3];
        value[5] = out[2];
        value[6] = out[1];
        value[7] = out[0];
    
        retval = true;
    }
    
    return retval;        
}

bool LDL_Stream_error(struct lora_stream *self)
{
    LORA_PEDANTIC(self != NULL)
    
    return self->error;
}
