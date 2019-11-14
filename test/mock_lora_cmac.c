#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <string.h>

#include "cmocka.h"

#include "lora_cmac.h"

void LDL_CMAC_init(struct ldl_cmac_ctx *ctx, const struct ldl_aes_ctx *aes_ctx)
{
}

void LDL_CMAC_update(struct ldl_cmac_ctx *ctx, const void *data, uint8_t len)
{
}

void LDL_CMAC_finish(const struct ldl_cmac_ctx *ctx, void *out, uint8_t outMax)
{
    memset(out, 0, outMax);
}
