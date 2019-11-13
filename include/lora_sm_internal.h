#ifndef __LORA_SM_INTERNAL_H
#define __LORA_SM_INTERNAL_H

/** @file */

/**
 * @defgroup ldl_tsm Security Module
 * @ingroup ldl
 * 
 * # Security Module Interface
 * 
 * The Security Module manages keys and performs cryptographic operations.
 * 
 * @{
 * 
 * */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** SM state */
struct lora_sm;

/** SM key descriptor */
enum lora_sm_key {
    
    LORA_SM_KEY_FNWKSINT,   /**< FNwkSInt forwarding/uplink (also used as NwkSKey) */
    LORA_SM_KEY_APPS,       /**< AppSKey */
    LORA_SM_KEY_SNWKSINT,   /**< SNwkSInt serving/downlink */
    LORA_SM_KEY_NWKSENC,    /**< NwkSEnc */
    
    LORA_SM_KEY_JSENC,      /**< JSEncKey */
    LORA_SM_KEY_JSINT,      /**< JSIntKey */
    
    LORA_SM_KEY_APP,        /**< application root key */
    LORA_SM_KEY_NWK         /**< network root key */        
};

/** Called from LDL_MAC_init() to either initiate restore or 
 * confirm that previous session keys are available.
 * 
 * @param[in] self
 * 
 * @retval true     keys have been restored
 * @retval false    keys have not been restored
 * 
 * The following weak implementation is provided:
 * 
 * @snippet src/lora_sm.c LDL_SM_restore
 * 
 * */
bool LDL_SM_restore(struct lora_sm *self);

/** Update a session key and save the result in the key store
 * 
 * LoRaWAN session keys are derived from clear text encrypted with a 
 * root key. 
 * 
 * @param[in] self
 * @param[in] keyDesc   #lora_sm_key the key to update
 * @param[in] rootDesc  #lora_sm_key the key to use as root key in derivation
 * @param[in] iv        16B of text used to derive key
 * 
 * The following weak implementation is provided:
 * 
 * @snippet src/lora_sm.c LDL_SM_updateSessionKey
 * 
 * */
void LDL_SM_updateSessionKey(struct lora_sm *self, enum lora_sm_key keyDesc, enum lora_sm_key rootDesc, const void *iv);

/** Signal the beginning of session key update transaction
 * 
 * SM implementations that perform batch updates can use
 * this signal to initialise a cache prior to receiving multiple 
 * LDL_SM_updateSessionKey() calls.
 * 
 * @param[in] self
 * 
 * 
 * The following weak implementation is provided:
 * 
 * @snippet src/lora_sm.c LDL_SM_beginUpdateSessionKey
 * 
 * */
void LDL_SM_beginUpdateSessionKey(struct lora_sm *self);

/** Signal the end session key update transaction
 * 
 * Always follows a previous call to LDL_SM_beginUpdateSessionKey().
 * 
 * SM implementations that perform batch updates can use
 * this signal to perform the actual update operation on the cached
 * key material.
 * 
 * @param[in] self
 * 
 * 
 * The following weak implementation is provided:
 * 
 * @snippet src/lora_sm.c LDL_SM_endUpdateSessionKey
 * 
 * */
void LDL_SM_endUpdateSessionKey(struct lora_sm *self);

/** Lookup a key and use it to produce a MIC
 * 
 * The MIC is the four least-significant bytes of an AES-128 CMAC digest of (hdr|data), intepreted
 * as a little-endian integer.
 * 
 * Note that sometimes hdr will be empty (hdr=NULL and hdrLen=0).
 * 
 * @param[in] self
 * @param[in] desc      #lora_sm_key
 * @param[in] hdr       may be NULL
 * @param[in] hdrLen    
 * @param[in] data      
 * @param[in] dataLen
 * 
 * @return MIC
 * 
 * The following weak implementation is provided:
 * 
 * @snippet src/lora_sm.c LDL_SM_mic
 * 
 * */
uint32_t LDL_SM_mic(struct lora_sm *self, enum lora_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);

/** Lookup a key and use it to perform ECB AES-128 in-place
 * 
 * @param[in] self
 * @param[in] desc       #lora_sm_key
 * @param[in] b         16B block to encrypt in-place (arbitrary alignment)
 * 
 * The following weak implementation is provided:
 * 
 * @snippet src/lora_sm.c LDL_SM_ecb
 * 
 * */
void LDL_SM_ecb(struct lora_sm *self, enum lora_sm_key desc, void *b);

/** Lookup a key and use it to perform CTR AES-128 in-place
 * 
 * @param[in] self
 * @param[in] desc       #lora_sm_key
 * @param[in] iv        16B block to be used as a nonce/intial value (word aligned)
 * @param[in] data      
 * @param[in] len      
 * 
 * The following weak implementation is provided:
 * 
 * @snippet src/lora_sm.c LDL_SM_ctr
 * 
 * */
void LDL_SM_ctr(struct lora_sm *self, enum lora_sm_key desc, const void *iv, void *data, uint8_t len);

#ifdef __cplusplus
}
#endif

/** @} */

#endif
