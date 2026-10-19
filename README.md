# Variometer firmware

ESP32 firmware for my independently developed paragliding variometer, using ESP-IDF and FreeRTOS for sensor fusion, flight state, an e-ink interface, and flight logging.

**Start with [VariometerV2](https://github.com/oiver12/VariometerV2)** for the project overview, PCB designs, architecture, screen captures, simulation/replay commands, and build/flash notes. Clone it recursively to obtain the hardware archive and firmware together.

`main/` contains the sensor drivers, quaternion and Kalman estimation, shared state, rendering/screens/widgets, IGC logging, airspace parsing, experimental BLE, and watchdog integration. Credits are in [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt).

Build with **ESP-IDF v5.1.4**, targeting **ESP32-S3**. The SDK is pinned in `main/idf_component.yml` and `dependencies.lock`; the firmware was compiled successfully with this version. Follow the main repository’s setup and flash instructions. The instrument has been tested by the author, including in flight.

My original software is licensed under [MIT](LICENSE). Third-party code keeps its own licenses, including GPL-3.0-or-later for the FIFO. Preserve upstream notices; the combined tree is not uniformly MIT-licensed.
