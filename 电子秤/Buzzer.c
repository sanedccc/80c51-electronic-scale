#include "AT89X52.h"
#include "Buzzer.h"
#include <intrins.h>

sbit BUZZER_PIN = P2^5;

void Buzzer_Init(void)
{
    BUZZER_PIN = 0;
}

void Buzzer_Off(void)
{
    BUZZER_PIN = 0;
}

void Buzzer_DelayShort(unsigned char t)
{
    while(t--)
    {
        _nop_();
        _nop_();
    }
}

void Buzzer_Alarm(unsigned char freq_delay, unsigned int duration_ms)
{
    unsigned int i;
    unsigned long cycles = (unsigned long)duration_ms * 100 / freq_delay;

    for(i = 0; i < cycles; i++)
    {
        BUZZER_PIN = !BUZZER_PIN;
        Buzzer_DelayShort(freq_delay);
    }
    BUZZER_PIN = 0;
}
