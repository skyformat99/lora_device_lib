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

#ifndef LDL_CHIP_H
#define LDL_CHIP_H

/** @file */

/**
 * @defgroup ldl_chip_interface Chip Interface
 * @ingroup ldl
 * 
 * # Chip Interface
 *
 * The Radio driver uses the members of ldl_chip_interface to control
 * the transceiver.
 *
 * ## SX1272 and SX1276
 * 
 * The following connections are required:
 * 
 * | signal | direction    | type                    | polarity    |
 * |--------|--------------|-------------------------|-------------|
 * | MOSI   | input        | hiz                     |             |
 * | MISO   | output       | push-pull/hiz           |             |
 * | SCK    | input        | hiz                     |             |
 * | NSS    | input        | hiz                     | active-low  |
 * | Reset  | input/output | open-collector + pullup | active-low  |
 * | DIO0   | output       | push-pull               | active-high |
 * | DIO1   | output       | push-pull               | active-high | 
 * 
 * - Direction is from perspective of transceiver
 * - SPI mode is CPOL=0 and CPHA=0
 * - consider adding pullup to MISO to prevent floating when not selected
 *
 * ### ldl_chip_interface.reset
 *
 * Should manipulate the chip reset line like this:
 *
 * @code{.c}
 * void chip_reset(void *self, bool state)
 * {
 *      if(state){
 * 
 *          // pull down
 *      }
 *      else{
 * 
 *          // hiz
 *      }
 * }
 * @endcode
 *
 * ### ldl_chip_interface.read
 *
 * Should manipulate the chip select line and SPI like this:
 *
 * @include examples/chip_interface/read_example.c
 * 
 * ### ldl_chip_interface.write
 *
 * Should manipulate the chip select line and SPI like this:
 *
 * @include examples/chip_interface/write_example.c
 *
 * ### LDL_Radio_interrupt()
 *
 * The chip interface code needs to be able to detect the rising edge and call
 * LDL_Radio_interrupt().
 *
 * The code might look like this:
 * 
 * @code{.c}
 * extern ldl_radio radio;
 * 
 * void dio0_rising_edge_isr(void)
 * {
 *   LDL_Radio_interrupt(&radio, 0);
 * }
 * void dio1_rising_edge_isr(void)
 * {
 *   LDL_Radio_interrupt(&radio, 1);
 * }
 * @endcode
 *
 * Note that LDL_SYSTEM_ENTER_CRITICAL() and LDL_SYSTEM_LEAVE_CRITICAL() must be defined
 * if LDL_Radio_interrupt() is called from an ISR.
 * 
 * @{
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** Operate reset line
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] state     **true** for hold, **true** for release
 * 
 * */
typedef void (*ldl_chip_reset_fn)(void *self, bool state);

/** Write bytes to address
 * 
 * @param[in] self      chip from LDL_Radio_init()
 * @param[in] addr      register address
 * @param[in] data      buffer to write
 * @param[in] size      size of buffer in bytes
 * 
 * This function must handle chip selection, addressing, and transferring zero
 * or more bytes. For example:
 * 
 * @include examples/chip_interface/write_example.c
 * 
 * */
typedef void (*ldl_chip_write_fn)(void *self, uint8_t addr, const void *data, uint8_t size);

/** Read bytes from address
 * 
 * @param[in] self      board from LDL_Radio_init()
 * @param[in] addr      register address
 * @param[out] data     read into this buffer
 * @param[in] size      number of bytes to read (and size of buffer in bytes)
 * 
 * This function must handle chip selection, addressing, and transferring zero
 * or more bytes. For example:
 * 
 * @include examples/chip_interface/read_example.c
 * 
 * */
typedef void (*ldl_chip_read_fn)(void *self, uint8_t addr, void *data, uint8_t size);

/** This struct connects the radio to the transceiver
 *
 * A const pointer to this struct must be passed to @ref ldl_radio
 * as a member of #ldl_radio_init_arg at LDL_Radio_init().
 *
 * */
struct ldl_chip_interface {

    ldl_chip_reset_fn reset;    /**< #ldl_chip_reset_fn */
    ldl_chip_write_fn write;    /**< #ldl_chip_write_fn */
    ldl_chip_read_fn read;      /**< #ldl_chip_read_fn */
};

#ifdef __cplusplus
}
#endif

/** @} */
#endif
