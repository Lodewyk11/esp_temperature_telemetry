
#ifndef _WIFI_H
#define _WIFI_H

typedef struct  {
  bool success;
  char* value;
} wifi_ap_scan_t;

void init_wifi(void);
void scan_for_wifi(void);
wifi_ap_scan_t get_aps(void);

#endif