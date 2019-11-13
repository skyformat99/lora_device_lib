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

#ifndef __LORA_DEBUG_H
#define __LORA_DEBUG_H

/** @file */

/**
 * @addtogroup ldl_build_options
 * 
 * @{
 * */

#include "lora_platform.h"

#ifndef LORA_ERROR
    /** A printf-like function that captures run-time error level messages 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LORA_ERROR(...) do{printf(__VA_ARGS__);printf("\n");}while(0);
     * @endcode
     * 
     * If not defined, all LORA_ERROR() messages will be left out of the build.
     * 
     * @param[in] APP app from LDL_MAC_init() or NULL if not available
     * 
     * */
    #define LORA_ERROR(APP,...)
#endif

#ifndef LORA_INFO
    /** A printf-like function that captures run-time info level messages 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LORA_INFO(...) do{printf(__VA_ARGS__);printf("\n");}while(0);
     * @endcode
     * 
     * If not defined, all LORA_INFO() messages will be left out of the build.
     * 
     * @param[in] APP app from LDL_MAC_init() or NULL if not available
     * 
     * */
    #define LORA_INFO(APP,...)
#endif

#ifndef LORA_DEBUG
    /** A printf-like function that captures run-time debug level messages with 
     * varaidic arguments 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LORA_DEBUG(APP, ...) do{printf(__VA_ARGS__);printf("\n");}while(0);
     * @endcode
     * 
     * If not defined, all LORA_DEBUG() messages will be left out of the build.
     * 
     * @param[in] APP app from LDL_MAC_init() or NULL if not available
     * 
     * */
    #define LORA_DEBUG(APP, ...)
#endif

#ifndef LORA_ASSERT
    /** An assert-like function that performs run-time assertions on 'X' 
     * 
     * Example:
     * 
     * @code{.c}
     * #define LORA_ASSERT(X) assert(X);
     * @endcode
     * 
     * If not defined, all LORA_ASSERT() checks will be left out of the build.
     * 
     * */
    #define LORA_ASSERT(X)
#endif

#ifndef LORA_PEDANTIC
    /** A assert-like function that performs run-time assertions on 'X' 
     * 
     * These assertions are considered pedantic. They are useful for development
     * but excessive for production.
     * 
     * Example:
     * 
     * @code{.c}
     * #define LORA_PEDANTIC(X) assert(X);
     * @endcode
     * 
     * If not defined, all LORA_PEDANTIC() checks will be left out of the build.
     * 
     * */
    #define LORA_PEDANTIC(X)
#endif

/** @} */
#endif
