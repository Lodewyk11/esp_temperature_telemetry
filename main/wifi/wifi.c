#include "esp_wifi.h"
#include "wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#define LOG_TAG "wifi"
#define DEBUG_LOG "***** DEBUG *****"

#define DEFAULT_SCAN_LIST_SIZE 5
#define BUFF_SIZE 200 * DEFAULT_SCAN_LIST_SIZE

static char ap_json[BUFF_SIZE];

xSemaphoreHandle connectionSemaphore;
xSemaphoreHandle scanningSemaphore;

static int8_t do_scan = 0;
static int8_t do_connect = 0;

static char *get_auth_mode(int authmode)
{
    char *str = (char *)malloc(30 * sizeof(char));
    switch (authmode)
    {
    case WIFI_AUTH_OPEN:
        strcpy(str, "WIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        strcpy(str, "WIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        strcpy(str, "WIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        strcpy(str, "WIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        strcpy(str, "WIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        strcpy(str, "WIFI_AUTH_WPA2_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        strcpy(str, "WIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        strcpy(str, "WIFI_AUTH_WPA2_WPA3_PSK");
        break;
    default:
        strcpy(str, "WIFI_AUTH_UNKNOWN");
        break;
    }
    return str;
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    if (do_scan == 1)
    {
        switch (event->event_id)
        {
        case SYSTEM_EVENT_STA_START:

            ESP_LOGI(LOG_TAG, "scanning...\n");
            uint16_t number = DEFAULT_SCAN_LIST_SIZE;
            wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
            uint16_t ap_count = 0;
            memset(ap_info, 0, sizeof(ap_info));

            ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

            memset(ap_json, 0, sizeof(ap_json));
            strcat(ap_json, "{\"ap_list\":[");
            for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
            {
                strcat(ap_json, "{");
                strcat(ap_json, "\"ssid\": \"");
                strcat(ap_json, (char *)ap_info[i].ssid);
                strcat(ap_json, "\",");

                char *str = (char *)malloc(100 * sizeof(char));
                strcat(ap_json, "\"channel\": ");
                sprintf(str, "%d", ap_info[i].primary);
                strcat(ap_json, str);
                strcat(ap_json, ",");

                free(str);

                strcat(ap_json, "\"auth_mode\": \"");
                char *auth_mode = get_auth_mode(ap_info[i].authmode);
                strcat(ap_json, auth_mode);
                strcat(ap_json, "\"");
                free(auth_mode);

                strcat(ap_json, "}");
                if (i != ap_count - 1 && i != DEFAULT_SCAN_LIST_SIZE - 1)
                {
                    strcat(ap_json, ",");
                }
            }
            strcat(ap_json, "]}");
            break;
        case WIFI_EVENT_SCAN_DONE:
            xSemaphoreGive(scanningSemaphore);
            break;
        default:
            break;
        }
    }
    else if (do_connect == 1)
    {
        switch (event->event_id)
        {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGI(LOG_TAG, "connecting...\n");
            break;

        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(LOG_TAG, "connected\n");
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(LOG_TAG, "got ip\n");
            xSemaphoreGive(connectionSemaphore);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(LOG_TAG, "disconnected\n");
            break;

        default:
            break;
        }
    }

    return ESP_OK;
}

void init_wifi()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

static void on_connected(void *para)
{
    while (true)
    {
        if (xSemaphoreTake(connectionSemaphore, 20000 / portTICK_RATE_MS))
        {
            ESP_LOGI(LOG_TAG, "connected");

            xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Failed to connect. Retry in");
            for (int i = 0; i < 5; i++)
            {
                ESP_LOGE(LOG_TAG, "...%d", i);
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
            esp_wifi_stop();
            esp_wifi_start();
        }
    }
}


int connect_to_ap(char *ssid, uint8_t channel, char *password)
{
    esp_wifi_stop();
    do_scan = 0;
    do_connect = 1;
    connectionSemaphore = xSemaphoreCreateBinary();
    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);
    wifi_config.sta.channel = channel;
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    esp_wifi_start();
    xTaskCreate(&on_connected, "on_connected", 1024 * 3, NULL, 5, NULL);
    return 0;
}

char *get_aps_json()
{
    esp_wifi_stop();
    do_scan = 1;
    do_connect = 0;
    scanningSemaphore = xSemaphoreCreateBinary();

    ESP_ERROR_CHECK(esp_wifi_start());
    while (true)
    {
        if (xSemaphoreTake(scanningSemaphore, 20000 / portTICK_RATE_MS))
        {
            ESP_LOGI(LOG_TAG, "scanning_complete");
            break;
        }
    }
    char *str = (char *)malloc(BUFF_SIZE * sizeof(char));
    strcpy(str, ap_json);
    return str;
}