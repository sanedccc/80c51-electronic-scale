#include <AT89X52.H>

sbit DS1302_SCLK=P3^6;
sbit DS1302_IO=P3^4;
sbit DS1302_CE=P3^5;

#define DS1302_SECOND		0x80
#define DS1302_MINUTE		0x82
#define DS1302_HOUR			0x84
#define DS1302_DATE			0x86
#define DS1302_MONTH		0x88
#define DS1302_DAY			0x8A
#define DS1302_YEAR			0x8C
#define DS1302_WP			0x8E

// BCD 转换宏定义
#define DEC_TO_BCD(dec) (((dec)/10)*16 + ((dec)%10))
#define BCD_TO_DEC(bcd) (((bcd)/16)*10 + ((bcd)%16))

unsigned char DS1302_Time[]={26,01,05,14,30,00,1};


void DS1302_Init(void)
{
	DS1302_CE=0;
	DS1302_SCLK=0;
}

void DS1302_WriteByte(unsigned char Command, unsigned char Data)
{
	unsigned char i;
	DS1302_CE=1;
	for(i=0;i<8;i++)
	{
		DS1302_IO=Command&(0x01<<i);
		DS1302_SCLK=1;
		DS1302_SCLK=0;
	}
	for(i=0;i<8;i++)
	{
		DS1302_IO=Data&(0x01<<i);
		DS1302_SCLK=1;
		DS1302_SCLK=0;
	}
	DS1302_CE=0;
}


unsigned char DS1302_ReadByte(unsigned char Command)
{
	unsigned char i,Data=0x00;
	Command|=0x01;	
	DS1302_CE=1;
	for(i=0;i<8;i++)
	{
		DS1302_IO=Command&(0x01<<i);
		DS1302_SCLK=0;
		DS1302_SCLK=1;
	}
	DS1302_IO=1;
	for(i=0;i<8;i++)
	{
		DS1302_SCLK=1;
		DS1302_SCLK=0;
		if(DS1302_IO){Data|=(0x01<<i);}
	}
	DS1302_CE=0;
	DS1302_IO=0;	
	return Data;
}

void DS1302_SetTime(void)
{
	DS1302_WriteByte(DS1302_WP, 0x00);
	DS1302_WriteByte(DS1302_YEAR, DEC_TO_BCD(DS1302_Time[0]));
	DS1302_WriteByte(DS1302_MONTH, DEC_TO_BCD(DS1302_Time[1]));
	DS1302_WriteByte(DS1302_DATE, DEC_TO_BCD(DS1302_Time[2]));
	DS1302_WriteByte(DS1302_HOUR, DEC_TO_BCD(DS1302_Time[3]));
	DS1302_WriteByte(DS1302_MINUTE, DEC_TO_BCD(DS1302_Time[4]));
	DS1302_WriteByte(DS1302_SECOND, DEC_TO_BCD(DS1302_Time[5]));
	DS1302_WriteByte(DS1302_DAY, DEC_TO_BCD(DS1302_Time[6]));
	DS1302_WriteByte(DS1302_WP, 0x80);
}


void DS1302_ReadTime(void)
{
	DS1302_Time[0] = BCD_TO_DEC(DS1302_ReadByte(DS1302_YEAR));
	DS1302_Time[1] = BCD_TO_DEC(DS1302_ReadByte(DS1302_MONTH));
	DS1302_Time[2] = BCD_TO_DEC(DS1302_ReadByte(DS1302_DATE));
	DS1302_Time[3] = BCD_TO_DEC(DS1302_ReadByte(DS1302_HOUR));
	DS1302_Time[4] = BCD_TO_DEC(DS1302_ReadByte(DS1302_MINUTE));
	DS1302_Time[5] = BCD_TO_DEC(DS1302_ReadByte(DS1302_SECOND));
	DS1302_Time[6] = BCD_TO_DEC(DS1302_ReadByte(DS1302_DAY));
}
