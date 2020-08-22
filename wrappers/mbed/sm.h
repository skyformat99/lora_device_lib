#ifndef MBED_LDL_SM_H
#define MBED_LDL_SM_H

#include "mbed.h"
#include "ldl_sm_internal.h"
#include "ldl_sm.h"

namespace LDL {

    class SM {

        public:

            const struct ldl_sm_adapter adapter = {0};
            
    };

    class DefaultSM : public SM {

        protected:

            struct ldl_sm state;

            static DefaultSM *to_obj(void *self)
            {
                return static_cast<DefaultSM *>(self);
            }

            static struct ldl_sm *to_state(void *self)
            {
                return &to_obj(self)->state;
            }
    
            static void begin_update_session_key(struct ldl_sm *self)
            {
                LDL_SM_beginUpdateSessionKey(to_state(self));
            }
            
            static void end_update_session_key(struct ldl_sm *self)
            {
                LDL_SM_endUpdateSessionKey(to_state(self));
            }
            
            static void update_session_key(struct ldl_sm *self, enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv)
            {
                LDL_SM_updateSessionKey(to_state(self), key_desc, root_desc, iv);
            }
            
            static uint32_t mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen)
            {
                return LDL_SM_mic(to_state(self), desc, hdr, hdrLen, data, dataLen);
            }
            
            static void ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b)
            {
                LDL_SM_ecb(to_state(self), desc, b);
            }
            
            static void ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len)
            {
                LDL_SM_ctr(to_state(self), desc, iv, data, len);
            }

        public:

            const struct ldl_sm_adapter adapter {
                .update_session_key = update_session_key,
                .begin_update_session_key = begin_update_session_key,
                .end_update_session_key = end_update_session_key,
                .mic = mic,
                .ecb = ecb,
                .ctr = ctr
            };
    
            DefaultSM(const void *app_key, const void *nwk_key)
            {
                LDL_SM_init(&state, app_key, nwk_key);
            }
    };

};

#endif
