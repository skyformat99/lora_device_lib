#ifndef MBED_LDL_MAC_H
#define MBED_LDL_MAC_H

#include "mbed.h"

#include "ldl_mac.h"
#include "ldl_system.h"

#include "radio.h"
#include "sm.h"
#include "store.h"

namespace LDL {

    class MAC {

        protected:

            enum {

                OFF,
                ON
            
            } run_state;
            
            struct ldl_mac mac;
            Radio& radio;
            SM& sm;
            Store& store;

            static Timer timer;

            EventQueue events;
            Thread event_thread;
            int next_event_handler;

            volatile bool done;
            Semaphore api;
            Mutex mutex;

            Callback<void(unsigned int)> entropy_cb;
            Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> event_cb;
            Callback<void(uint8_t, const void *, uint8_t)> data_cb;
            
            void begin_api();
            void wait_until_api_done();
            void notify_api();

            static MAC *to_obj(void *ptr);
            
            static void app_handler(void *app, enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);

            /* called by radio in ISR */
            void handle_radio_event(enum ldl_radio_event event);

            /* executed from the event queue */
            void do_process();
            void do_unconfirmed(bool *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);
            void do_confirmed(bool *retval, uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts);
            void do_otaa(bool *retval);
            void do_forget();
            void do_set_rate(bool *retval, uint8_t value);
            void do_get_rate(uint8_t *retval);
            void do_set_power(bool *retval, uint8_t value);
            void do_get_power(uint8_t *retval);
            void do_enable_adr();
            void do_disable_adr();
            void do_adr(bool *retval);
            void do_get_errno(enum ldl_mac_errno *retval);
            void do_joined(bool *retval);
            void do_ready(bool *retval);
            void do_get_op(enum ldl_mac_operation *retval);
            void do_get_state(enum ldl_mac_state *retval);
            void do_set_max_dcycle(uint8_t value);
            void do_get_max_dcycle(uint8_t *retval);
            void do_handle_radio_event(enum ldl_radio_event event);

        public:

            static uint32_t ticks(void *self);            

            MAC(Store& store, SM& sm, Radio& radio);

            bool start(enum ldl_region region);
            void stop();

            bool unconfirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);   
            bool confirmed(uint8_t port, const void *data, uint8_t len, const struct ldl_mac_data_opts *opts = NULL);        

            bool otaa();
                           
            void forget();

            bool set_rate(uint8_t value);            
            uint8_t get_rate();

            bool set_power(uint8_t value);
            uint8_t get_power();

            void enable_adr();
            void disable_adr();
            bool adr();     
            
            enum ldl_mac_errno get_errno();                
            
            bool joined();            
            bool ready();
            enum ldl_mac_operation get_op();
            enum ldl_mac_state get_state();

            void set_max_dcycle(uint8_t value);            
            uint8_t get_max_dcycle();

            void set_event_cb(Callback<void(enum ldl_mac_response_type, const union ldl_mac_response_arg *)> cb)
            {
                event_cb = cb;
            }

            void set_entropy_cb(Callback<void(unsigned int)> cb)
            {
                entropy_cb = cb;
            }

            void set_data_cb(Callback<void(uint8_t, const void *, uint8_t)> cb)
            {
                data_cb = cb;
            }
            
            void print_event(enum ldl_mac_response_type type, const union ldl_mac_response_arg *arg);
    };

};

#endif
