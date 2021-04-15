#ifndef __GAUGE_H__
#define __GAUGE_H__

#include <stdint.h>
#define GAUGE_DATA_MAGIC_VALUE 0x1A2B3C4D

typedef struct _GaugeData_t
{
    uint32_t Magic;
    float WaterTemperature;
    float OilTemperature;
    float OilPressure;
}
GaugeData_t;

#endif