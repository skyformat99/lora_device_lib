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

struct lora_block {
    
    uint8_t value[16U];
};


void LDL_OPS_deriveKeys(struct lora_mac *self, uint32_t joinNonce, uint32_t netID, uint16_t devNonce);
void LDL_OPS_deriveKeys2(struct lora_mac *self, uint32_t joinNonce, const uint8_t *joinEUI, uint16_t devNonce);

bool LDL_OPS_receiveFrame(struct lora_mac *self, struct lora_frame_down *f, uint8_t *in, uint8_t len);

void LDL_OPS_encryptData(struct lora_mac *self, void *buf, uint8_t buflen);
void LDL_OPS_decryptData(struct lora_mac *self, struct lora_frame_down *f);

uint32_t LDL_OPS_micDataUp(struct lora_mac *self, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len);
uint32_t LDL_OPS_micDataUp2(struct lora_mac *self, uint16_t downCounter, uint8_t rate, uint8_t chIndex, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len);

uint32_t LDL_OPS_micDataDown(struct lora_mac *self, uint32_t devAddr, uint32_t downCounter, const void *data, uint8_t len);
uint32_t LDL_OPS_micDataDown2(struct lora_mac *self, uint16_t downCounter, uint32_t devAddr, uint32_t upCounter, const void *data, uint8_t len);

uint32_t LDL_OPS_micJoinAccept(struct lora_mac *self, const void *joinAccept, uint8_t len);
uint32_t LDL_OPS_micJoinAccept2(struct lora_mac *self, uint8_t joinReqType, const uint8_t *joinEUI, uint16_t devNonce, const void *joinAccept, uint8_t len);

uint32_t LDL_OPS_micJoinRequest(struct lora_mac *self, const void *joinRequest, uint8_t len);

uint32_t LDL_OPS_deriveDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter);
void LDL_OPS_syncDownCounter(struct lora_mac *self, uint8_t port, uint16_t counter);

#endif
