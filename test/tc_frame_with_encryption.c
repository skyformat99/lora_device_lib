#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_frame.h"
#include "lora_mac.h"
#include "lora_sm.h"
#include "lora_ops.h"

#include <string.h>
#include <stdio.h>

/* note these tests only exist to catch when something changes, they don't validate the correct
 * implementation of the lorawan encryption since lorawan don't provide test vectors */

static void init_mac(struct lora_mac *mac, const void *key, enum lora_mac_operation op)
{
    static struct lora_sm sm;
    size_t i;
    
    (void)memset(mac, 0, sizeof(*mac));    
    (void)memset(&sm, 0, sizeof(sm));    
    
    mac->sm = &sm;
    mac->op = op;
    
    for(i=0; i < sizeof(sm.keys)/sizeof(*sm.keys); i++){
        
        (void)memcpy(sm.keys[i].value, key, sizeof(sm.keys[i].value));
    }
}

static void encode_unconfirmed_up(void **user)
{
    uint8_t retval;
    uint8_t buffer[UINT8_MAX];
    const uint8_t payload[] = "hello world";
    const uint8_t expected[] =  "\x40\x00\x00\x00\x00\x00\x00\x00\x00\xBD\x1D\x9E\x61\x6F\xB5\xFB\x03\x22\x02\x52\xAB\xDC\x77\x2F";
    const uint8_t key[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    struct lora_frame_data f;
    
    struct lora_mac mac;
    init_mac(&mac, key, LORA_OP_DATA_UNCONFIRMED);
    
    (void)memset(&f, 0, sizeof(f));
    
    f.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    f.data = payload;
    f.dataLen = sizeof(payload)-1;
    
    retval = LDL_OPS_prepareData(&mac, &f, buffer, sizeof(buffer));
    
    assert_int_equal(sizeof(expected)-1U, retval);    
    assert_memory_equal(expected, buffer, retval);        
}

static void encode_join_request(void **user)
{
    uint8_t retval;
    uint8_t buffer[UINT8_MAX];
    const uint8_t key[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    const uint8_t expected[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x71\x84\x9D\xAA";
    
    struct lora_mac mac;
    init_mac(&mac, key, LORA_OP_JOINING);
    
    struct lora_frame_join_request f;
    
    (void)memset(&f, 0, sizeof(f));
    
    retval = LDL_OPS_prepareJoinRequest(&mac, &f, buffer, sizeof(buffer));
    
    assert_int_equal(sizeof(expected)-1U, retval);    
    assert_memory_equal(expected, buffer, retval);        
}

static void encode_croft_example(void **user)
{
    uint8_t retval;
    uint8_t buffer[UINT8_MAX];
    const uint8_t payload[] = "{\"name\":\"Turiphro\",\"count\":13,\"water\":true}";
    const uint8_t key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
    const uint8_t expected[] = {0x80, 0x8F, 0x77, 0xBB, 0x07, 0x00, 0x02, 0x00, 0x06, 0xBD, 0x33, 0x42, 0xA1, 0x9F, 0xCC, 0x3C, 0x8D, 0x6B, 0xCB, 0x5F, 0xDB, 0x05, 0x48, 0xDB, 0x4D, 0xC8, 0x50, 0x14, 0xAE, 0xEB, 0xFE, 0x0B, 0x54, 0xB1, 0xC9, 0x98, 0xDE, 0xF5, 0x3E, 0x97, 0x9B, 0x70, 0x1D, 0xAB, 0xB0, 0x45, 0x30, 0x0E, 0xF8, 0x69, 0x9C, 0x38, 0xFC, 0x1A, 0x34, 0xD5};
    
    struct lora_mac mac;
    init_mac(&mac, key, LORA_OP_DATA_CONFIRMED);
    
    struct lora_frame_data f;
    
    (void)memset(&f, 0, sizeof(f));
    
    f.type = FRAME_TYPE_DATA_CONFIRMED_UP;
    f.devAddr = 0x07BB778F;
    f.counter = 2;
    f.port = 6;
    f.data = payload;
    f.dataLen = sizeof(payload)-1;
    
    retval = LDL_OPS_prepareData(&mac, &f, buffer, sizeof(buffer));
    
    assert_int_equal(sizeof(expected), retval);    
    assert_memory_equal(expected, buffer, retval);    
}

static void encode_random_internet_join_request_example(void **user)
{
    uint8_t retval;
    uint8_t buffer[UINT8_MAX];
    const uint8_t key[] = {0xB6, 0xB5, 0x3F, 0x4A, 0x16, 0x8A, 0x7A, 0x88, 0xBD, 0xF7, 0xEA, 0x13, 0x5C, 0xE9, 0xCF, 0xCA};
    uint8_t expected[] = {0x00, 0xDC, 0x00, 0x00, 0xD0, 0x7E, 0xD5, 0xB3, 0x70, 0x1E, 0x6F, 0xED, 0xF5, 0x7C, 0xEE, 0xAF, 0x00, 0x85, 0xCC, 0x58, 0x7F, 0xE9, 0x13};
    
    uint8_t appEUI[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0x00, 0xDC};
    uint8_t devEUI[] = {0x00, 0xAF, 0xEE, 0x7C, 0xF5, 0xED, 0x6F, 0x1E};
    
    struct lora_mac mac;
    init_mac(&mac, key, LORA_OP_JOINING);
    
    struct lora_frame_join_request f;
    
    (void)memset(&f, 0, sizeof(f));
    
    (void)memcpy(f.joinEUI, appEUI, sizeof(appEUI));
    (void)memcpy(f.devEUI, devEUI, sizeof(devEUI));
    f.devNonce = 0xCC85;
    
    retval = LDL_OPS_prepareJoinRequest(&mac, &f, buffer, sizeof(buffer));
    
    assert_int_equal(sizeof(expected), retval);    
    assert_memory_equal(expected, buffer, retval);        
}

static void decode_join_accept(void **user)
{
    const uint8_t key[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    uint8_t input[] = "\x20\xE3\xDE\x10\x87\x95\xF7\x76\xB8\x03\x76\x10\xEF\x78\x69\xB5\xB3";
    bool retval;
    
    struct lora_mac mac;
    init_mac(&mac, key, LORA_OP_JOINING);
    
    struct lora_frame_down f;
    
    retval = LDL_OPS_receiveFrame(&mac, &f, input, sizeof(input)-1U);
    
    assert_true(retval);  
    
    assert_int_equal(FRAME_TYPE_JOIN_ACCEPT, f.type);    
}

static void decode_join_accept_with_cf_list(void **user)
{    
    const uint8_t key[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    uint8_t input[] = "\x20\x14\x0F\x0F\x10\x11\xB5\x22\x3D\x79\x58\x77\x17\xFF\xD9\xEC\x3A\xB6\x05\xA8\x02\xAC\x97\xDD\xE7\xAC\xF0\x5C\x87\xEF\xAC\x47\xAF";
    bool retval;
    
    struct lora_mac mac;
    init_mac(&mac, key, LORA_OP_JOINING);
    
    struct lora_frame_down f;
    
    retval = LDL_OPS_receiveFrame(&mac, &f, input, sizeof(input)-1U);
    
    assert_true(retval);    
    
    assert_int_equal(FRAME_TYPE_JOIN_ACCEPT, f.type);    
}


int main(void)
{
    const struct CMUnitTest tests[] = {
        
        cmocka_unit_test(encode_unconfirmed_up),        
        cmocka_unit_test(encode_join_request),        
        cmocka_unit_test(encode_croft_example),        
        cmocka_unit_test(encode_random_internet_join_request_example),        
        
        cmocka_unit_test(decode_join_accept),        
        cmocka_unit_test(decode_join_accept_with_cf_list)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
