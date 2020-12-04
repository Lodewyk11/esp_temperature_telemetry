#include "esp_wifi.h"
#include "wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include <string.h>

#define LOG_TAG "wifi"

#define DEFAULT_SCAN_LIST_SIZE 5
#define BUFF_SIZE 200 * DEFAULT_SCAN_LIST_SIZE
static char ap_json[BUFF_SIZE];

static char *get_auth_mode(int authmode)
{
    char *str = (char *)malloc(100 * sizeof(char));
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


void init_wifi()
{
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_scan()
{
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
}

char* get_aps_json()
{
    wifi_scan();
    char *str = (char *)malloc(BUFF_SIZE * sizeof(char));
    strcpy(str, ap_json);
    return str;
}