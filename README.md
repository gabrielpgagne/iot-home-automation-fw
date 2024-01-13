# iot-home-automation-fw
This is a door alarm for a fridge.

Uses a custom STM32WB5MMGH6TR board with the Zephyr SDK platform.
We transmit the state of this iot device using the BTHome protocol (<https://bthome.io/format/>).

Ref: <https://docs.zephyrproject.org/latest/boards/arm/nucleo_wb55rg/doc/nucleo_wb55rg.html>

## Toolchain configuration

### MCU Preparation
## Toolchain configuration

### MCU Preparation
You must flash the binary bluetooth stack code into the MCU (<https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WB_BLE_Hardware_Setup>) using STM32CubeProgrammer utility.

For the present version, we need to flash (Zephyr limit which BT stack we can use):
```STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/stm32wb5x_BLE_HCILayer_fw.bin```
The start address is 0x080E0000 for our MCU.

To get the address value, look into 
STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/Release_Notes.html

### Zephyr toolchain
Refer to "Install dependencies" in <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>
to get started.

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

Then, refer to "Install the Zephyr SDK" in <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>
in order to install the different MCU compilers and utilities

### vscode integration
(ref: https://github.com/beriberikix/zephyr-vscode-example)

Turn on Compilation Database:

    west config build.cmake-args -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

(?) Copy the .code-workspace for your OS to samples/basic/thread as well the .vscode folder (?)
Retrieve ZEPHYR_SDK_INSTALL_DIR with cmake -P zephyr/cmake/verify-toolchain.cmake. It'll be the prefix of the C_Cpp.default.compilerPath
To get the address value, look into 
STM32CubeWB/Projects/STM32WB_Copro_Wireless_Binaries/STM32WB5x/Release_Notes.html

### Zephyr toolchain
Refer to "Install dependencies" in <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>
to get started.

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

Then, refer to "Install the Zephyr SDK" in <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>
in order to install the different MCU compilers and utilities

### vscode integration
(ref: https://github.com/beriberikix/zephyr-vscode-example)

Turn on Compilation Database:

    west config build.cmake-args -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

(?) Copy the .code-workspace for your OS to samples/basic/thread as well the .vscode folder (?)
Retrieve ZEPHYR_SDK_INSTALL_DIR with cmake -P zephyr/cmake/verify-toolchain.cmake. It'll be the prefix of the C_Cpp.default.compilerPath

## Compile
We use zephyr SDK (<https://www.zephyrproject.org>) to compile the source.
An introduction: <https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/develop/west/basics.html>.

Activate the west environement, then in the Zephyr directory, 

    cd workspace
    west init
    west update
    west build -b nucleo_wb55rg -p always application/
    // west -z /Volumes/External/zephyrproject/zephyr  build -b nucleo_wb55rg -p always ../zephyr/

## VSCode integration

ref:  https://github.com/beriberikix/zephyr-vscode-example
ref: https://zmk.dev/docs/development/ide-integration

This is an example configuration for setting up Visual Studio Code for Zephyr application development. It was originally created for the talk entitled, "[Zephyr & Visual Studio Code: How to Develop Zephyr Apps with a Modern, Visual IDE](https://youtu.be/IKNHPmG-Qxo)" at EOSS 2023.

## Example details

There are many, many different ways to develop a Zephyr application. This example currently assumes:

* All paths follow the Getting Started Guide
* Uses the Zephyr SDK where possible (ex. Host tools are not available on macOS or Windows)
* In-tree development
* Use of Python Virtual Environments
* Building `samples/basic/thread` so we can demonstrate thread-aware debugging
* Targetting an nRF52840 DK 

## Steps

1. Follow the Zephyr [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) for your OS and make sure to use virtual environments
2. Turn on Compilation Database with `west config build.cmake-args -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
3. Copy the `.code-workspace` for your OS to `samples/basic/thread` as well the `.vscode` folder
4. Retrieve `ZEPHYR_SDK_INSTALL_DIR` with `cmake -P zephyr/cmake/verify-toolchain.cmake`. It'll be the prefix of the `C_Cpp.default.compilerPath`
5. Set the `Python: Interpreter Path`
6. CD to `samples/basic/thread`
7. Try the different build tasks and the flash task
8. Try to connect over the serial terminal
9. TODO try step debugging

## Credits

https://zmk.dev/docs/development/ide-integration