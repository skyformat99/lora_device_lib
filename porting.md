Porting Guide
=============

1. Review Doxygen groups

- [system](https://cjhdev.github.io/lora_device_lib_api/group__ldl__system.html)
- [radio connector](https://cjhdev.github.io/lora_device_lib_api/group__ldl__radio__connector.html)
- [build options](https://cjhdev.github.io/lora_device_lib_api/group__ldl__build__options.html)

2. Implement the following system functions

- LDL_System_getIdentity()
- LDL_System_ticks()
- LDL_System_tps()
- LDL_System_eps()

3. Define the following macros

- LORA_SYSTEM_ENTER_CRITICAL()
- LORA_SYSTEM_LEAVE_CRITICAL()

4. Define at least one region

- LORA_ENABLE_EU_863_870
- LORA_ENABLE_US_902_928
- LORA_ENABLE_AU_915_928
- LORA_ENABLE_EU_433

5. Define at least one radio driver

- LORA_ENABLE_SX1272
- LORA_ENABLE_SX1276

6. Connect radio driver to SPI by implementing radio adapter interfaces

- LDL_SPI_select()
- LDL_SPI_reset()
- LDL_SPI_write()
- LDL_SPI_read()

7. Connect the radio chip interrupt signal to LDL_MAC_interrupt()
