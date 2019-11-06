#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_radio.h"
#include "lora_mac.h"

static const uint8_t eui[] = "\x00\x00\x00\x00\x00\x00\x00\x00";
static const uint8_t key[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

static void responseHandler(void *receiver, enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
}

static void test_init(void **user)
{
    struct lora_mac self;
    struct lora_radio radio;
    
    LDL_Radio_init(&radio, LORA_RADIO_SX1272, NULL);
    LDL_MAC_init(&self, &self, EU_863_870, &radio, responseHandler);
}

void LDL_Chip_select(void *self, bool state)
{
}

void LDL_Chip_reset(void *self, bool state)
{
}

void LDL_Chip_write(void *self, uint8_t data)
{
}

uint8_t LDL_Chip_read(void *self)
{
    return 0U;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
