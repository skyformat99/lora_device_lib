#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_frame.h"

#include <string.h>

static void encode_an_empty_frame(void **user)
{
    uint32_t devAddr = 0U;
    const uint8_t expected[] = "\x40\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00";
    struct lora_frame_data input;
    (void)memset(&input, 0, sizeof(input));
    uint8_t outLen;
    uint8_t out[UINT8_MAX];
    enum lora_frame_type type;

    type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    input.counter = 256;
    input.devAddr = devAddr;
     
    outLen = LDL_Frame_putData(type, &input, out, sizeof(out));
    
    assert_int_equal(sizeof(expected)-1U, outLen);
    assert_memory_equal(expected, out, sizeof(expected)-1U);
}

static void decode_an_empty_frame(void **user)
{
    uint32_t devAddr = 0U;
    uint8_t input[] = "\x40\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00";
    struct lora_frame_down expected;
    struct lora_frame_down output;
    bool result;
    enum lora_frame_type type;

    (void)memset(&expected, 0, sizeof(expected));
    
    type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    
    expected.type = type;
    expected.counter = 256;
    expected.devAddr = devAddr;

    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_true(result);
    
    assert_int_equal(expected.counter, output.counter);
    assert_int_equal(expected.devAddr, output.devAddr);
    assert_int_equal(expected.ack, output.ack);
    assert_int_equal(expected.adr, output.adr);
    assert_int_equal(expected.adrAckReq, output.adrAckReq);
    assert_int_equal(expected.pending, output.pending);
    
    assert_int_equal(expected.optsLen, output.optsLen);
    
    if(expected.opts != NULL){
    
        assert_memory_equal(expected.opts, output.opts, sizeof(expected.optsLen));
    }
    
    assert_int_equal(expected.dataLen, output.dataLen);
    
    if(expected.data != NULL){
        
        assert_memory_equal(expected.data, output.opts, sizeof(expected.optsLen));
    }
    
    //assert_int_equal(expected., output.);
    //ssert_int_equal(expected., output.);
    
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(encode_an_empty_frame),        
        cmocka_unit_test(decode_an_empty_frame),        
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
