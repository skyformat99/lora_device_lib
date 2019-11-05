# Release History

## 0.1.7

### features

- redundant unconfirmed data frames now defer off-time until all frames (i.e. nbTrans) have been sent
    - off-time will never exceed one hour
    - off-time calculated from band limits as well as maxDCycle (as per usual)
- confirmed and unconfirmed data interfaces now accept an invocation option structure
    - can be set to NULL to use defaults
    - can request piggy-backed LinkCheckReq
    - can specify nbTrans
    - can specify schedule dither
- confirmed data services now use the same retry/backoff strategy as otaa
- standard retry/backoff strategy now adds up to 30 seconds of dither to each retransmission attempt
- improved doxygen

### changes

- renamed LDL_MAC_setRedundancy() to LDL_MAC_setNbtrans()
- renamed LDL_MAC_getRedundancy() to LDL_MAC_getNbTrans()
- renamed LDL_MAC_setAggregated() to LDL_MAC_setMaxDCycle()
- renamed LDL_MAC_getAggregated() to LDL_MAC_getMaxDCycle()
- renamed LDL_System_time() to LDL_System_ticks()
- changed LDL_MAC_unconfirmedData() to accept additional argument (invocation option structure)
- changed LDL_MAC_confirmedData() to accept additional argument(invocation option structure)
- changed OTAA procedure so that maxDCycle is no longer applied (this should only apply to data service)
- changed radio to board interfaces (now using LDL_SPI_*)
- removed LDL_MAC_ticksUntilNextChannel()
- removed LDL_MAC_check() since this can now be requested via invocation option structure
- removed LDL_MAC_setSendDither() since this can now be requested via invocation option structure
- removed weak implementations of mandatory system interfaces
- removed lora_board.c and lora_board.h
- added   lora_spi.h
- changed LDL_System_rand() to accept additional argument (app pointer)
- changed LORA_DEBUG() to accept argument (app pointer)
- changed LORA_INFO() to accept argument (app pointer)
- changed LORA_ERROR() to accept argument (app pointer)

### bugs

## 0.1.6

- arduino wrapper improvements
    - debug messages now included/excluded by code in the the event callback handler
    - added sleepy example
    - global duty cycle limit used to ensure TTN fair use policy is met by default
- fixed global duty cycle limit bug where the limit was being reset after a join
