Release History
===============

# 0.4.0

In this release effort has gone into making it easier to evaluate
a working version of this project on MBED. This is useful for quick
evaluations as well as regression testing. 

Arduino was meant to be the evaluation platform but frankly it's a nuisance
to use. To save time, the Arduino wrapper has not been updated
to work with the changes made in this release.

I've made a number of breaking changes to make the project easier to
wrap in C++. A consequence of this is that you will likely need to change
the way you init the library if you are updating from an ealier version.
Recommend brushing up on the initialisation notes in the porting guide.

- decoupled MAC from SM by way of struct ldl_sm_interface
- decoupled MAC from Radio by way of struct ldl_radio_interface
- decoupled Radio from Chip Interface by way of struct ldl_chip_interface
- added default behaviour to LDL_MAC_init() so that the default
  interfaces are used if the interface struct pointers are left NULL
- MAC no longer sets the radio interrupt callback in MAC_init(), the
  application must now do this during initialisation
- removed duplicate code from the Arduino wrapper
- added MBED wrapper

The decoupling structs make it possible to wrap SM and Radio up as classes
without any weird linkages caused by the underlying C code. Being
able to decouple the SM is especially useful for implementing different
types of SMs for different application requirements.

# 0.3.1

- fixed LDL_MAC_JOIN_COMPLETE event to pass argument instead of NULL (thanks dzurikmiroslav)
- transmit channel is now selected before MIC is generated to solve LoRaWAN 1.1 issues
  caused by the chIndex being part of the MIC generation
- MIC is now updated for redundant transmissions in LoRaWAN 1.1 mode
- fixed implementation to reject MAC commands that would cause all
  channels to be masked
- upstream MAC commands are now deferred until the application sends the next upstream message
- LDL_MAC_unconfirmedData() and LDL_MAC_confirmedData() will now indicate if they
  have failed due to prioritising deferred MAC commands which cannot fit in
  in the same frame
- removed LDL_MAC_setNbTrans() and LDL_MAC_getNbTrans() since the per-invocation
  override feature makes it redundant

# 0.3.0

- BREAKING CHANGE to ldl_chip.h interface to work with SPI transactions; see radio connector documentation or header file for more details
- updated arduino wrapper to work with new ldl_chip.h interface
- fixed SNR margin calculation required for DevStatus MAC command; was
  previously returning SNR not margin
- fixed arduino wrapper garbled payload issue (incorrect session key index from changes made at 0.2.4)
- fopts IV now being correctly generated for 1.1 servers (i was 1 instead of 0)

# 0.2.4

- now deriving join keys in LDL_MAC_otaa() so they are ready to check
  joinAccept
- fixed joinNonce comparison so that 1.1 joins are possible
- join nonce was being incremented before key derivation on joining which
  produced incorrect keys in 1.1 mode
- implemented a special security module for the arduino wrapper to save
  some memory in exchange for limiting the wrapper to LoRaWAN 1.0 servers
- added little endian optimisation build option (LDL_LITTLE_ENDIAN)
- processCommands wasn't recovering correctly from badly formatted
  mac commands. 
- added LDL_DISABLE_POINTONE option to remove 1.1 features for devices
  that will only be used with 1.0 servers
- removed LDL_ENABLE_RANDOM_DEV_NONCE since it is now covered by
  LDL_DISABLE_POINTONE
- changed the way ctr mode IV is generated so that a generic ctr implementation
  can now be substituted

# 0.2.3

## bugs

- was using nwk instead of app to derive apps key in 1.1 mode

# 0.2.2

## features

none

## changes

- changed LDL_OPS_receiveFrame() to use nwk key to decrypt and MIC join accepts
  when they are answering a join request

## bugs

- was using app key instead of nwk to decrypt and MIC join accepts

# 0.2.1

## features

- new build options
    - LDL_DISABLE_FULL_CHANNEL_CONFIG halves memory required for channel config
    - LDL_DISABLE_CMD_DL_CHANNEL removes handling for this MAC command

## changes

- reduced stack usage during MAC command processing
- Arduino wrapper now uses LDL_ENABLE_STATIC_RX_BUFFER
- Arduino wrapper now uses LDL_DISABLE_CHECK
- Arduino wrapper now uses LDL_DISABLE_DEVICE_TIME
- Arduino wrapper now uses LDL_DISABLE_FULL_CHANNEL_CONFIG
- Arduino wrapper now uses LDL_DISABLE_CMD_DL_CHANNEL

## bugs

- Arduino wrapper on ATMEGA328P was running out of stack at the point where it had to
  process a MAC command. This is a worry because there is ~1K available for stack.

# 0.2.0

Warning: this update has a significant number of interface name changes. 

## features

- LoRaWAN 1.1 support
- encryption and key management is now the domain of the Security Module (SM)
    - keys are referenced by descriptors
    - cipher/plain text is sent to SM for processing
    - generic cryptographic operations decoupled from LoRaWAN concerns
    - implementation is simple to modify or replace completely
- redundant unconfirmed data frames now defer off-time until all frames (i.e. nbTrans) have been sent
    - off-time will never exceed LDL_Region_getMaxDCycleOffLimit()
    - off-time calculated from band limits as well as maxDCycle (as per usual)
- confirmed and unconfirmed data interfaces now accept an invocation option structure
    - can be set to NULL to use defaults
    - can request piggy-backed LinkCheckReq
    - can request piggy-backed DeviceTimeReq
    - can specify nbTrans
    - can specify schedule dither
- confirmed data services now use the same retry/backoff strategy as otaa
- standard retry/backoff strategy now guarantees up to 30 seconds of dither to each retransmission attempt
- antenna gain/loss can now be compensated for at LDL_MAC_init()
- improved doxygen interface documentation

## changes

- changed all source files to use 'ldl' prefix instead of 'lora'
- added   'LDL' and 'ldl' prefixes to all enums that were not yet prefixed
- changed all 'LORA' and 'lora' prefixes to 'LDL' and 'ldl' for consistency
- added   LDL_Radio_interrupt() to take over from LDL_MAC_interrupt()
- changed LDL_MAC_interrupt() to LDL_MAC_radioEvent() which gets called by LDL_Radio_interrupt()
- changed lora_frame.c and lora_mac_commands.c to use the same pack/unpack code
- changed Region module to handle cflist processing instead of MAC
- changed Region module to handle cflist unpacking instead of Frame
- changed Frame to no longer perform encryption/decryption (now the domain of TSM)
- changed MAC to no longer perform key derivation (now the domain of TSM)
- changed uplink and downlink counters to 32 bit
- renamed LDL_MAC_setRedundancy() to LDL_MAC_setNbtrans()
- renamed LDL_MAC_getRedundancy() to LDL_MAC_getNbTrans()
- renamed LDL_MAC_setAggregated() to LDL_MAC_setMaxDCycle()
- renamed LDL_MAC_getAggregated() to LDL_MAC_getMaxDCycle()
- renamed LDL_System_time() to LDL_System_ticks()
- changed LDL_MAC_interrupt() to use one less argument (no need to pass time)
- changed LDL_MAC_unconfirmedData() to accept additional argument (invocation option structure)
- changed LDL_MAC_confirmedData() to accept additional argument(invocation option structure)
- changed OTAA procedure so that maxDCycle is no longer applied (this should only apply to data service)
- changed radio to board interfaces (now using LDL_Chip_*)
- removed LDL_MAC_ticksUntilNextChannel()
- removed LDL_MAC_check() since this can now be requested via invocation option structure
- removed LDL_MAC_setSendDither() since this can now be requested via invocation option structure
- removed weak implementations of mandatory system interfaces
- removed lora_board.c and lora_board.h
- added   lora_chip.h
- changed LDL_System_rand() to accept additional argument (app pointer)
- changed LORA_DEBUG() to accept argument (app pointer)
- changed LORA_INFO() to accept argument (app pointer)
- changed LORA_ERROR() to accept argument (app pointer)
- changed how off-time is accounted for

## bugs

none

# 0.1.6

- arduino wrapper improvements
    - debug messages now included/excluded by code in the the event callback handler
    - added sleepy example
    - global duty cycle limit used to ensure TTN fair use policy is met by default
- fixed global duty cycle limit bug where the limit was being reset after a join
