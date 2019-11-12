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

#ifndef __LORA_OPS_H
#define __LORA_OPS_H

#include <stdint.h>
#include <stdbool.h>

struct lora_mac;
struct lora_frame_data;
struct lora_frame_down;
struct lora_frame_join_request;

struct lora_block {
    
    uint8_t value[16U];
};

/* derive all session keys and write to lora_sm */
void LDL_OPS_deriveKeys(struct lora_mac *self, uint32_t joinNonce, uint32_t netID, uint16_t devNonce);
void LDL_OPS_deriveKeys2(struct lora_mac *self, uint32_t joinNonce, const uint8_t *joinEUI, const uint8_t *devEUI, uint16_t devNonce);

/* decode and verify a frame (does not update any lora_mac state) */
bool LDL_OPS_receiveFrame(struct lora_mac *self, struct lora_frame_down *f, uint8_t *in, uint8_t len);

/* encode a frame */
uint8_t LDL_OPS_prepareData(struct lora_mac *self, const struct lora_frame_data *f, uint8_t *out, uint8_t max);
uint8_t LDL_OPS_prepareJoinRequest(struct lora_mac *self, const struct lora_frame_join_request *f, uint8_t *out, uint8_t max);

/* derive expected 32 bit downcounter from 16 least significant bits and update the copy in lora_mac */
void LDL_OPS_syncDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter);

#endif
