#ifndef __DISPLAY_H__
#define __DISPLAY_H__

void display_init();

void display_update(uint8_t value1, uint8_t value1GaugeWarning, 
                    uint8_t value2, uint8_t value2GaugeWarning, 
                    float value3, float value3GaugeWarning, 
                    uint8_t alarm, uint8_t selectedValue,
                    wchar_t * menuMessage);

#endif