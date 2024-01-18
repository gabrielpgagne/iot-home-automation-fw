# iot-home-automation-fw
This is a door alarm for a fridge.

Uses a custom STM32WB5MMGH6TR board with the Zephyr SDK platform.

We transmit the state of this iot device using the BTHome protocol (<https://bthome.io/format/>).

We use VS Code as an IDE.

## References

- Repo organisation reference: <https://github.com/zephyrproject-rtos/example-application>
- STM32 support in Zephyr: <https://docs.zephyrproject.org/latest/boards/arm/nucleo_wb55rg/doc/nucleo_wb55rg.html>


## Toolchain configuration

### MCU Preparation

The binary bluetooth stack code must be flashed into the MCU (<https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB_BLE_Hardware_Setup>) using STM32CubeProgrammer utility.

For this project, we need to flash (Zephyr limit which BT stack we can use):
```STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/stm32wb5x_BLE_HCILayer_fw.bin```
The start address is 0x080E0000 for our MCU.

To get the address value, look into 
STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/Release_Notes.html

### Zephyr toolchain
Refer to "Install dependencies" in <https://docs.zephyrproject.org/latest/develop/getting_started/index.html> to get started.

Then, for an intree configuration:

    cd <this_solution_directory>
    python3 -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    pip install west
    west init .
    west update
    west zephyr-export
    pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt

Do not forget to register the environement in VSCode.

Then, refer to "Install the Zephyr SDK" in <https://docs.zephyrproject.org/latest/develop/getting_started/index.html> in order to install the different MCU compilers and utilities

### vscode integration
ref: https://github.com/beriberikix/zephyr-vscode-example
ref: https://zmk.dev/docs/development/ide-integration

Turn on Compilation Database:

    west config build.cmake-args -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

Verify the paths in the files in .vscode.

## Compile

The VSCode Tasks and Launch menus can be used to compile and flash the code.

It is also possible to do it manually:

- To build the code:

    west build -b nucleo_wb55rg -p always application/
    // west -z /Volumes/External/zephyrproject/zephyr  build -b nucleo_wb55rg -p always application

- To flash:

    west flash


## Credits

* https://github.com/beriberikix/zephyr-vscode-example
* https://zmk.dev/docs/development/ide-integration
