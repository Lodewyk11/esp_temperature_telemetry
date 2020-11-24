#ifndef bool
#define  bool _Bool
#endif

#ifndef _TEMPERATURE_H
#define _TEMPERATURE_H

typedef struct  {
  bool success;
  int value;
} temperature_reading_t;

temperature_reading_t get_temperature_in_c();

#endif