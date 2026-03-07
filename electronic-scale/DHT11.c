#include "DHT11.h"

void Delay_ms_DHT11(unsigned int ms)
{
    unsigned int i, j;
    for (i = ms; i > 0; i--)
        for (j = 110; j > 0; j--);
}

void Delay_10us_DHT11(void)
{
    _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
    _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
}

unsigned char DHT11_Read_Byte(void)
{
    unsigned char i, dat = 0;
    unsigned char timeout;

    for (i = 0; i < 8; i++)
    {
        dat <<= 1;

        timeout = 0;
        while (!DHT11_DATA)
        {
            if(++timeout > 100) break;
        }

        Delay_10us_DHT11();
        Delay_10us_DHT11();
        Delay_10us_DHT11();

        if (DHT11_DATA)
        {
            dat |= 1;

            timeout = 0;
            while (DHT11_DATA)
            {
                if(++timeout > 100) break;
            }
        }
    }
    return dat;
}

// 返回 1 表示读取成功，0 表示失败
bit Dht11_Get_Temp_Humi_Value(uint *temp, uint *humi)
{
    unsigned char rh_int, rh_dec, t_int, t_dec, sum;
    unsigned char timeout = 0;

    DHT11_DATA = 1;
    _nop_();
    DHT11_DATA = 0;
    Delay_ms_DHT11(20);
    DHT11_DATA = 1;

    Delay_10us_DHT11();
    Delay_10us_DHT11();
    Delay_10us_DHT11();

    if (!DHT11_DATA)
    {
        timeout = 0;
        while (!DHT11_DATA && ++timeout < 100);

        timeout = 0;
        while (DHT11_DATA && ++timeout < 100);

        EA = 0;

        rh_int = DHT11_Read_Byte();
        rh_dec = DHT11_Read_Byte();
        t_int  = DHT11_Read_Byte();
        t_dec  = DHT11_Read_Byte();
        sum    = DHT11_Read_Byte();

        EA = 1;

        Delay_10us_DHT11();
        DHT11_DATA = 1;

        if (sum == (rh_int + rh_dec + t_int + t_dec))
        {
            *humi = (unsigned int)rh_int * 10 + rh_dec;
            *temp = (unsigned int)t_int * 10 + t_dec;
            return 1;  // 读取成功
        }
    }
    return 0;  // 读取失败
}

void Dht11_Connection_Reset(void)
{
    DHT11_DATA = 1;
}
