
# Temperature telemetry

This project showcases how to read a temperature value from a ds18x20 temperature sensor and push the temperature to AWS IoT for further processing.

BLE (Bluetooth Low Energy) using NimBLE is used to configure the device intially. This initial configuration includes:
1. Setting scanning for available Wifi access points and returning this to the paired device.
1. Getting the password and selected wifi SSID from the paired device.

After initial setup the device connects to Wifi and provisions with AWS. Flash memory is used to save device state.

## Flashing the ESP32

1. From the `IDF_PATH` run the following command to export the require IDF variables to to the `PATH`
```bash
. export.sh
```
1. Navige to the root of the project and run the following:

```bash
idf.py -p [your com port] flash monitor
```
Where the port indicates the port where the ESP32 is connected (i.e. /dev/ttyUSB0)
