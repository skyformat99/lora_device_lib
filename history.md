# Release History

## 0.1.7

- new off-time accumulator implementation for duty cycle and retry timing that allows up to one hour of off-time to be deferred
- redundant unconfirmed data frames are now sent back-to-back when nbTrans > 1    
- global duty cycle limits are no longer applied to join request transmission
- confirmed data now uses the same retransmission back-off and dithering as join request
    - retransmission are now always dithered by up to 30s
- new invocation options structure for confirmed and unconfirmed data services
    - can be set to NULL to rely on defaults
    - can request a link-check be performed
    - can specify nbTrans to apply only to the next data service
    - can specify seconds of dither to apply to the next data service
- removed LDL_MAC_check() since this can now be requested via the data invocation option structure
- improved memory footprint (mainly due to the new off-time implementation)
    - Arduino mbed_sx1272_small_code occupies 23622B of 30729B flash and 711B of 2048B ram on an ATMEGA328P

## 0.1.6

- arduino wrapper improvements
    - debug messages now included/excluded by code in the the event callback handler
    - added sleepy example
    - global duty cycle limit used to ensure TTN fair use policy is met by default
- fixed global duty cycle limit bug where the limit was being reset after a join
