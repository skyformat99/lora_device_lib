LDL
===

LDL is a LoRaWAN implementation for nodes/devices.

See [history file](history.md) for releases.

[![Build Status](https://travis-ci.org/cjhdev/lora_device_lib.svg?branch=master)](https://travis-ci.org/cjhdev/lora_device_lib)

## Features

- LoRaWAN 1.1
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
    - DeviceTimeReq/Ans
    - ADRParamSetupReq/Ans
    - RekeyInd/Conf
- Supported regions (run-time option)
    - EU_868_870
    - EU_433
    - US_902_928
    - AU_915_928
- Supported radios (run-time option)
    - SX1272
    - SX1276
- Build [options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html)
- [API documentation](https://cjhdev.github.io/lora_device_lib_api/)
- Examples
    - [Arduino wrapper](wrappers/arduino/output/arduino_ldl)
    - [bare metal](examples/doxygen/example.c)
    
## Limitations

- Class B and C not supported
- GFSK modulation not supported
- ABP not supported
- rejoin not supported yet
- experimental

## Getting Started

- [porting guide](porting.md)
- [API documentation](https://cjhdev.github.io/lora_device_lib_api/)
- [design goals](design_goals.md)

## License

MIT
