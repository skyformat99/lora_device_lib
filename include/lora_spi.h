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

#ifndef LORA_SPI_H
#define LORA_SPI_H

/** @file */

/**
 * @defgroup ldl_radio_connector Radio Connector
 * @ingroup ldl
 * @ingroup ldl_radio_connector
 * 
 * The Radio Connector connects the @ref ldl_radio to the radio chip.
 * 
 * The following interfaces MUST be implemented:
 * 
 * - LDL_SPI_read()
 * - LDL_SPI_write()
 * - LDL_SPI_select()
 * - LDL_SPI_reset()
 * 
 * The following interface MUST be called when the radio signals an interrupt:
 * 
 * - LDL_MAC_interrupt()
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include "lora_platform.h"
#include <stdint.h>
#include <stdbool.h>

/** Operate select line
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] state     **true** for hold, **false** for release
 * 
 * */
void LDL_SPI_select(void *self, bool state);

/** Operate reset line
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] state     **true** for hold, **true** for release
 * 
 * */
void LDL_SPI_reset(void *self, bool state);

/** Write byte
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] data
 * 
 * */
void LDL_SPI_write(void *self, uint8_t data);

/** Read byte
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @return data
 * 
 * */
uint8_t LDL_SPI_read(void *self);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
