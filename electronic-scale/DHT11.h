#ifndef _DHT11_H_
#define _DHT11_H_

/**********************************
包含头文件
**********************************/
#include "AT89X52.h"
#include <intrins.h>

/**********************************
类型定义
**********************************/
#ifndef uint
typedef unsigned int uint;
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

/**********************************
重定义关键词
**********************************/
#define DHT11_NOACK 0
#define DHT11_ACK   1
#define DHT11_STATUS_REG_W 0x06
#define DHT11_STATUS_REG_R 0x07
#define DHT11_MEASURE_TEMP 0x03
#define DHT11_MEASURE_HUMI 0x05
#define DHT11_RESET        0x1e


/**********************************
PIN口定义
**********************************/
sbit DHT11_SCK = P2^1;				//DHT11时钟引脚P1.5
sbit DHT11_DATA = P2^2;				//DHT11数据引脚P1.6


/**********************************
变量定义
**********************************/
enum { TEMP, HUMI };

typedef union               //定义共用同类型  
{
	unsigned int i;
	float f;
} value;


/**********************************
函数声明
**********************************/
void Dht11_Connection_Reset(void);             							//DHT11连接复位函数
bit Dht11_Get_Temp_Humi_Value(uint *temp,uint *humi);			//DHT11获取温湿度值函数，返回1成功0失败


#endif