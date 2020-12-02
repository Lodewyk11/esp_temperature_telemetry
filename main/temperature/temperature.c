#include <ds18x20.h>
#include "temperature.h"

static const gpio_num_t SENSOR_GPIO = 5;

/**
 * Gets the temperature from the sensor connected to GPIO pin 5.
 * @returns A temperature_reading_t struct. If the read was successful the 
 * success value will equal 1 (true).
 */
temperature_reading_t get_temperature_in_c()
{
    temperature_reading_t reading;
    ds18x20_addr_t addrs[1];
    float temps[1];
    int sensor_count;

    sensor_count = ds18x20_scan_devices(SENSOR_GPIO, addrs, 1);
    if (sensor_count < 1)
    {
        reading.success = ESP_FAIL;
        reading.value = 0;
    }
    else
    {
        if (sensor_count > 1)
        {
            sensor_count = 1;
        }
        ds18x20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count, temps);
        reading.success = ESP_OK;
        reading.value = (int)temps[0];
    }
    return reading;
}
