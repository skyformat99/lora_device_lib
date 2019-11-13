lora_device_lib
===============

A LoRaWAN implementation for nodes/devices.

This project is experimental, which means:

- interfaces are not stable
- formal conformance testing has not been completed
- it should not be trusted

See [history file](history.md) for releases.

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

## Features

- LoRaWAN 1.1 support 
- Class A
- OTAA
- Confirmed and Unconfirmed Data
    - per invocation options (overriding global settings)
        - redundancy (nbTrans)
        - piggy-back LinkCheckReq
        - transmit start time dither
    - deferred duty cycle limit for redundant transmissions
- ADR
- Supported MAC commands
    - LinkCheckReq/Ans
    - LinkADRReq/Ans
    - DutyCycleReq/Ans
    - RXParamSetupReq/Ans
    - DevStatusReq/Ans
    - NewChannelReq/Ans
    - RXTimingSetupReq/Ans
    - RXParamSetupReq/Ans
    - DlChannelReq/Ans
- Minimal system interface
    - requires read access to a free-running counter incrementing at a rate in the range of 1KHz to 1MHz
    - requires a psuedorandom number generator (e.g. rand())
    - requires access to the radio
- Supported regions (run-time option)
    - EU_868_870
    - EU_433
    - US_902_928
    - AU_915_928
- Supported radios (run-time option)
    - SX1272
    - SX1276
- Build time [options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html)
- [API documentation](https://cjhdev.github.io/lora_device_lib_api/)
- Examples
    - [Arduino wrapper](bindings/arduino/output/arduino_ldl)
    
## Limitations

- Class B and C not supported
- GFSK modulation not supported
- ABP not supported
- rejoin not supported

## Getting Started

- [porting guide](porting.md)

## Design Goals

LDL should be simple to use and port.

LDL should provide minimal interfaces that line up with words used in the 
specification. More sophisticated interfaces should be implemented as wrappers.

LDL should not repeat itself when implementing regions. All LoRaWAN regions are based on one of two patterns; fixed or
programmable channels. LDL should take advantage of the overlap.

It should be possible to wrap LDL in another programming language.

It should be possible to run LDL with the hardware layer replaced by software.

LDL should be able to share a single thread of execution with other tasks. It should:

- not block
- indicate when it requires prioritisation (critical timing window)
- indicate time until the next event
- calculate how late it is to handling an event
- compensate for timing jitter
- push events to the application asynchronously

LDL should fit into an existing system, the existing system should not 
have to fit into LDL.

## See Also

- [LoRaMAC-Node](https://github.com/Lora-net/LoRaMac-node): Semtech reference implementation

## License

MIT
