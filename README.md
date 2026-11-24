# Smart DMX

Smart DMX is ESP32 firmware that exposes a three-channel RGB DMX fixture to Homey as a Matter over Wi-Fi **Extended Color Light**. It uses Homey's built-in Matter support. Homey can control power, brightness, hue, saturation and color temperature. Color temperature is simulated with RGB mixing.

**Signal path** · Homey → Matter over Wi-Fi → ESP32 → RS-485 transmitter → DMX fixture

The target setup is an ESP32 DevKit V1 with 4 MB flash and an ADJ 64P LED PRO in three-channel RGB mode. The default firmware runs in dry-run mode and logs the RGB output over USB. You can pair the ESP32 and test the controls before attaching a DMX transmitter.

| Feature | Validation |
| --- | --- |
| RGB conversion and DMX frame mapping | Covered by host-side C++ tests |
| Homey SHS pairing and direct lamp controls | Verified with dry-run firmware |
| Reconnection after an ESP32 restart | Verified with dry-run firmware |
| Physical DMX output and Homey Flow actions | Pending hardware and Flow validation |

## Build your own

- [Step-by-step tutorial](docs/setup.md) for installing the pinned tools, flashing the ESP32, pairing in Homey and testing the lamp.
- [Parts list and wiring diagram](docs/hardware.md) for the ESP32 DevKit V1, RS-485 interface and American DJ 64P LED PRO.

Start with dry-run mode if you only have the ESP32 and a USB data cable. Add the transmitter after the Matter controls work in Homey.

## Run the host tests

From the repository root with a C++17 compiler:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  firmware/matter-dmx/test/light_model_test.cpp -o /tmp/studio-dmx-light-model-test
/tmp/studio-dmx-light-model-test
```

[GitHub Actions](.github/workflows/matter.yml) runs these tests and builds both firmware configurations. See [Contributing](CONTRIBUTING.md) for validation expectations.

## License

Smart DMX uses the [MIT License](LICENSE). ESP-IDF, ESP-Matter and their dependencies retain their respective licenses.
