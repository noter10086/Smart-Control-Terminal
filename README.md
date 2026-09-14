# Smart Control Terminal

基于 **STM32F407 + FreeRTOS** 的智能控制终端：以 ADC 采集外部模拟量为输入，经任务间通信传递至控制任务，映射为 PWM 占空比驱动执行器，形成一条完整的「采集 → 处理 → 输出」闭环链路，并配套任务监控与独立看门狗保障长期运行可靠性。

## 功能特性

- **闭环控制**：ADC 采集经 DMA 搬运，控制任务实时换算占空比驱动 PWM 输出
- **多任务架构**：基于 FreeRTOS 抢占式调度，采集 / 控制 / 日志 / 看门狗任务职责分离
- **任务健康监控**：每个业务任务上报「心跳」与「功能状态」，双重维度判定系统健康度
- **失效安全设计**：任一关键任务异常时停止喂狗，由 IWDG 触发 MCU 复位，避免系统挂死后失控
- **异步日志**：日志经队列解耦后由独立任务串口输出，不阻塞业务任务
- **复位原因回溯**：上电时读取并打印上次复位是否由看门狗触发

## 硬件平台

| 项目 | 参数 |
|---|---|
| MCU | STM32F407VET6（Cortex-M4F，512KB Flash / 192KB RAM，LQFP100） |
| 主频 | 168 MHz（HSE 8MHz 晶振，PLLM=4 / PLLN=168 / PLLP=2） |
| 总线时钟 | AHB 168MHz，APB1 42MHz，APB2 84MHz |
| RTOS | FreeRTOS Kernel V10.5.1 |
| 开发板 | 自定义板（`board=custom`） |

## 外设与引脚分配

| 引脚 | 外设 | 配置 | 说明 |
|---|---|---|---|
| PA0 | ADC1_IN0 | 12 位，采样时间 84 周期，连续转换 | 模拟量采集输入 |
| PA6 | TIM3_CH1 | PWM 输出，1 kHz | 驱动执行器，占空比 0–100% |
| PA9 | USART1_TX | 115200-8-N-1 | 日志输出（`printf` 重定向） |
| PA10 | USART1_RX | 115200-8-N-1 | 接收 |
| PC10 | GPIO_Output | 推挽输出 | 运行指示灯（500ms 翻转） |
| PA13 / PA14 | SYS | Serial Wire | SWD 调试下载 |

**外设关键参数**

- `TIM3` —— PWM 输出：预分频 84-1，自动重装载 1000-1，计数时钟 1MHz → **1 kHz PWM 频率**
- `TIM2` —— HAL 时基：作为 `HAL_IncTick()` 的时基源，将 SysTick 让渡给 FreeRTOS 使用
- `IWDG` —— 独立看门狗：LSI 32kHz 经 16 分频，重装载值 4095 → **超时约 2.05 s**
- `DMA2_Stream0` —— ADC1 循环模式搬运，半字对齐，硬件自动搬运避免 CPU 轮询

## 软件架构

### 任务划分

| 任务 | 优先级 | 栈（word） | 触发方式 | 职责 |
|---|---|---|---|---|
| `start_task` | 1 | 128 | 一次性 | 初始化监控/IPC/软件定时器，创建其余任务后自删除 |
| `log_task` | 2 | 128 | 队列驱动 | 从 `log_queue` 取出日志经 UART1 输出 |
| `watchdog_task` | 3 | 128 | 200 ms 周期 | 巡检全系统健康度，决定是否喂狗 |
| `collect_task` | 4 | 128 | 100 ms 周期 | 读取 ADC 值，投递至 `sensor_queue`，上报心跳 |
| `control_task` | 5 | 128 | 队列驱动 | 取出采样值换算占空比，调用 `BSP_PWM_SetDuty()` 输出 |

> 优先级数值越大优先级越高（FreeRTOS 约定）。

### 数据流

```
                    ┌──────────────┐
   PA0 ──► ADC1 ──► │ DMA2_Stream0 │ ──► adc_value
                    └──────────────┘          │
                                              ▼
  ┌───────────────┐   sensor_queue    ┌────────────────┐
  │ collect_task  │ ────────────────► │ control_task   │
  │   (100 ms)    │                   │                │
  └───────┬───────┘                   └────────┬───────┘
          │                                    │ duty = adc * 100 / 4095
          │ 心跳 / 功能上报                     ▼
          │                            BSP_PWM_SetDuty()
          │                                    │
          │                                    ▼
          │                            TIM3_CH1 ──► PA6 ──► 执行器
          ▼
  ┌──────────────────┐
  │  MonitorTask     │◄──────────── 心跳 / 功能状态上报
  │  (健康度判定)     │
  └────────┬─────────┘
           │ IsAllHealthy()?
           ▼
  ┌──────────────────┐        健康 ──► BSP_IWDG_Refresh()  喂狗
  │  watchdog_task   │
  │    (200 ms)      │        异常 ──► 停止喂狗 ──► IWDG 超时复位 MCU
  └──────────────────┘
```

### 任务间通信（IPC）

在 `app_ipc.c` 中统一创建，作为任务解耦的枢纽：

| 对象 | 类型 | 长度 | 用途 |
|---|---|---|---|
| `sensor_queue` | 队列 | 10 × `SensorData_t` | 采集任务 → 控制任务的采样值传递 |
| `log_queue` | 队列 | 10 × `LogMessage_t` | 任意任务 → 日志任务的异步日志缓冲 |
| `system_event_group` | 事件组 | — | 系统就绪状态同步 |

事件组标志位（`app_ipc.h`）：

| 标志 | 值 | 含义 |
|---|---|---|
| `EVENT_ADC_READY` | `1 << 0` | ADC 采集通道就绪 |
| `EVENT_PWM_READY` | `1 << 1` | PWM 输出通道就绪 |
| `EVENT_SYSTEM_READY` | `1 << 2` | 系统初始化完成 |
| `EVENT_FAULT` | `1 << 3` | 系统故障 |

`control_task` 在进入主循环前会阻塞等待 `EVENT_ADC_READY | EVENT_PWM_READY | EVENT_SYSTEM_READY` **三者同时置位**（`xEventGroupWaitBits` 的 `xWaitForAllBits = pdTRUE`），确保外设就绪后才开始输出，避免上电瞬间输出异常。

### 健康监控机制

`monitor_task.c` 为每个受监控任务维护一份 `TaskMonitorInfo_t`，从三个维度判定（`MonitorTask_IsAllHealthy()`，任一不满足即判定系统异常）：

1. **Liveness 存活检查** —— 距上次心跳是否超过 `heartbeat_timeout`
2. **Functionality 功能检查** —— 最近一次上报的功能状态 `function_ok` 是否有效
3. **Functionality 时效检查** —— 功能状态是否长期未刷新，超过 `function_timeout`

| 受监控任务 | 心跳超时 | 功能超时 |
|---|---|---|
| `collect_task` | 500 ms | 500 ms |
| `control_task` | 500 ms | 500 ms |
| `log_task` | 1000 ms | 1000 ms |

这一设计的价值在于：**任务「还在跑」不等于「干得对」**。只检测心跳会漏掉任务陷入死循环但仍在喂心跳的情况，因此额外引入功能状态维度。一旦判定异常，`watchdog_task` 停止喂狗，IWDG 在约 2 秒后强制复位 MCU，使系统从故障中自动恢复。

### 软件定时器

`app_timer.c` 创建两个周期定时器（由 FreeRTOS Timer Task 服务）：

| 定时器 | 周期 | 回调动作 |
|---|---|---|
| `HeartbeatTimer` | 500 ms | 翻转 PC10 LED，指示系统运行 |
| `LogTimer` | 2000 ms | 输出 `system alive` 存活日志 |

## 目录结构

```
Smart Control Terminal/
├── Core/
│   ├── Inc/                    # CubeMX 生成的头文件
│   │   └── FreeRTOSConfig.h    # FreeRTOS 内核配置（堆大小、优先级数、tick 频率等）
│   └── Src/                    # CubeMX 生成的外设初始化与外设中断服务
│       ├── main.c              # 主流程：HAL 初始化 → 读取复位原因 → BSP_Init → 启动调度器
│       ├── adc.c / dma.c       # ADC1 与 DMA2_Stream0 初始化
│       ├── tim.c               # TIM3（PWM）与 TIM2（HAL 时基）初始化
│       ├── usart.c / gpio.c    # USART1 与 GPIO 初始化
│       ├── iwdg.c              # 独立看门狗初始化
│       └── stm32f4xx_it.c      # 中断向量服务函数
├── Drivers/                    # STM32F4 HAL 库（CubeMX 生成）
├── FreeRTOS/
│   ├── source/                 # FreeRTOS 内核源码
│   ├── include/                # 内核头文件
│   └── portable/RVDS/ARM_CM3/  # Cortex-M3/M4 移植层（Keil RVDS 工具链）
├── User/                       # ★ 应用层代码（本项目核心）
│   ├── APP/                    # 应用逻辑
│   │   ├── app_data.h          # 数据结构定义（SensorData_t / LogMessage_t）
│   │   ├── app_ipc.c/.h        # 队列与事件组的统一创建与声明
│   │   ├── app_timer.c/.h      # 软件定时器（心跳灯 / 存活日志）
│   │   ├── app_freertos.c/.h   # 任务统一创建入口
│   │   ├── collect_task.c/.h   # 采集任务
│   │   ├── control_task.c/.h   # 控制任务
│   │   ├── log_task.c/.h       # 日志任务 + App_Log() 接口
│   │   ├── monitor_task.c/.h   # 任务健康监控
│   │   └── watchdog_task.c/.h  # 看门狗喂狗决策
│   └── BSP/                    # 板级支持包（硬件抽象）
│       ├── bsp.c/.h            # BSP 统一初始化入口
│       ├── bsp_adc.c/.h        # ADC 采样封装
│       ├── bsp_pwm.c/.h        # PWM 占空比设置封装
│       ├── bsp_usart.c/.h      # 串口收发 + printf 重定向（fputc）
│       └── bsp_iwdg.c/.h       # 看门狗喂狗与复位标志读取
├── MDK-ARM/                    # Keil 工程与启动文件
│   ├── Smart Control Terminal.uvprojx   # ★ Keil 工程文件
│   └── startup_stm32f407xx.s            # 启动汇编文件
└── Smart Control Terminal.ioc  # STM32CubeMX 工程配置
```

**分层设计**：`APP/` 只调用 `BSP/` 提供的接口，不直接触碰 HAL 与外设句柄；`BSP/` 负责封装 HAL 调用。更换底层硬件或外设时只需重写 `BSP/`，应用逻辑不受影响。

## 编译与烧录

### 环境要求

- **Keil MDK-ARM V5.32** 及以上（工程使用 Arm Compiler 5）
- STM32F4xx 器件支持包
- ST-Link / J-Link 调试器

### 编译步骤

1. 用 Keil MDK 打开 `MDK-ARM/Smart Control Terminal.uvprojx`
2. 确认目标器件为 `STM32F407VETx`
3. 全量编译（**Rebuild**）后下载至目标板

> 编译产物（`.o` / `.crf` / `.axf` / `.map` 等）已通过 `.gitignore` 排除，克隆后首次需全量编译。

### 从 CubeMX 重新生成

若需修改外设配置，用 **STM32CubeMX 6.9.2** 打开 `Smart Control Terminal.ioc` 修改后重新生成代码。注意 CubeMX 会重写 `Core/` 下的外设初始化代码，用户代码须写在 `/* USER CODE BEGIN */` 与 `/* USER CODE END */` 之间才不会被覆盖；`User/` 目录不受 CubeMX 影响。

### 运行观察

将 USART1（PA9）接 USB-TTL 连接 PC，波特率 **115200** 打开串口终端，可见如下输出：

```
CollectTask ADC = 2048
ControlTask PWM = 50%
system alive
```

若上次复位由看门狗触发，上电时会额外打印：

```
Previous reset reason: IWDG
```

## 设计要点

- **DMA + 连续转换**：ADC 配置为连续转换模式并由 DMA 循环搬运，`BSP_ADC_GetValue()` 直接读取 DMA 目标缓冲区，业务任务无需等待转换完成
- **printf 重定向**：`bsp_usart.c` 中实现 `fputc()` 将标准库输出重定向至 USART1；日志统一经队列由 `log_task` 输出，避免多任务并发调用 `printf` 造成串口数据交错
- **SysTick 归属**：FreeRTOS 占用 SysTick 产生系统节拍，HAL 时基改用 TIM2，二者互不干扰
- **看门狗作为最后防线**：健康监控负责发现异常，IWDG 负责在软件已无法自救时强制硬件复位

## 已知问题与踩坑记录

本节记录移植 FreeRTOS 与调试过程中实际遇到并解决的问题，以及当前仍存在的限制，供后续维护与同类项目参考。

### 已修复

#### 1. 系统节拍频率偏差 2.33 倍

**现象**：所有延时与超时判断的实际时长都短于标称值，但功能表现「看似正常」，极难察觉。

**原因**：`FreeRTOSConfig.h` 中的 `configCPU_CLOCK_HZ` 配置为 `72000000`（该值与其他 STM32F1 系列工程的典型主频相符，应为移植模板时未同步修改）。FreeRTOS 的 `port.c` 在未定义 `configSYSTICK_CLOCK_HZ` 时会直接取用 `configCPU_CLOCK_HZ`，而 Cortex-M 的 SysTick 以核心时钟计数。本工程 SYSCLK 为 168MHz，重装载值却按 72MHz 计算，导致实际节拍为 2333Hz 而非 1000Hz。

**修复**：将 `configCPU_CLOCK_HZ` 修正为 `168000000`，与实际 SYSCLK 保持一致。

> 移植 FreeRTOS 配置模板时此项极易被忽略 —— 数值错误既不会导致编译失败，也不会让程序立刻崩溃，只会让所有时间参数静默失准。

#### 2. control_task 优先级越界后被静默截断

**现象**：`control_task` 期望以最高优先级抢占运行，实际却与 `collect_task` 平级。

**原因**：`configMAX_PRIORITIES` 为 5（合法优先级为 0~4），而 `control_task` 被赋予优先级 5。FreeRTOS 的 `tasks.c` 会将越界优先级截断为 `configMAX_PRIORITIES - 1`；由于工程未定义 `configASSERT`，该越界不会触发断言告警，属于静默发生。

**修复**：将 `configMAX_PRIORITIES` 提升至 6，保留原有的优先级编排意图。

#### 3. start_task 的临界区永不退出

**现象**：`taskEXIT_CRITICAL()` 被写在 `vTaskDelete(NULL)` 之后。

**原因**：`vTaskDelete(NULL)` 删除任务自身后不会返回，其后的语句成为死代码。在 Cortex-M 上 `taskENTER_CRITICAL()` 通过设置 `BASEPRI` 屏蔽中断，中断嵌套计数 `uxCriticalNesting` 因此无法归零，中断将持续处于屏蔽状态。

**修复**：调整语句顺序，先退出临界区，再删除任务自身。

### 已知限制

- **采集与控制任务的功能检查恒为通过**：`collect_task` 以 `adc_value > 4095U` 判定业务异常，但 ADC 为 12 位、量程上限即 4095，该条件永不成立；`control_task` 的 `duty > 100U` 同理。即功能监控的「业务正确性」维度目前形同虚设，实际生效的是心跳超时检查。
- **看门狗任务自身不在监控范围内**：`TaskMonitorId_t` 仅涵盖 collect / control / log 三个任务，`watchdog_task` 未纳入自检。
- **控制策略为线性映射**：占空比由采样值线性换算（`duty = adc × 100 / 4095`），未引入 PID 等闭环调节。
- **源码中文注释为 GBK 编码**：Keil 与 GitHub 代码页均可正常显示（GitHub 会自动探测编码），但用 VS Code 等默认以 UTF-8 解析的工具打开会显示乱码，不影响编译。

## 许可

本项目为学习与实践性质的开源工程。`Drivers/` 目录下的 STM32 HAL 库版权归 STMicroelectronics 所有（遵循其附带许可），`FreeRTOS/` 目录版权归 Amazon.com, Inc. 所有（MIT 许可）。
