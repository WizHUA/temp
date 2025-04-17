### 一、整体功能概述
此代码基于 STM32 微控制器实现了超声波测距功能，并通过串口将测量结果输出。主要步骤包括初始化超声波模块、定时器和串口，触发超声波发射，测量超声波往返时间，计算距离并判断是否在有效范围内，最后将结果通过串口发送。

### 二、代码详细分析

#### 1. 引脚定义
```c
// 定义超声波模块引脚
#define TRIG_PIN GPIO_Pin_8
#define TRIG_GPIO_PORT GPIOC
#define ECHO_PIN GPIO_Pin_6
#define ECHO_GPIO_PORT GPIOC
```


这里定义了超声波模块的触发引脚（Trig）和回声引脚（Echo）及其对应的 GPIO 端口。

#### 2. 延时函数c
```c
// 延时函数（微秒级）
void delay_us(uint32_t us) {
    uint32_t i;
    for (i = 0; i < us * 72 / 5; i++);
}
```
该函数通过简单的 for 循环实现微秒级延时，用于控制超声波触发信号的时长。

#### 3. 超声波模块初始化
```c
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
```
此函数用于初始化超声波模块的引脚。开启 GPIOC 时钟，将 Trig 引脚配置为推挽输出，用于发送触发信号；将 Echo 引脚配置为浮空输入，用于接收超声波返回信号。
#### 4. 触发超声波模块
```c
// 触发超声波模块
void SR04_Trigger(void) {
    GPIO_SetBits(TRIG_GPIO_PORT, TRIG_PIN);
    delay_us(10);  // 发送 10us 高电平
    GPIO_ResetBits(TRIG_GPIO_PORT, TRIG_PIN);
}
```
该函数通过给 Trig 引脚发送一个 10us 的高电平脉冲来触发超声波模块发射超声波。

#### 5. 测量距离
```c
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
```
该函数先调用 SR04_Trigger 触发超声波发射，然后等待 Echo 引脚变高记录起始时间，再等待 Echo 引脚变低记录结束时间，计算时间差得到超声波传播时间。根据物理公式 $s = vt$（其中 $s$ 为距离，$v$ 为声速，$t$ 为时间），由于超声波是往返的路程，所以实际距离 $$d=\frac{vt}{2}$$
这里声速 $v = 340m/s = 34000cm/s$，时间 $t$ 的单位是微秒（\(\mu s\)），换算成秒需要除以 \(10^6\)，所以距离计算公式为 \(d=\frac{34000\times t}{2\times10^6}=\frac{t\times340}{2\times10000}\)。

#### 6. 定时器初始化
```c
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
```
此函数初始化 TIM2 定时器，设置其分辨率为 1us，然后启动定时器，用于精确测量超声波传播时间。

#### 7. 串口初始化
```c// 初始化 USART1 串口
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
```
该函数初始化 USART1 串口，配置其波特率、数据位、停止位等参数，然后启用串口，用于将测量结果发送到电脑。
#### 8. 重定向 printf 函数
```c
// 重定向 printf 到 USART1
int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}
```
此函数将 printf 函数的输出重定向到 USART1 串口，方便使用 printf 发送数据。

#### 9. 主函数
```c
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
```
主函数中先初始化各个外设，然后进入无限循环，不断调用 SR04_MeasureDistance 测量距离并判断是否在有效范围内，最后通过串口输出结果，每次测量后延时 100ms。

### 三、使用方法
1. 将代码下载到 STM32 开发板。
2. 按照硬件连接说明连接超声波模块
- Trig 接 PC8
- Echo 接 PC6
- gnd 接 电源模块-GND
- vcc 接 电源模块-5V
3. 打开串口助手，设置波特率为 9600，即可看到实时测距结果。