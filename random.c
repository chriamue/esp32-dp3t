#include <Arduino.h>

void sys_random(uint8_t *buf, int len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        buf[i] = (uint8_t)(esp_random());
    }
}