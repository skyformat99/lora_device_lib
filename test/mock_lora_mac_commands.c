#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cmocka.h"
#include "lora_mac_commands.h"

bool LDL_MAC_putLinkCheckReq(struct ldl_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putLinkCheckAns(struct ldl_stream *s, const struct ldl_link_check_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putLinkADRReq(struct ldl_stream *s, const struct ldl_link_adr_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putLinkADRAns(struct ldl_stream *s, const struct ldl_link_adr_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDutyCycleReq(struct ldl_stream *s, const struct ldl_duty_cycle_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDutyCycleAns(struct ldl_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXParamSetupReq(struct ldl_stream *s, const struct ldl_rx_param_setup_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDevStatusReq(struct ldl_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putDevStatusAns(struct ldl_stream *s, const struct ldl_dev_status_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putNewChannelReq(struct ldl_stream *s, const struct ldl_new_channel_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXParamSetupAns(struct ldl_stream *s, const struct ldl_rx_param_setup_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putNewChannelAns(struct ldl_stream *s, const struct ldl_new_channel_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDLChannelReq(struct ldl_stream *s, const struct ldl_dl_channel_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putDLChannelAns(struct ldl_stream *s, const struct ldl_dl_channel_ans *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXTimingSetupReq(struct ldl_stream *s, const struct ldl_rx_timing_setup_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putRXTimingSetupAns(struct ldl_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_putTXParamSetupReq(struct ldl_stream *s, const struct ldl_tx_param_setup_req *value)
{
    return mock_type(bool);
}

bool LDL_MAC_putTXParamSetupAns(struct ldl_stream *s)
{
    return mock_type(bool);
}

bool LDL_MAC_getDownCommand(struct ldl_stream *s, struct ldl_downstream_cmd *cmd)
{
    return mock_type(bool);
}

bool LDL_MAC_getUpCommand(struct ldl_stream *s, struct ldl_upstream_cmd *cmd)
{
    return mock_type(bool);
}

bool LDL_MAC_peekNextCommand(struct ldl_stream *s, enum ldl_mac_cmd_type *type)
{
    return mock_type(bool);
}

uint8_t LDL_MAC_sizeofCommandUp(enum ldl_mac_cmd_type type)
{
    return mock();
}
