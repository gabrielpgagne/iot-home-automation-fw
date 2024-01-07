# iot-home-automation-fw
This is a door alarm for a fridge.

Uses a custom STM32WB5MMGH6TR board with the Zephyr SDK platform.
We transmit the state of this iot device using the BTHome protocol (<https://bthome.io/format/>).

Ref: <https://docs.zephyrproject.org/latest/boards/arm/nucleo_wb55rg/doc/nucleo_wb55rg.html>

## Preparation
You must flash the binary bluetooth stack code into the MCU (<https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB_BLE_Hardware_Setup>) using STM32CubeProgrammer utility.

For the present version, we need to flash (Zephyr limit which BT stack we can use):
```STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/stm32wb5x_BLE_HCILayer_fw.bin```
The start address is 0x080E0000 for our MCU.

To get the address value, look into STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/Release_Notes.html

## Compile
We use zephyr SDK (<https://www.zephyrproject.org>) to compile the source.
An introduction: <https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/develop/west/basics.html>.

Activate the west environement, then in the Zephyr directory, 

    cd workspace
    west init
    west update
    west build -b nucleo_wb55rg -p always ../zephyr/
    west -z /Volumes/External/zephyrproject/zephyr  build -b nucleo_wb55rg -p always ../zephyr/
