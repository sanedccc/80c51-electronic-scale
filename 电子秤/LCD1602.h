#ifndef __LCD1602_H__
#define __LCD1602_H__

void LCD_WriteCommand(unsigned char Command);
void LCD_WriteData(unsigned char Data);
void LCD_SetCursor(unsigned char Line,unsigned char Column);
void LCD_Init();
void LCD_ShowChar(unsigned char Line,unsigned char Column,char Char);
void LCD_ShowString(unsigned char Line,unsigned char Column,char *String);
void LCD_ShowNum(unsigned char Line,unsigned char Column,unsigned int Number,unsigned char Length);
void LCD_ShowSignedNum(unsigned char Line,unsigned char Column,int Number,unsigned char Length);
void LCD_ShowHexNum(unsigned char Line,unsigned char Column,unsigned int Number,unsigned char Length);
void LCD_ShowBinNum(unsigned char Line,unsigned char Column,unsigned int Number,unsigned char Length);
void LCD_ShowLongNum(unsigned char Line,unsigned char Column,unsigned long Number,unsigned char Length);
void LCD_ShowFloatNum(unsigned char Line,unsigned char Column,float Number,unsigned char IntLength,unsigned char FraLength,unsigned char IsShowSign);
void LCD_Clear(void);
void LCD_IsShowCursor(unsigned char State);

#endif