# Contributing

Run the [C++ host tests](README.md#run-the-host-tests) and build the dry-run and physical firmware variants when changing the firmware. Record any hardware or Homey validation you performed. Start Homey tests with the dry-run variant and check the USB logs before testing a fixture.

The Matter task reads committed attributes and queues the desired lamp state. A separate output task converts the state to RGB and owns DMX timing. Keep changes to timing independent from Matter callbacks.

Use synthetic or redacted logs in issues and pull requests. Keep local provisioning data and onboarding codes private.
