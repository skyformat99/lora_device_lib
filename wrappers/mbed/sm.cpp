#include "sm.h"

using namespace LDL;

const struct ldl_sm_interface DefaultSM::_interface = {
    .update_session_key = update_session_key,
    .begin_update_session_key = begin_update_session_key,
    .end_update_session_key = end_update_session_key,
    .mic = mic,
    .ecb = ecb,
    .ctr = ctr
};

DefaultSM::DefaultSM(const void *app_key, const void *nwk_key)    
{
    interface = &_interface;
    LDL_SM_init(&state, app_key, nwk_key);
}

DefaultSM *
DefaultSM::to_obj(void *self)
{
    return static_cast<DefaultSM *>(self);
}

struct ldl_sm *
DefaultSM::to_state(void *self)
{
    return &to_obj(self)->state;
}
    
void
DefaultSM::begin_update_session_key(struct ldl_sm *self)
{
    LDL_SM_beginUpdateSessionKey(to_state(self));
}
            
void
DefaultSM::end_update_session_key(struct ldl_sm *self)
{
    LDL_SM_endUpdateSessionKey(to_state(self));
}

void
DefaultSM::update_session_key(struct ldl_sm *self, enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv)
{
    LDL_SM_updateSessionKey(to_state(self), key_desc, root_desc, iv);
}

uint32_t
DefaultSM::mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen)
{
    return LDL_SM_mic(to_state(self), desc, hdr, hdrLen, data, dataLen);
}

void
DefaultSM::ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b)
{
    LDL_SM_ecb(to_state(self), desc, b);
}

void
DefaultSM::ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len)
{
    LDL_SM_ctr(to_state(self), desc, iv, data, len);
}

