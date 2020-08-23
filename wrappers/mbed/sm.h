#ifndef MBED_LDL_SM_H
#define MBED_LDL_SM_H

#include "mbed.h"
#include "ldl_sm_internal.h"
#include "ldl_sm.h"

namespace LDL {

    class SM {

        public:

            const struct ldl_sm_interface *interface;

    };

    class DefaultSM : public SM {

        protected:

            struct ldl_sm state;

            static DefaultSM *to_obj(void *self);
            static struct ldl_sm *to_state(void *self);
            static void begin_update_session_key(struct ldl_sm *self);
            static void end_update_session_key(struct ldl_sm *self);            
            static void update_session_key(struct ldl_sm *self, enum ldl_sm_key key_desc, enum ldl_sm_key root_desc, const void *iv);
            static uint32_t mic(struct ldl_sm *self, enum ldl_sm_key desc, const void *hdr, uint8_t hdrLen, const void *data, uint8_t dataLen);
            static void ecb(struct ldl_sm *self, enum ldl_sm_key desc, void *b);
            static void ctr(struct ldl_sm *self, enum ldl_sm_key desc, const void *iv, void *data, uint8_t len);

            static const struct ldl_sm_interface _interface;

        public:

            DefaultSM(const void *app_key, const void *nwk_key);            
    };

};

#endif
