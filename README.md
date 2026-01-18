# IoT Fridge V2

The V2 adopts many improvements:

- Feather format
- nRF MCU
- USBC connector with CDC ACM (virtual COM port)
- LiPo battery charging (nPM1300)
- Nordic Connect SDK

## SDK & IDE setup

Follow Nordic's [instructions](https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK/GetStarted#infotabs) to setup NCS. I use their VSCode extension for very easy development.

## Creating the board

See [this tutorial](https://www.digikey.com/en/maker/tutorials/2025/introduction-to-zephyr-part-12-how-to-create-a-custom-board-definition) and refer to [bl653](https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/ezurio/bl653_dvk/bl653_dvk.dts)

## Build configuration

**TODO** create a custom zephyr board instead of overlaying over a DK.

In the vscode extension:

- Board: `bl653_dvk/nrf52833`
- Base Devicetree overlays: `board.overlay`
- Extra Devicetree overlays: `config.overlay`
  
Leave the rest as is. Press _Generate and build_.

## Flashing

Use a nRF dev kit's onboard debugger. For example, _PCA10056_ aka _nRF52840 DK_.

## Over-the-air update

BLE Over-the-air (OTA) Direct Firmware Upgrade (DFU) allows flashing new firmware without a wired connection to a debugger. 

**This only works if an MCUBoot-abled and `CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y` firmware has been uploaded previous via JTAG to the board.**

After compiling the project, a `build/npm1300_one_button/zephyr/zephyr.signed.bin` file should have been created. Transfer it to your phone. 

Power up the board. Open NRF Connect Mobile on your phone and connect to the board. In the DFU tab's document picker, select the signed binary firmware image, press start and voilà. 

## Virtual COM port

With proper assembly (good luck), the board should open a virtual COM port (USB CDC ACM) (e.g., `/dev/ttyACM0`). You can open it and read the logs freely.