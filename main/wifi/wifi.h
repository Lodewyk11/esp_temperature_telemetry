
#ifndef _WIFI_H
#define _WIFI_H

void init_wifi(void);
char* get_aps_json(void);
int connect_to_ap(char *ssid, uint8_t channel, char *password);

#endif