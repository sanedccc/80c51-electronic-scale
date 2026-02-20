#include "Key.h"
#include <INTRINS.H> 

// 无按键返回值
#define KEY_NONE 99

// 记录上一次的按键扫描结果，用于去抖
static unsigned char LastKey = 0;
// 记录按键按下的持续时间（次数）
static unsigned char KeyPressCount = 0;

/*
 * 内部函数：扫描矩阵键盘硬件状态
 * 返回值：0 (无按键) 或 键值 (1~16)
 * 注意：这里不处理逻辑，只读硬件
 */
static unsigned char Key_Driver_Read(void)
{
    unsigned char KeyNumber = 0;

    // --- 全低扫描，快速判断是否有键 ---
    ROW1 = 0; ROW2 = 0; ROW3 = 0; ROW4 = 0;
    COL1 = 1; COL2 = 1; COL3 = 1; COL4 = 1;

    if (COL1 == 0 || COL2 == 0 || COL3 == 0 || COL4 == 0)
    {
        // 确实有键按下，进行逐行扫描
        // 第一行
        ROW1 = 0; ROW2 = 1; ROW3 = 1; ROW4 = 1;
        if (COL1 == 0) KeyNumber = 7;
        if (COL2 == 0) KeyNumber = 8;
        if (COL3 == 0) KeyNumber = 9;
        if (COL4 == 0) KeyNumber = 10; // 去皮

        // 第二行
        ROW1 = 1; ROW2 = 0; ROW3 = 1; ROW4 = 1;
        if (COL1 == 0) KeyNumber = 4;
        if (COL2 == 0) KeyNumber = 5;
        if (COL3 == 0) KeyNumber = 6;
        if (COL4 == 0) KeyNumber = 11; // 清空

        // 第三行
        ROW1 = 1; ROW2 = 1; ROW3 = 0; ROW4 = 1;
        if (COL1 == 0) KeyNumber = 1;
        if (COL2 == 0) KeyNumber = 2;
        if (COL3 == 0) KeyNumber = 3;
        if (COL4 == 0) KeyNumber = 12; // 删除

        // 第四行
        ROW1 = 1; ROW2 = 1; ROW3 = 1; ROW4 = 0;
        if (COL1 == 0) KeyNumber = 14; // Mode
        if (COL2 == 0) KeyNumber = 0;
        if (COL3 == 0) KeyNumber = 15; // Dot
        if (COL4 == 0) KeyNumber = 13; // DHT11
    }
    
    return KeyNumber;
}


unsigned char Key_Scan(void)
{
    unsigned char CurKey;
    
    CurKey = Key_Driver_Read();

    if (CurKey != 0) 
    {
        if (CurKey == LastKey) 
        {
            KeyPressCount++;
            if (KeyPressCount == 2) 
            {
                return CurKey; 
            }
        }
        else 
        {
            KeyPressCount = 0;
        }
    }
    else 
    {
        KeyPressCount = 0;
    }
    
    LastKey = CurKey;
    return KEY_NONE; 
}