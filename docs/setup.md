# Smart DMX build tutorial

This guide starts with an ESP32 on USB, then adds the physical DMX fixture. Allow time and disk space for the ESP-Matter SDK and its submodules. The 4 MB requirement below applies to the **ESP32 flash**.

## 1. Gather the hardware

The reference setup uses Homey Self-Hosted Server, an ESP32 DevKit V1 with an ESP-WROOM-32 module, and an American DJ 64P LED PRO fixture. Start with the board and its micro-USB data cable. See the [parts list and wiring diagram](hardware.md) for the RS-485 transmitter, connector and DMX cable.

## 2. Install the toolchain

Use Linux or WSL2. On Ubuntu, install the build prerequisites:

```sh
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 \
  build-essential pkg-config libdbus-1-dev libglib2.0-dev unzip
```

Install ESP-IDF **v5.4.1** for the classic `esp32` target and ESP-Matter at the pinned revision. Do this in a directory where you want to keep the SDKs:

```sh
git clone --branch v5.4.1 --recursive https://github.com/espressif/esp-idf.git
(cd esp-idf && ./install.sh esp32)
. ./esp-idf/export.sh

git clone --no-checkout https://github.com/espressif/esp-matter.git
cd esp-matter
git checkout 4957e8331fbbed67d7b52010458fa59eab82a921
git submodule update --init --depth 1
(cd connectedhomeip/connectedhomeip && ./scripts/checkout_submodules.py --platform esp32 linux --shallow)
./install.sh --no-host-tool
. ./export.sh
cd ..
```

For later terminal sessions, source both SDKs' `export.sh` files in the same order. Use their absolute paths if you are in another directory. WSL2 users need to make the USB serial device available to Linux before flashing.

## 3. Build and flash in dry-run mode

Connect the ESP32 by USB. Identify the serial port, commonly `/dev/ttyUSB0` for a CP2102 on Linux, and check the reported flash size. Use at least **4 MB**. Back up any existing firmware and device state you need before replacing it.

```sh
port=/dev/ttyUSB0
python -m esptool --port "$port" flash_id

git clone https://github.com/FML21/Smart-DMX.git
cd Smart-DMX/firmware/matter-dmx
idf.py -B build -DIDF_TARGET=esp32 build
idf.py -B build -p "$port" flash monitor
```

Use your own serial port in place of the example. `idf.py flash` uses the generated offsets for the bootloader, partition table and application. The serial console runs at 115200 baud. Expect a dry-run message and a Matter setup code. Keep the code and serial logs private. The dry-run firmware logs RGB values and leaves the DMX GPIO pins unused.

If the flash tool cannot connect, close other serial monitors, check your USB cable and port permissions, and retry. If the board does not enter the bootloader automatically, hold **BOOT**, tap **EN**, and release **BOOT** before retrying.

## 4. Pair and test in Homey

1. Put Homey and the ESP32 on the same local network for the first test. The ESP32 needs 2.4 GHz Wi-Fi. Local IPv6 and multicast discovery must be available. Keep your phone close to the ESP32 for Bluetooth commissioning.
2. In the Homey app, open **Devices**, choose **+**, then **New Device** and **Matter**. Follow the Matter setup prompts for your Homey and use the QR or manual code from the serial console. This is a development device with SDK test provisioning. See [Security](../SECURITY.md) before deploying it permanently.
3. Switch the light on, dim it, and try red, green, blue and color temperature. Look for `RGB … (dry-run)` in the console. At full brightness the active channels should be near their maximum values. Turning the light off should produce `RGB 0 0 0 (dry-run)`.
4. Reboot the board and verify that the existing Homey device reconnects in the off state. Turn it on again to confirm control.
5. To check your own Homey Flow, create a manual Flow with the lamp's **Turn on** action, then run it and check the device tile and serial output. Direct controls and restart recovery passed in the original dry-run test. Flow execution still needs a recorded test.

If pairing fails, check local discovery, 2.4 GHz Wi-Fi, and whether the device is already commissioned. For a deliberate fresh pairing, remove the device from Homey and use `matter esp factoryreset` in the serial shell. This erases saved Wi-Fi and Matter pairing data.

## 5. Add physical DMX

1. Follow the [wiring diagram](hardware.md#wiring-diagram) and set the ADJ fixture to three-channel RGB mode at address `001`.
2. Enable the physical firmware using the separate build directory and settings below. Run the command from `Smart-DMX/firmware/matter-dmx` with both SDKs active. Close the serial monitor before flashing.

   ```sh
   port=/dev/ttyUSB0
   idf.py -B build-hardware -DSDKCONFIG=sdkconfig.hardware.local \
     -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.hardware" \
     -DIDF_TARGET=esp32 -p "$port" flash monitor
   ```

3. Check for `RGB … (UART)` when changing the light in Homey. Check the fixture against the same on/off, brightness and color tests from the dry run. Verify the transmitter and timing before relying on the physical output.
4. If the console updates and the fixture stays dark, check the fixture mode and address, XLR pin numbering, data polarity, transmitter power and driver enable, and termination. If colors are crossed, test each RGB channel at full brightness and check the fixture is in three-channel mode.

The firmware defaults are TX **GPIO17**, active-high driver enable **GPIO21**, and DMX address **1**. To change them, use `menuconfig` with the same hardware build and configuration options, then open **Studio Matter DMX**.

## References

- [ESP-IDF v5.4.1 installation guide](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/get-started/index.html)
- [Add a Matter device to Homey](https://support.homey.app/hc/en-us/articles/9075551662108-Add-a-Matter-device-to-Homey)
- [Pinned ESP-Matter build commands in CI](../.github/workflows/matter.yml)
