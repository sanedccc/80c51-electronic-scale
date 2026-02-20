
#ifndef __Key_H__
#define __Key_H__

#include "AT89X52.h"     

sbit ROW1 = P1^7;   
sbit ROW2 = P1^6;   
sbit ROW3 = P1^5;   
sbit ROW4 = P1^4;   

sbit COL1 = P1^3;   
sbit COL2 = P1^2;   
sbit COL3 = P1^1;   
sbit COL4 = P1^0;   

unsigned char Key_Scan(void);

#endif