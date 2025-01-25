# Jirachi GUI

The Jirachi GUI is a cross-platform GUI made in Rust to control and tune the PID controller inside of the jirachi device. It is written around the [iced GUI framework](https://github.com/iced-rs/iced) and the [btleplug library](https://github.com/deviceplug/btleplug/tree/master) to easily control the jirachi device and let the GUI app handle all the BLE communication.

## Quick Start

```bash
$ cargo run --release
```

### TODO: add pretty gif of the application and more documentation