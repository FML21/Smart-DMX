# Security

Smart DMX uses authenticated Matter commissioning and local control. The default ESP-Matter configuration uses development commissioning and attestation data. Use unique commissioning credentials and suitable attestation provisioning before routine deployment. The firmware is a development accessory with no Matter certification, OTA updates, secure boot or flash encryption configured.

Keep Wi-Fi credentials, pairing codes, private keys, serial logs and device flash dumps private. Routine updates can preserve Wi-Fi and Matter state. A factory reset removes both from the ESP32.

For a security issue, contact the repository owner privately. Include the firmware version and affected component. Redact credentials from any reproduction details.
