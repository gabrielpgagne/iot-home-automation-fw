# Basement temperature & humidity sensor

A BTHome v2 sensor built on the `doorio` board: the on-board SHTC3 is read every
five minutes and the reading is broadcast in the BLE advertisement, where Home
Assistant picks it up. Advertising stays connectable so OTA DFU keeps working.

This branch shares `boards/gabiche/doorio` with the other application branches
(`temp-based` for the freezer alarm, `switch-based` for the door sensor); only
`src/` and `prj.conf` differ.

## What it advertises

The BTHome v2 service data (UUID `0xFCD2`, unencrypted, regular updates) carries,
in ascending object-id order:

| Object id | Field               | Encoding              |
| --------- | ------------------- | --------------------- |
| `0x00`    | Packet id           | uint8, wraps at 255   |
| `0x01`    | Battery             | uint8, percent        |
| `0x02`    | Temperature         | sint16, 0.01 °C       |
| `0x03`    | Humidity            | uint16, 0.01 %RH      |
| `0x16`    | Battery charging    | uint8, 0/1            |

The packet id lets Home Assistant drop duplicated adverts and notice updates it
missed. With the device name `basement` the whole advertisement is 30 of the 31
available bytes — lengthening `CONFIG_BT_DEVICE_NAME` will overflow it.

Battery percent is a crude voltage-to-percent curve over the nPM1300's measured
cell voltage (see `src/charger.c`), not a real fuel gauge.

## Behaviour

- Three green blinks at boot, then an immediate first reading so Home Assistant
  discovers the device without waiting out an interval.
- A reading every `MEASUREMENT_INTERVAL` (`src/main.c`, five minutes by default).
- Pressing the user button (`sw0`) takes a reading immediately.
- The green activity LED only blinks while USB is connected — on battery it
  would cost milliamps for nobody to see.
- Plugging USB in lights the red LED and brings up the CDC ACM console.

## SDK & IDE setup

Follow Nordic's [instructions](https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK/GetStarted#infotabs)
to set up NCS. Built and tested against NCS v2.9.2.

## Building

Board: `doorio/nrf52833`, with sysbuild enabled (MCUboot comes from
`sysbuild.conf`). The board definition lives in this repo, so the application
directory has to be on the board root:

```sh
west build -b doorio/nrf52833 --sysbuild . -- -DBOARD_ROOT=$PWD
```

In the nRF Connect VS Code extension, add the application as a build
configuration and pick `doorio/nrf52833`; the extension finds `boards/` on its
own.

See [this tutorial](https://www.digikey.com/en/maker/tutorials/2025/introduction-to-zephyr-part-12-how-to-create-a-custom-board-definition)
and [bl653](https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/ezurio/bl653_dvk/bl653_dvk.dts)
for how the board definition was put together.

## Flashing

Use an nRF dev kit's onboard debugger, for example a _PCA10056_ (nRF52840 DK).

## Over-the-air update

BLE OTA DFU allows flashing new firmware without a debugger.

**This only works if an MCUboot-enabled and `CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y`
firmware was previously flashed over JTAG.**

After building, transfer `build/iot-home-automation-fw/zephyr/zephyr.signed.bin`
to your phone. Power up the board, open nRF Connect Mobile, connect to
`basement`, and in the DFU tab pick the signed binary.

## Home Assistant

The BTHome integration auto-discovers the device once an ESPHome Bluetooth proxy
or a local adapter sees the advert; accept the discovery for `basement`. No
encryption key is configured, so there is nothing to paste.

## Virtual COM port

With proper assembly (good luck), the board opens a virtual COM port (USB CDC
ACM, e.g. `/dev/ttyACM0`) carrying the logs.
