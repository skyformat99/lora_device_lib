Porting Guide
=============

## General

- LDL interfaces, except those marked as interrupt safe, must be accessed from a single thread of execution. 
- If interrupt safe interfaces are accessed from ISRs
    - LORA_SYSTEM_ENTER_CRITICAL() and LORA_SYSTEM_LEAVE_CRITICAL() must be defined
    - the ISR must have a higher priority than the non-interrupt thread of execution
    - bear in mind that interrupt-safe interfaces never block and return as quickly as possible

## Compiling Doxygen

Doxygen generated documentation for the master branch is always available
online. Doxygen can be generated locally for branches and old releases:

- `cd doxygen && make`
- open doxygen/output/index.html in browser

If you find Doxygen offensive you can read the documentation directly
from the headers. Group descriptions can be found by searching 
for @defgroup markup.

## Checklist

1. Review Doxygen groups

    - [system](https://cjhdev.github.io/lora_device_lib_api/group__ldl__system.html)
    - [radio connector](https://cjhdev.github.io/lora_device_lib_api/group__ldl__radio__connector.html)
    - [build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html)

2. Implement the following system functions

    - LDL_System_getIdentity()
    - LDL_System_ticks()
    - LDL_System_tps()
    - LDL_System_eps()

3. Define the following macros (if interrupt-safe functions are called from ISRs)

    - LORA_SYSTEM_ENTER_CRITICAL()
    - LORA_SYSTEM_LEAVE_CRITICAL()

4. Define at least one region via build options

    - LORA_ENABLE_EU_863_870
    - LORA_ENABLE_US_902_928
    - LORA_ENABLE_AU_915_928
    - LORA_ENABLE_EU_433

5. Define at least one radio driver via build options

    - LORA_ENABLE_SX1272
    - LORA_ENABLE_SX1276

6. Implement the radio connector

    - See [radio connector](https://cjhdev.github.io/lora_device_lib_api/group__ldl__radio__connector.html)

## Building Source

- define preprocessor symbols as needed ([see build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html))
- add `include` folder to include search path
- build all sources in `src`

## Advanced Topics

### Modifying the Security Module

A common porting task is to replace the default cryptographic implementations with
hardened implementations.

LDL depends on the following modes:

- AES128-ECB (default: lora_aes.c)
- AES128-CTR (default: lora_ctr.c)
- AES128-CMAC (default: lora_cmac.c)

These are integrated in the default security module (lora_sm.c). The default SM
functions are marked as "weak" for the linker.

If your toolchain supports weak symbols, replacing some or all of the
default implementations is as simple as re-implementing the function
somewhere else.

If you don't want to use weak symbols, remove lora_sm.c
from the build and re-implement the whole thing somewhere else. 
The same applies to situations where a more robust security 
module (i.e. HSM) is required.

### Persistent Sessions

LDL notifies the application of changes to session state
via the LORA_MAC_SESSION_UPDATED event. A pointer to the new session
state is passed with this event.

To initialise LDL from a cached state, a pointer can be passed 
to LDL_MAC_init(). LDL_MAC_init() will copy from the pointer, or reset
to default if one is not provided.

Therefore, to implement persistent sessions, the application must: 

- deal with caching session state when it is provided
- deal with recovering session state prior to initialising LDL
- deal with migration if LDL is updated

Note that:

- session state does not contain sensitive information
- session state is best treated as opaque data
- loading invalid/corrupt session state may result in undefined behaviour

### Sleeping

LDL is designed to work with "sleepy" applications.

- ensure that LDL_System_ticks() is incremented by a time source
that keeps working in sleep mode
- use LDL_MAC_ticksUntilNextEvent() to set a wakeup timer before entering sleep mode
- use external interrupts to receive radio interrupts

### Co-operative Scheduling

LDL is designed to work in a mainloop style application.

If another task is expected to run for an excessive period of time (e.g. >1 second),
a simple scheduler can use LDL_MAC_critical() to check if now is a good time to run
that task.
