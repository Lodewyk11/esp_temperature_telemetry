#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "temperature/temperature.h"
#include "bluetooth/bluetooth.h"
#include "flash/flash.h"
#include "wifi/wifi.h"

/**
 * Temperature telemetry task. 
 */
void temperature_telemetry(void *params)
{
  while (true)
  {
    temperature_reading_t temperature_reading = get_temperature_in_c();
    if (temperature_reading.success == true)
    {
      printf("Temperature is %d deg C\n", temperature_reading.value);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void app_main(void)
{
  init_flash();
  init_wifi();
  init_ble();

  //xTaskCreate(&temperature_telemetry, "Temperature Telemetry", 2048, "Temperature Telemetry", 2, NULL);
}
