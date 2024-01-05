# iot-home-automation-fw
This is a door alarm for a fridge.

Uses a custom STM32WB5MMGH6TR board with the Zephyr SDK platform.

## Preparation
You must flash the binary bluetooth stack code into the MCU (<https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB_BLE_Hardware_Setup>) using STM32CubeProgrammer utility.

For the present version, we need to flash
STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/stm32wb5x_BLE_Stack_full_fw.bin
The start address is 0x080CE000 for our MCU.

To get the address value, look into STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/Release_Notes.html

## Compile
We use zephyr SDK (<https://www.zephyrproject.org>) to compile the source.