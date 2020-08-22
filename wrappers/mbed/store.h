#ifndef MBED_LDL_STORE_H
#define MBED_LDL_STORE_H

#include <string.h>

namespace LDL {

    /** LDL::Store provides information like the dev_eui and session
     * data.
     *
     * This abstract class can be subclassed with application specific
     * 
     *
     *
     *
     * */
    class Store {

        public:

            virtual void get_dev_eui(void *data) = 0;            
            virtual void get_join_eui(void *data) = 0;
            
    };

    /** A minimal implmentation of the data store object intended
     * for quick demonstration. 
     *
     * - initialise with const pointers to dev_eui and join_eui
     * - session is not stored or restored
     * - counters are set to zero
     *
     * A practical store object will at least need to keep the
     * join nonce to ensure the device can establish a fresh session.
     *
     * */
    class DefaultStore : public Store {

        private:

            const void *dev_eui;
            const void *join_eui;

        public:

            void get_dev_eui(void *data)
            {
                memcpy(data, dev_eui, 8U);
            }

            void get_join_eui(void *data)
            {
                memcpy(data, join_eui, 8U);
            }
    
            DefaultStore(const void *dev_eui, const void *join_eui) :
                dev_eui(dev_eui),
                join_eui(join_eui)
            {}                
    };
};


#endif
