#include "AT89X52.h"
#include "LCD1602.h"
#include "Key.h"
#include "DS1302.h"
#include "DHT11.h"
#include "Buzzer.h"
#include <intrins.h>

#define KEY_TARE      10
#define KEY_CLEAR     11
#define KEY_BACKSPACE 12
#define KEY_DHT11     13
#define KEY_MODE      14
#define KEY_DOT       15
#define KEY_NONE      99

#define Multiple 405.40
#define MULTIPLE_X100 40540L

#define FILTER_SIZE 8

#define HX711_READY_STABLE_COUNT   3
#define HX711_WAIT_READY_TIMEOUT   250000UL
#define HX711_SPIKE_THRESHOLD_COUNTS  5000000L
#define HX711_SAT_MARGIN_COUNTS     1000L

// 重量噪声阈值，低于此值视为0
#define WEIGHT_NOISE_THRESHOLD 1

sbit HX711_OUT = P3^2;
sbit HX711_SCK = P3^3;

volatile unsigned char g_tick10ms = 0;   
volatile unsigned char g_evt200ms = 0;   
volatile unsigned char g_evt500ms = 0;   

volatile unsigned long g_uptime10ms = 0;

#define SCREEN_OFF_TIMEOUT_TICKS 500UL
#define RAW_ACTIVITY_THRESHOLD   800L

static bit idata g_screenOff = 0;

static unsigned long idata g_lastActivityTick10ms = 0;
static long idata g_lastTotalForActivity          = 0;

long idata FurResult = 0;
long idata TotalResult = 0;
long idata NetWeight = 0;
unsigned long idata NetWeightAbs = 0;

long xdata Weight_Buffer[FILTER_SIZE] = {0};
unsigned char Filter_Index = 0;
bit g_isFirstSample = 1;

static long idata g_lastRawValid = 0;
static bit  idata g_hasLastRaw  = 0;

unsigned long idata UnitPrice_x100 = 0;

char idata InputBuf[8] = {0};
unsigned char idata InputIndex = 0;

static signed char idata g_dotPos = -1;      
static unsigned char idata g_fracDigits = 0; 
static bit idata g_inputDirty = 0;
static bit idata g_priceDirty = 0;

unsigned char idata SystemMode = 0;

unsigned int idata year;
unsigned char idata month, day, hour, minute, second;

unsigned int idata dht_temp = 0;
unsigned int idata dht_humi = 0;
unsigned char idata dht_div = 0;

static long idata lastNetWeight = 0x7FFFFFFF;
static unsigned long idata lastUnitPrice_x100 = 0xFFFFFFFFUL;
static unsigned long idata lastTotal_fen = 0xFFFFFFFFUL; 
static bit idata lastWeightNeg = 0;
static unsigned int idata last_dht_temp = 0xFFFF;
static unsigned int idata last_dht_humi = 0xFFFF;

void Timer0_Init(void);
void Delay(unsigned int t);
unsigned long GetWeight_NoWait(void);
long Filter_Weight(long raw);

static bit  HX711_IsReadyStable(void);
static bit  HX711_WaitReady(unsigned long timeout);
static long HX711_ReadRawSafe(void);
static long HX711_ReadRawSpikeRejected(void);
static void ResetInput(void);
static void ShowInputStringIfDirty(void);
static unsigned long ParsePriceToX100(const char* str);
static void Init_Display(void);
static void ProcessKey(unsigned char key);
static long CalcNetWeightFromDiff(long diff);
static void UpdateScaleDisplay(void);
static void UpdateScaleLogicOnly(void);
static void UpdateClockDisplay(void);
static void UpdateDHTDisplay(void);

static void Screen_Off(void);
static void Screen_On(void);
static void MarkActivity(void);
static unsigned long ReadUptime10msSafe(void);

void Timer0_Init(void)
{
    TMOD &= 0xF0;
    TMOD |= 0x01;      
    TL0 = 0x00;
    TH0 = 0xDC;
    TF0 = 0;
    TR0 = 1;
    ET0 = 1;
    EA  = 1;
}

void Delay(unsigned int t)
{
    unsigned char i, j;
    while(t--)
    {
        i = 2; j = 199;
        do { while (--j); } while (--i);
    }
}

unsigned long GetWeight_NoWait(void)
{
    unsigned long Temp = 0;
    unsigned char i;
    bit ea_bak = EA;

    HX711_SCK = 0;
    Temp = 0;

    for(i = 0; i < 24; i++)
    {
        EA = 0;
        HX711_SCK = 1; _nop_();
        Temp <<= 1;
        if(HX711_OUT) Temp++;
        HX711_SCK = 0; _nop_();
        EA = ea_bak;
    }

    EA = 0;
    HX711_SCK = 1; _nop_();
    HX711_SCK = 0; _nop_();
    EA = ea_bak;

    if(Temp & 0x00800000) Temp |= 0xFF000000;
    return Temp;
}

static bit HX711_IsReadyStable(void)
{
    unsigned char i;
    for(i = 0; i < HX711_READY_STABLE_COUNT; i++)
    {
        if(HX711_OUT) return 0;
        _nop_(); _nop_();
    }
    return 1;
}

static bit HX711_WaitReady(unsigned long timeout)
{
    while(timeout--)
    {
        if(!HX711_OUT)
        {
            if(HX711_IsReadyStable()) return 1;
        }
    }
    return 0;
}

static bit HX711_IsSaturatedLike(long raw)
{
    if(raw > (8388607L - HX711_SAT_MARGIN_COUNTS)) return 1;
    if(raw < (-8388608L + HX711_SAT_MARGIN_COUNTS)) return 1;

    if(raw == -1L) return 1;

    return 0;
}

static long HX711_ReadRawSafe(void)
{
    if(!HX711_WaitReady(HX711_WAIT_READY_TIMEOUT))
    {
        return g_hasLastRaw ? g_lastRawValid : 0L;
    }
    return (long)GetWeight_NoWait();
}

static long HX711_ReadRawSpikeRejected(void)
{
    long raw = HX711_ReadRawSafe();

    if(HX711_IsSaturatedLike(raw))
    {
        return g_hasLastRaw ? g_lastRawValid : raw;
    }

    if(!g_hasLastRaw)
    {
        g_lastRawValid = raw;
        g_hasLastRaw = 1;
        return raw;
    }

    if(raw > g_lastRawValid)
    {
        if((raw - g_lastRawValid) > HX711_SPIKE_THRESHOLD_COUNTS) return g_lastRawValid;
    }
    else
    {
        if((g_lastRawValid - raw) > HX711_SPIKE_THRESHOLD_COUNTS) return g_lastRawValid;
    }

    g_lastRawValid = raw;
    return raw;
}

long Filter_Weight(long raw)
{
    long sum = 0;
    unsigned char i;
    
    if (g_isFirstSample)
    {
        for(i=0; i<FILTER_SIZE; i++) Weight_Buffer[i] = raw;
        g_isFirstSample = 0;
        return raw;
    }

    Weight_Buffer[Filter_Index] = raw;
    Filter_Index++;
    if (Filter_Index >= FILTER_SIZE) Filter_Index = 0;

    for(i=0; i<FILTER_SIZE; i++)
    {
        sum += Weight_Buffer[i];
    }
    
    return (sum / FILTER_SIZE);
}

static void ResetInput(void)
{
    InputIndex = 0;
    InputBuf[0] = '\0';
    g_dotPos = -1;
    g_fracDigits = 0;
    UnitPrice_x100 = 0;
    g_inputDirty = 1;
    g_priceDirty = 0;
}

static unsigned long ParsePriceToX100(const char* str)
{
    unsigned long val = 0;
    unsigned char i = 0;
    bit afterDot = 0;
    unsigned char frac = 0;

    while(str[i] != '\0')
    {
        if(str[i] == '.') afterDot = 1;
        else if(str[i] >= '0' && str[i] <= '9')
        {
            if(!afterDot || frac < 2)
            {
                val = val * 10UL + (unsigned long)(str[i] - '0');
                if(afterDot) frac++;
            }
        }
        i++;
    }

    if(!afterDot) return val * 100UL;
    if(frac == 0) return val * 100UL;
    if(frac == 1) return val * 10UL;
    return val;
}

static void ShowInputStringIfDirty(void)
{
    if(!g_inputDirty) return;
    g_inputDirty = 0;
    LCD_ShowString(2, 3, "      ");
    if(InputIndex == 0) LCD_ShowString(2, 3, "00.00");
    else LCD_ShowString(2, 3, InputBuf);
}

static void Init_Display(void)
{
    LCD_Clear();
    Buzzer_Off();
    
    lastNetWeight = 0x7FFFFFFF;
    lastUnitPrice_x100 = 0xFFFFFFFFUL;
    lastTotal_fen = 0xFFFFFFFFUL;
    lastWeightNeg = 0;
    last_dht_temp = 0xFFFF;
    last_dht_humi = 0xFFFF;

    if (SystemMode == 0)
    {
        LCD_ShowString(1, 1, "W:");
        LCD_ShowChar(1, 3, ' ');
        LCD_ShowString(1, 9, "g ");
        LCD_ShowString(2, 1, "U:");
        LCD_ShowString(2, 9, "T:");
        LCD_ShowChar(2, 14, '.');
        ResetInput();
        ShowInputStringIfDirty();
        UpdateScaleDisplay();
    }
    else if (SystemMode == 1)
    {
        LCD_ShowChar(1, 8, '-');
        LCD_ShowChar(1, 11, '-');
        LCD_ShowChar(2, 6, ':');
        LCD_ShowChar(2, 9, ':');
        UpdateClockDisplay();
    }
    else
    {
        LCD_ShowString(1, 1, "Temp: --.- C");
        LCD_ShowString(2, 1, "Humi: --.- %");
        dht_div = 1;
    }
}

static void ProcessKey(unsigned char key)
{
    unsigned char removed;

    if(key == KEY_MODE)
    {
        SystemMode = (SystemMode == 1) ? 0 : 1;
        Init_Display();
        return;
    }
    if(key == KEY_DHT11)
    {
        SystemMode = (SystemMode == 2) ? 0 : 2;
        Init_Display();
        return;
    }

    if(SystemMode != 0) return;

    if(key <= 9)
    {
        if(g_dotPos >= 0 && g_fracDigits >= 2) return;
        if(InputIndex < 6)
        {
            InputBuf[InputIndex++] = (char)('0' + key);
            InputBuf[InputIndex] = '\0';
            if(g_dotPos >= 0) g_fracDigits++;
            g_inputDirty = 1;
            g_priceDirty = 1;
        }
        return;
    }

    if(key == KEY_DOT)
    {
        if(g_dotPos >= 0) return;
        if(InputIndex == 0)
        {
            if(InputIndex < 5) { InputBuf[InputIndex++] = '0'; InputBuf[InputIndex] = '\0'; }
            else return;
        }
        if(InputIndex < 6)
        {
            g_dotPos = (signed char)InputIndex;
            InputBuf[InputIndex++] = '.';
            InputBuf[InputIndex] = '\0';
            g_fracDigits = 0;
            g_inputDirty = 1;
            g_priceDirty = 1;
        }
        return;
    }

    if(key == KEY_TARE)
    {
        FurResult = TotalResult;
        lastNetWeight = 0x7FFFFFFF;
        return;
    }

    if(key == KEY_CLEAR)
    {
        ResetInput();
        return;
    }

    if(key == KEY_BACKSPACE)
    {
        if(InputIndex == 0) return;
        removed = (unsigned char)InputBuf[InputIndex - 1];
        InputIndex--;
        InputBuf[InputIndex] = '\0';
        if(removed == '.') { g_dotPos = -1; g_fracDigits = 0; }
        else { if(g_dotPos >= 0 && InputIndex > (unsigned char)g_dotPos && g_fracDigits > 0) g_fracDigits--; }
        g_inputDirty = 1;
        g_priceDirty = 1;
        return;
    }
}

static long CalcNetWeightFromDiff(long diff)
{
    long num = diff * 100L;
    if(num >= 0) num += (MULTIPLE_X100 / 2);
    else         num -= (MULTIPLE_X100 / 2);
    return (num / MULTIPLE_X100);
}

static void UpdateScaleDisplay(void)
{
    long WeightDiff;
    unsigned long total_fen;
    unsigned int TotalInt, TotalDec;
    bit neg;

    ShowInputStringIfDirty();

    if(g_priceDirty)
    {
        g_priceDirty = 0;
        UnitPrice_x100 = (InputIndex == 0) ? 0 : ParsePriceToX100(InputBuf);
    }

    WeightDiff = TotalResult - FurResult;
    NetWeight = CalcNetWeightFromDiff(WeightDiff);
    
    if(NetWeight >= -WEIGHT_NOISE_THRESHOLD && NetWeight <= WEIGHT_NOISE_THRESHOLD) NetWeight = 0;
    if(g_screenOff) { Buzzer_Off(); }
    else
    {
        if(NetWeight > 1000) Buzzer_Alarm(20, 50);
        else Buzzer_Off();
    }

    neg = (NetWeight < 0) ? 1 : 0;
    NetWeightAbs = (neg) ? (unsigned long)(-NetWeight) : (unsigned long)NetWeight;

    if(neg != lastWeightNeg)
    {
        LCD_ShowChar(1, 3, neg ? '-' : ' ');
        lastWeightNeg = neg;
    }

    if(NetWeight != lastNetWeight)
    {
        LCD_ShowNum(1, 4, NetWeightAbs, 5);
        lastNetWeight = NetWeight;
    }

    if(!neg) total_fen = (unsigned long)NetWeightAbs * (unsigned long)UnitPrice_x100 / 1000UL;
    else total_fen = 0;

    if(total_fen != lastTotal_fen || UnitPrice_x100 != lastUnitPrice_x100)
    {
        lastTotal_fen = total_fen;
        lastUnitPrice_x100 = UnitPrice_x100;
        TotalInt = (unsigned int)(total_fen / 100UL);
        TotalDec = (unsigned int)(total_fen % 100UL);
        LCD_ShowNum(2, 11, TotalInt, 3);
        LCD_ShowNum(2, 15, TotalDec, 2);
    }
}

static void UpdateScaleLogicOnly(void)
{
    long WeightDiff;
    WeightDiff = TotalResult - FurResult;
    NetWeight = CalcNetWeightFromDiff(WeightDiff);
    if(NetWeight >= -WEIGHT_NOISE_THRESHOLD && NetWeight <= WEIGHT_NOISE_THRESHOLD) NetWeight = 0;
    Buzzer_Off();
}

static void UpdateClockDisplay(void)
{
    DS1302_ReadTime();
    year   = 2000 + DS1302_Time[0];
    month  = DS1302_Time[1];
    day    = DS1302_Time[2];
    hour   = DS1302_Time[3];
    minute = DS1302_Time[4];
    second = DS1302_Time[5];

    LCD_ShowNum(1, 4, year, 4);
    LCD_ShowNum(1, 9, month, 2);
    LCD_ShowNum(1, 12, day, 2);
    LCD_ShowNum(2, 4, hour, 2);
    LCD_ShowNum(2, 7, minute, 2);
    LCD_ShowNum(2, 10, second, 2);
}

static void UpdateDHTDisplay(void)
{
    if(dht_div == 0) { dht_div = 1; return; }
    dht_div = 0;

    Dht11_Get_Temp_Humi_Value(&dht_temp, &dht_humi);

    if(dht_temp != last_dht_temp)
    {
        last_dht_temp = dht_temp;
        LCD_ShowNum(1, 7, dht_temp/10, 2);
        LCD_ShowChar(1, 9, '.');
        LCD_ShowNum(1, 10, dht_temp%10, 1);
    }
    if(dht_humi != last_dht_humi)
    {
        last_dht_humi = dht_humi;
        LCD_ShowNum(2, 7, dht_humi/10, 2);
        LCD_ShowChar(2, 9, '.');
        LCD_ShowNum(2, 10, dht_humi%10, 1);
    }
}

static unsigned long ReadUptime10msSafe(void)
{
    unsigned long t;
    bit ea_bak = EA;
    EA = 0;
    t = g_uptime10ms;
    EA = ea_bak;
    return t;
}

static void MarkActivity(void)
{
    g_lastActivityTick10ms = ReadUptime10msSafe();
}

static void Screen_Off(void)
{
    if(g_screenOff) return;
    g_screenOff = 1;
    Buzzer_Off();
    LCD_Clear();
}

static void Screen_On(void)
{
    g_screenOff = 0;
    Init_Display();
    MarkActivity();
}

void main(void)
{
    unsigned char key;
    unsigned char timeFlag;
    long rawWeight;
    unsigned char k;

    HX711_SCK = 0;
    Buzzer_Init();
    LCD_Init();

    LCD_ShowString(1, 1, "Name: xxx");
    LCD_ShowString(2, 1, "ID: 222xxxxxxx");
    
    for(k=0; k<3; k++)
    {
        Delay(1000); 
    }
    
    DS1302_Init();

    timeFlag = DS1302_ReadByte(0xC0);
    if(timeFlag != 0xAA)
    {
        DS1302_SetTime();
        DS1302_WriteByte(0x8E, 0x00);
        DS1302_WriteByte(0xC0, 0xAA);
        DS1302_WriteByte(0x8E, 0x80);
        Delay(500);
    }

    Timer0_Init();

    HX711_WaitReady(HX711_WAIT_READY_TIMEOUT);

    rawWeight = HX711_ReadRawSafe();
    FurResult = Filter_Weight(rawWeight);
    g_lastRawValid = rawWeight;
    g_hasLastRaw = 1;

    for(key=0; key<10; key++)
    {
        rawWeight = HX711_ReadRawSafe();
    }
    rawWeight = HX711_ReadRawSafe();
    FurResult = Filter_Weight(rawWeight);
    g_lastRawValid = rawWeight;

    SystemMode = 0;
    Init_Display();

    g_screenOff = 0;
    g_lastActivityTick10ms = g_uptime10ms;
    g_lastTotalForActivity = TotalResult;

    {
        unsigned char lastTick = g_tick10ms;

        while(1)
        {
            while(lastTick != g_tick10ms)
            {
                lastTick++;

                key = Key_Scan();
                if(key != KEY_NONE)
                {
                    if(g_screenOff) Screen_On();
                    ProcessKey(key);
                    MarkActivity();
                }

                if(SystemMode == 0 && HX711_IsReadyStable())
                {
                    rawWeight = HX711_ReadRawSpikeRejected();
                    TotalResult = Filter_Weight(rawWeight);
                    {
                        long d = TotalResult - g_lastTotalForActivity;
                        if(d > RAW_ACTIVITY_THRESHOLD || d < -RAW_ACTIVITY_THRESHOLD)
                        {
                            g_lastTotalForActivity = TotalResult;
                            if(g_screenOff) Screen_On();
                            MarkActivity();
                        }
                    }
                }
            }

            while(g_evt200ms)
            {
                g_evt200ms--;
                if(SystemMode == 0)
                {

                    if(g_screenOff)
                    {
                        UpdateScaleLogicOnly();
                    }
                    else
                    {
                        UpdateScaleDisplay();
                    }
                }
            }

            while(g_evt500ms)
            {
                g_evt500ms--;
                if(g_screenOff) {  }
                else
                {
                    if(SystemMode == 1) UpdateClockDisplay();
                    else if(SystemMode == 2) UpdateDHTDisplay();
                }
            }

            if(!g_screenOff)
            {
                unsigned long now = ReadUptime10msSafe();
                if((now - g_lastActivityTick10ms) >= SCREEN_OFF_TIMEOUT_TICKS)
                {
                    Screen_Off();
                }
            }

        }
    }
}

void Timer0_Routine(void) interrupt 1
{
    static unsigned char cnt200 = 0;
    static unsigned char cnt500 = 0;

    TL0 = 0x00;
    TH0 = 0xDC; 

    g_tick10ms++;
    g_uptime10ms++;
    cnt200++;
    cnt500++;

    if(cnt200 >= 20) { cnt200 = 0; g_evt200ms++; }
    if(cnt500 >= 50) { cnt500 = 0; g_evt500ms++; }
}
