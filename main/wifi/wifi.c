#include "esp_wifi.h"
#include "wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include <string.h>

#define DEBUG_LOG "***** DEBUG *****"
#define LOG_TAG "wifi"
#define BUFF_SIZE 1024

char ap_str[BUFF_SIZE];
int scan_result = ESP_FAIL;

void wifi_scan_all()
{
    uint16_t ap_count = 0;
    wifi_ap_record_t *ap_list;
    uint8_t i;
    // char *authmode;

    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count <= 0)
    {

        ESP_LOGE(LOG_TAG, "No Wifi networks found");
        return;
    }

    ap_list = (wifi_ap_record_t *)malloc(ap_count * sizeof(wifi_ap_record_t));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

    // printf("======================================================================\n");
    // printf("             SSID             |    RSSI    |           AUTH           \n");
    // printf("======================================================================\n");
    ap_str[0] = '\0';
    char seperator[2] = ",";
    for (i = 0; i < ap_count; i++)
    {
        if (strstr(ap_str, (char*)ap_list[i].ssid) == NULL)
        {

            if (strlen(ap_str) + strlen(seperator) + strlen((char *)ap_list[i].ssid) <= BUFF_SIZE)
            {
                strcat(ap_str, (char *)ap_list[i].ssid);
                if (strlen(ap_str) + strlen(seperator) + strlen((char *)ap_list[i + 1].ssid) <= BUFF_SIZE && i != (ap_count - 1))
                {
                    strcat(ap_str, seperator);
                }
            }
        }
        // switch (ap_list[i].authmode)
        // {
        // case WIFI_AUTH_OPEN:
        //     authmode = "WIFI_AUTH_OPEN";
        //     break;
        // case WIFI_AUTH_WEP:
        //     authmode = "WIFI_AUTH_WEP";
        //     break;
        // case WIFI_AUTH_WPA_PSK:
        //     authmode = "WIFI_AUTH_WPA_PSK";
        //     break;
        // case WIFI_AUTH_WPA2_PSK:
        //     authmode = "WIFI_AUTH_WPA2_PSK";
        //     break;
        // case WIFI_AUTH_WPA_WPA2_PSK:
        //     authmode = "WIFI_AUTH_WPA_WPA2_PSK";
        //     break;
        // default:
        //     authmode = "Unknown";
        //     break;
        // }
        // printf("%26.26s    |    % 4d    |    %22.22s\n", ap_list[i].ssid, ap_list[i].rssi, authmode);
    }
    free(ap_list);
    scan_result = ESP_OK;
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(LOG_TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(LOG_TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(LOG_TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        break;
    case SYSTEM_EVENT_SCAN_DONE:
        ESP_LOGI(LOG_TAG, "SYSTEM_EVENT_SCAN_DONE");
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_scan_all();
        break;
    default:
        break;
    }
    return ESP_OK;
}

void init_wifi()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void scan_for_wifi()
{
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE};
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    //invoke callback here
}

wifi_ap_scan_t get_aps()
{
    wifi_ap_scan_t scan_results = {
        .success = scan_result,
        .value = ap_str};
    return scan_results;
}