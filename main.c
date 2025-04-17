#include "stm32f10x.h"
#include <stdio.h>

// 定义超声波模块引脚
#define TRIG_PIN GPIO_Pin_8
#define TRIG_GPIO_PORT GPIOC
#define ECHO_PIN GPIO_Pin_6
#define ECHO_GPIO_PORT GPIOC

// 延时函数（微秒级）
void delay_us(uint32_t us) {
    uint32_t i;
    for (i = 0; i < us * 72 / 5; i++);
}

// 超声波模块初始化
void SR04_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // 开启 GPIOC 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // 配置 Trig 引脚为推挽输出
    GPIO_InitStructure.GPIO_Pin = TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TRIG_GPIO_PORT, &GPIO_InitStructure);

    // 配置 Echo 引脚为浮空输入
    GPIO_InitStructure.GPIO_Pin = ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(ECHO_GPIO_PORT, &GPIO_InitStructure);
}

// 触发超声波模块
void SR04_Trigger(void) {
    GPIO_SetBits(TRIG_GPIO_PORT, TRIG_PIN);
    delay_us(10);  // 发送 10us 高电平
    GPIO_ResetBits(TRIG_GPIO_PORT, TRIG_PIN);
}

// 测量距离
float SR04_MeasureDistance(void) {
    uint32_t start_time, end_time, duration;
    float distance;

    // 触发超声波
    SR04_Trigger();

    // 等待 Echo 引脚变高
    while (GPIO_ReadInputDataBit(ECHO_GPIO_PORT, ECHO_PIN) == 0);
    start_time = TIM_GetCounter(TIM2);

    // 等待 Echo 引脚变低
    while (GPIO_ReadInputDataBit(ECHO_GPIO_PORT, ECHO_PIN) == 1);
    end_time = TIM_GetCounter(TIM2);

    // 计算持续时间
    duration = end_time - start_time;

    // 计算距离（单位：cm）
    distance = (float)duration * 340 / 2 / 10000;

    return distance;
}

// 初始化 TIM2 定时器
void TIM2_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    // 开启 TIM2 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 配置 TIM2
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;  // 1us 分辨率
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // 启动 TIM2
    TIM_Cmd(TIM2, ENABLE);
}

// 初始化 USART1 串口
void USART1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 开启 GPIOA 和 USART1 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // 配置 USART1 Tx (PA9)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 USART1 Rx (PA10)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 USART1 参数
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    // 启用 USART1
    USART_Cmd(USART1, ENABLE);
}

// 重定向 printf 到 USART1
int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

int main(void) {
    float distance;

    // 初始化外设
    SR04_Init();
    TIM2_Init();
    USART1_Init();

    while (1) {
        // 测量距离
        distance = SR04_MeasureDistance();

        // 判断距离范围并输出
        if (distance >= 5 && distance <= 200) {
            printf("Distance: %.2f cm\r\n", distance);
        } else {
            printf("Out of range!\r\n");
        }

        // 延时
        delay_us(100000);  // 延时 100ms
    }
}
