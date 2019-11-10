/* Copyright (c) 2019 Cameron Harper
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */

#ifndef LORA_PLATFORM_H
#define LORA_PLATFORM_H

/** @file */

/**
 * @defgroup ldl_build_options Build Options
 * @ingroup ldl
 * 
 * # Build Options
 * 
 * These can be defined in two ways:
 * 
 * - using your build system (e.g. using -D)
 * - in a header file which is then included using the #LORA_TARGET_INCLUDE macro
 * 
 * @{
 * */

#ifndef LORA_MAX_PACKET
    /** Redefine this to reduce the stack and data memory footprint.
     * 
     * The maximum allowable size is UINT8_MAX bytes
     * 
     * */
    #define LORA_MAX_PACKET UINT8_MAX    
#endif
 
#ifndef LORA_DEFAULT_RATE
    /** Redefine to limit maximum rate to this setting
     * 
     * Useful if you want to avoid a large spreading factor if your
     * hardware doesn't support it.
     * 
     * */
    #define LORA_DEFAULT_RATE 1U

#endif

#ifndef LORA_REDUNDANCY_MAX
    /** Redefine to limit the maximum redundancy setting 
     * 
     * (i.e. LinkADRReq.redundancy.NbTrans)
     * 
     * The implementation and that standard limit this value to 15. Since
     * the network can set this value, if you feel it is way too high to 
     * ever consider using, you can use this macro to further limit it.
     * 
     * e.g.
     * 
     * @code{.c}
     * #define LORA_REDUNDANCY_MAX 3
     * @endcode
     * 
     * would ensure there will never be more than 3 redundant frames.
     * 
     * */
    #define LORA_REDUNDANCY_MAX 0xfU
#endif

#ifndef LORA_REDUNANCY_OFFTIME_LIMIT
    /** This value is the limit which accumulated off-time must not
     * exceed when redundant unconfirmed data frames are sent
     * back-to-back.
     * 
     * The default is one hour. This is appropriate for EU_863_870
     * where duty cycle is evaluated over one hour.
     * 
     * Units are milliseconds.
     * 
     * */
    #define LORA_REDUNANCY_OFFTIME_LIMIT (60UL*60UL*1000UL)
#endif

#ifdef DOXYGEN

    /** Include a target specific file in all headers.
     * 
     * This file can be used to define:
     * 
     * - @ref ldl_build_options
     * - anything else you want included
     * 
     * Example (defined from a makefile):
     * 
     * @code
     * -DLORA_TARGET_INCLUDE='"target_specific.h"'
     * @endcode
     * 
     * Where `target_specific.h` would be kept somewhere on the include
     * search path.
     * 
     * */
    #define LORA_TARGET_INCLUDE
    #undef LORA_TARGET_INCLUDE
    
    /** 
     * Define to add support for SX1272
     * 
     * */
    #define LORA_ENABLE_SX1272
    //#undef LORA_ENABLE_SX1272

    /** 
     * Define to add support for SX1276
     * 
     * */
    #define LORA_ENABLE_SX1276
    //#undef LORA_ENABLE_SX1276 

    /** 
     * Define to remove parts of the codec not required for a device
     * 
     * */
     #define LORA_DISABLE_FULL_CODEC
     #undef LORA_DISABLE_FULL_CODEC

    /** 
     * Define to remove the link-check feature.
     * 
     * */
    #define LORA_DISABLE_CHECK 
    #undef LORA_DISABLE_CHECK

    /** 
     * Define to remove event generation when RX1 and RX2 are opened.
     * 
     * */
    #define LORA_DISABLE_SLOT_EVENT
    #undef LORA_DISABLE_SLOT_EVENT

    /** 
     * Define to remove event generation when TX completes.
     * 
     * */
    #define LORA_DISABLE_TX_COMPLETE_EVENT
    #undef LORA_DISABLE_TX_COMPLETE_EVENT 

    /** 
     * Define to remove event generation when TX begins.
     * 
     * */
    #define LORA_DISABLE_TX_BEGIN_EVENT  
    #undef LORA_DISABLE_TX_BEGIN_EVENT 

    /**
     * Define to remove event generation for downstream stats
     * 
     * */
    #define LORA_DISABLE_DOWNSTREAM_EVENT
    #undef LORA_DISABLE_DOWNSTREAM_EVENT
    
    /**
     * Define to remove event generation for chip error event
     * 
     * */
    #define LORA_DISABLE_CHIP_ERROR_EVENT
    #undef LORA_DISABLE_CHIP_ERROR_EVENT
    
    /**
     * Define to remove event generation for mac reset event
     * 
     * */
    #define LORA_DISABLE_MAC_RESET_EVENT
    #undef LORA_DISABLE_MAC_RESET_EVENT
    
    /**
     * Define to remove RX event
     * 
     * */
    #define LORA_DISABLE_RX_EVENT
    #undef LORA_DISABLE_RX_EVENT
    
    /**
     * Define to remove startup event
     * 
     * */
    #define LORA_DISABLE_MAC_STARTUP_EVENT
    #undef LORA_DISABLE_MAC_STARTUP_EVENT
    
    /**
     * Define to remove join timeout event
     * 
     * */
    #define LORA_DISABLE_JOIN_TIMEOUT_EVENT
    #undef LORA_DISABLE_JOIN_TIMEOUT_EVENT
    
    /**
     * Define to remove data complete event
     * 
     * */
    #define LORA_DISABLE_DATA_COMPLETE_EVENT
    #undef LORA_DISABLE_DATA_COMPLETE_EVENT
    
    /**
     * Define to remove data timeout event
     * 
     * */
    #define LORA_DISABLE_DATA_TIMEOUT_EVENT
    #undef LORA_DISABLE_DATA_TIMEOUT_EVENT
    
    /**
     * Define to remove join complete event
     * 
     * */
    #define LORA_DISABLE_JOIN_COMPLETE_EVENT
    #undef LORA_DISABLE_JOIN_COMPLETE_EVENT
    
    /** 
     * Define to add a startup delay (in milliseconds) to when bands become
     * available.
     * 
     * This is an optional safety feature to ensure a device stuck
     * in a reset loop does transmit too often.
     *
     * If undefined this defaults to zero.
     * 
     * */
    #define LORA_STARTUP_DELAY
    #undef LORA_STARTUP_DELAY
    
    /**
     * Define to enable support for AU_915_928
     * 
     * */
    #define LORA_ENABLE_AU_915_928
    //#undef LORA_ENABLE_AU_915_928
    
    /**
     * Define to enable support for EU_863_870
     * 
     * */
    #define LORA_ENABLE_EU_863_870
    //#undef LORA_ENABLE_EU_863_870
    
    /**
     * Define to enable support for EU_433
     * 
     * */
    #define LORA_ENABLE_EU_433
    //#undef LORA_ENABLE_EU_433
    
    /**
     * Define to enable support for US_902_928
     * 
     * */
    #define LORA_ENABLE_US_902_928
    //#undef LORA_ENABLE_US_902_928

    /**
     * Define to keep RX buffer in mac state rather than
     * on the stack.
     * 
     * This will save the stack from growing by #LORA_MAX_PACKET bytes
     * when LDL_MAC_process() is called.
     * 
     * */
    #define LORA_ENABLE_STATIC_RX_BUFFER
    #undef LORA_ENABLE_STATIC_RX_BUFFER

    /** 
     * Define to include LoRaWAN 1.1 features in build
     * 
     * This is backwards compatible with 1.0 but will require
     * more resources.
     * 
     * */
    #define LORA_ENABLE_POINT_ONE
    #undef LORA_ENABLE_POINT_ONE
    

#endif

#ifdef LORA_TARGET_INCLUDE
    #include LORA_TARGET_INCLUDE    
#endif


/** @} */
#endif
