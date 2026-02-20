#include <REGX52.H>

sbit LCD_RS=P2^6;
sbit LCD_RW=P2^5;
sbit LCD_EN=P2^7;
#define LCD_DataPort P0

// 10的幂查表法，避免循环计算
static code unsigned long PowersOf10[] = {1UL, 10UL, 100UL, 1000UL, 10000UL, 100000UL};
#define LCD_Pow10(Y) PowersOf10[Y]

sbit LCD_BL=P2^3; 

void LCD_Delay50us(void)	
{
	unsigned char i;
	i=20;
	while(--i);
}

void LCD_Delay2ms(void)	
{
	unsigned char i,j;
	i=4;
	j=146;
	do
	{
		while(--j);
	}while(--i);
}

void LCD_WriteCommand(unsigned char Command)
{
	LCD_RS=0;
	LCD_RW=0;
	LCD_DataPort=Command;
	LCD_EN=1;
	LCD_Delay50us();
	LCD_EN=0;
	LCD_Delay50us();
}

void LCD_WriteData(unsigned char Data)
{
	LCD_RS=1;
	LCD_RW=0;
	LCD_DataPort=Data;
	LCD_EN=1;
	LCD_Delay50us();
	LCD_EN=0;
	LCD_Delay50us();
}

void LCD_SetCursor(unsigned char Line,unsigned char Column)
{
	if(Line==1)
	{
		LCD_WriteCommand(0x80|(Column-1));
	}
	else if(Line==2)
	{
		LCD_WriteCommand(0x80|(Column-1+0x40));
	}
}

void LCD_Init()
{
	LCD_WriteCommand(0x38);	//八位数据接口，两行显示，5*7点阵
	LCD_WriteCommand(0x0C);	//显示开，光标关，闪烁关
	LCD_WriteCommand(0x06);	//数据读写操作后，光标自动加一，画面不动
	LCD_WriteCommand(0x01);	//光标复位，清屏
	LCD_Delay2ms();	//清屏指令执行需要较长时间，需要较长的延时
    LCD_BL = 1;     // 初始化默认开启背光
}

void LCD_ShowChar(unsigned char Line,unsigned char Column,char Char)
{
	LCD_SetCursor(Line,Column);
	LCD_WriteData(Char);
}

void LCD_ShowString(unsigned char Line,unsigned char Column,char *String)
{
	unsigned char i;
	LCD_SetCursor(Line,Column);
	for(i=0;String[i]!='\0';i++)
	{
		LCD_WriteData(String[i]);
	}
}

// 保留通用版本用于十六进制和二进制显示
unsigned long LCD_Pow(unsigned char X, unsigned char Y)
{
	unsigned char i;
	unsigned long Result = 1;
	for(i = 0; i < Y; i++)
	{
		Result *= X;
	}
	return Result;
}

// 优化：使用查表法显示数字，性能更好
void LCD_ShowNum(unsigned char Line, unsigned char Column, unsigned int Number, unsigned char Length)
{
	unsigned char i;
	LCD_SetCursor(Line, Column);
	for(i = Length; i > 0; i--)
	{
		LCD_WriteData(Number / LCD_Pow10(i - 1) % 10 + '0');
	}
}

void LCD_ShowSignedNum(unsigned char Line,unsigned char Column,int Number,unsigned char Length)
{
	unsigned char i;
	unsigned int Number1;
	LCD_SetCursor(Line,Column);
	if(Number>=0)
	{
		LCD_WriteData('+');
		Number1=Number;
	}
	else
	{
		LCD_WriteData('-');
		Number1=-Number;
	}
	for(i=Length;i>0;i--)
	{
		LCD_WriteData(Number1/LCD_Pow(10,i-1)%10+'0');
	}
}

void LCD_ShowHexNum(unsigned char Line,unsigned char Column,unsigned int Number,unsigned char Length)
{
	unsigned char i,SingleNumber;
	LCD_SetCursor(Line,Column);
	for(i=Length;i>0;i--)
	{
		SingleNumber=Number/LCD_Pow(16,i-1)%16;
		if(SingleNumber<10)
		{
			LCD_WriteData(SingleNumber+'0');
		}
		else
		{
			LCD_WriteData(SingleNumber-10+'A');
		}
	}
}

void LCD_ShowBinNum(unsigned char Line,unsigned char Column,unsigned int Number,unsigned char Length)
{
	unsigned char i;
	LCD_SetCursor(Line,Column);
	for(i=Length;i>0;i--)
	{
		LCD_WriteData(Number/LCD_Pow(2,i-1)%2+'0');
	}
}

// 优化：使用查表法显示长整数
void LCD_ShowLongNum(unsigned char Line, unsigned char Column, unsigned long Number, unsigned char Length)
{
	unsigned char i;
	for(i = 0; i < Length; i++)
	{
		LCD_ShowChar(Line, Column + i, Number / LCD_Pow10(Length - i - 1) % 10 + '0');
	}
}

void LCD_ShowFloatNum(unsigned char Line,unsigned char Column,float Number,unsigned char IntLength,unsigned char FraLength,unsigned char IsShowSign)
{
	unsigned long PowNum,IntNum,FraNum;
	unsigned char Offset=0;

	if(IsShowSign){Offset=1;}
	
	if(Number>=0)	
	{
		if(IsShowSign)
		{
			LCD_ShowChar(Line,Column,'+');	
		}
	}
	else	
	{
		if(IsShowSign)
		{
			LCD_ShowChar(Line,Column,'-');	
		}
		Number=-Number;	
	}


	IntNum=Number;	
	Number-=IntNum;	
	PowNum=LCD_Pow(10,FraLength);	
	FraNum=Number*PowNum*10;	
	FraNum+=5;	
	FraNum/=10;	
	IntNum+=FraNum/PowNum;	

	LCD_ShowLongNum(Line,Column+Offset,IntNum,IntLength);

	LCD_ShowChar(Line,Column+IntLength+Offset,'.');

	LCD_ShowLongNum(Line,Column+IntLength+1+Offset,FraNum,FraLength);
}

void LCD_Clear(void)
{
	LCD_WriteCommand(0x01);
	LCD_Delay2ms();
}

void LCD_IsShowCursor(unsigned char State)
{
	if(State){LCD_WriteCommand(0x0E);}
	else{LCD_WriteCommand(0x0C);}
}

void LCD_SetBacklight(unsigned char State)
{
    if(State)
    {
        LCD_BL = 1; // 开背光 
    }
    else
    {
        LCD_BL = 0; // 关背光
    }
}