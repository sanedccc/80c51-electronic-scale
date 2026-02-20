#ifndef __BUZZER_H__
#define __BUZZER_H__

// 初始化蜂鸣器
void Buzzer_Init(void);

// 关闭蜂鸣器
void Buzzer_Off(void);

// 发出报警声 (freq_delay: 音调值越小声音越尖, duration_ms: 持续时间)
void Buzzer_Alarm(unsigned char freq_delay, unsigned int duration_ms);

#endif