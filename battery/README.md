# mu_battery — 电池计算模块

## 0 修改记录

| 日期 | 版本 | 内容 |
|------|------|------|
| 2026-07-06 | v1.3 | 静态函数名加 `mu_battery_` 前缀、增加注释；规范整理 |
| 2026-07-06 | v1.2 | 调整函数编写顺序（静态→初始化→外部→Get） |
| 2026-07-03 | v1.1 | 新增 `is_charging` 标志独立于状态位；`cell_mv` 四舍五入；边界检验和曲线合法校验 |
| 2026-07-03 | v1.0 | 初始版本，ADC 转换/滤波/线性百分比/曲线插值/状态机 |

## 1 简介

`mu_battery` 是面向 MCU 的电池电量计算库，完成从 ADC 原始值到电量百分比、电池状态的完整链路。

特点：
- ADC 原始值 → 电压 → 滤波 → 百分比 → 状态，一条链路
- 支持线性映射和自定义分段曲线插值
- 多电芯支持（1S~4S），所有电压参数按单电芯配置
- 滑动窗口滤波，可配置窗口大小
- 充电状态由 `is_charging` 标志独立管理，不受电压波动影响
- 参数校验完善（曲线表顺序、百分比范围、各字段合法性）

## 2 文件清单

| 文件 | 说明 |
|------|------|
| `mu_battery.h` | 接口声明、结构体定义、状态枚举 |
| `mu_battery.c` | 实现 |
| `test_mu_battery.c` | 测试 |
| `README.md` | 本文件 |

## 3 数据结构

### 3.1 电池状态

```c
typedef enum {
    MU_BATTERY_STATE_UNKNOWN  = 0,  // 初始化后尚未获取有效电压
    MU_BATTERY_STATE_LOW,           // 低电量
    MU_BATTERY_STATE_NORMAL,        // 正常
    MU_BATTERY_STATE_CHARGING,      // 充电中
    MU_BATTERY_STATE_FULL,          // 已充满
} mu_battery_state_t;
```

### 3.2 曲线映射点

```c
typedef struct {
    uint16_t voltage_mv;  // 电压 mV（单电芯），必须从高到低排列
    uint8_t  percent;     // 对应百分比 0~100，建议从高到低
} mu_battery_curve_point_t;
```

### 3.3 电池参数

```c
typedef struct {
    uint8_t  cell_count;          // 电芯串联数 1~4
    uint16_t voltage_full_mv;     // 满电电压 mV（单电芯）
    uint16_t voltage_empty_mv;    // 亏电电压 mV（单电芯）
    uint16_t voltage_low_mv;      // 低电量告警阈值 mV（单电芯）
    uint16_t adc_ref_mv;          // ADC 参考电压 mV
    uint16_t adc_resolution;      // ADC 分辨率（4096=12bit）
    uint16_t adc_divider_ratio;   // 分压比 (R1+R2)/R2 ×100（如 4.73→473）
    uint8_t  filter_window;       // 滤波窗口 1~16
    uint8_t  reserve;             // 预留
    const mu_battery_curve_point_t *p_curve;  // 曲线表（NULL=线性）
    uint8_t  curve_point_count;   // 曲线表点数
} mu_battery_params_t;
```

### 3.4 运行上下文

```c
typedef struct {
    const mu_battery_params_t *p_params;  // 参数指针（需保证生命周期）
    uint16_t voltage_mv;                  // 滤波后电压 mV
    uint16_t voltage_raw_mv;              // 原始电压 mV
    uint8_t  percent;                     // 电量百分比 0~100
    mu_battery_state_t state;             // 当前状态
    bool     is_charging;                 // 充电标志（独立于 state）
    uint16_t filter_buf[16];              // 滤波缓冲
    uint8_t  filter_idx;                  // 写入索引
    uint8_t  filter_cnt;                  // 已累积采样数
} mu_battery_t;
```

## 4 API 参考

### 4.1 初始化

| 宏/函数 | 说明 |
|------|------|
| `MU_BATTERY_INIT(params_ptr)` | 静态初始化宏，仅赋初值不做校验 |
| `mu_battery_init(p_battery, p_params)` | 运行时初始化，含完整参数校验，返回 `mu_status_t` |

> `MU_BATTERY_INIT` 不调用参数校验。推荐使用 `mu_battery_init()`。

### 4.2 数据输入

| 函数 | 说明 |
|------|------|
| `mu_battery_feed_adc(p_battery, adc_raw)` | 喂入 ADC 原始值，自动转电压→滤波→更新百分比和状态 |
| `mu_battery_feed_voltage(p_battery, voltage_mv)` | 直接喂入电压值（整包总电压），跳过 ADC 转换 |

### 4.3 状态获取

| 函数 | 说明 |
|------|------|
| `mu_battery_get_voltage(p_battery)` | 获取滤波后电压（mV） |
| `mu_battery_get_percent(p_battery)` | 获取电量百分比（0~100） |
| `mu_battery_get_state(p_battery)` | 获取电池状态枚举 |

### 4.4 充电控制

| 函数 | 说明 |
|------|------|
| `mu_battery_set_charging(p_battery, true/false)` | 设置充电状态。`is_charging` 标志持续有效直到再次调用 |


## 5 使用例程

### 5.1 线性模式（1S 锂电池）

```c
#include "mu_battery.h"

static const mu_battery_params_t g_params =
{
    .cell_count        = 1,
    .voltage_full_mv   = 4200,
    .voltage_empty_mv  = 3300,
    .voltage_low_mv    = 3500,
    .adc_ref_mv        = 3300,
    .adc_resolution    = 4096,
    .adc_divider_ratio = 200,   // 分压比 2.0 ×100
    .filter_window     = 4,
};

mu_battery_t g_bat;

void app_init( void )
{
    mu_battery_init( &g_bat, &g_params );
}

void adc_isr( uint16_t adc_val )
{
    mu_battery_feed_adc( &g_bat, adc_val );
    uint8_t pct = mu_battery_get_percent( &g_bat );
    // 更新显示...
}
```

### 5.2 自定义曲线模式

```c
static const mu_battery_curve_point_t curve[] =
{
    { 4200, 100 },
    { 4000, 75  },
    { 3800, 50  },
    { 3600, 25  },
    { 3300, 0   },
};

static const mu_battery_params_t g_params =
{
    .cell_count        = 1,
    .voltage_full_mv   = 4200,
    .voltage_empty_mv  = 3300,
    .voltage_low_mv    = 3500,
    .adc_ref_mv        = 3300,
    .adc_resolution    = 4096,
    .adc_divider_ratio = 200,
    .filter_window     = 4,
    .p_curve           = curve,
    .curve_point_count = 5,
};
```

### 5.3 2S 电池

```c
static const mu_battery_params_t g_params =
{
    .cell_count        = 2,
    .voltage_full_mv   = 4200,
    .voltage_empty_mv  = 3300,
    // ...其他参数同 1S
};

// feed_voltage 传入整包总电压，内部自动除以 cell_count
mu_battery_feed_voltage( &g_bat, 7600 );  // 2S, 每节 3800mV → 50%
```

### 5.4 充电状态管理

```c
// 充电器插入
mu_battery_set_charging( &g_bat, true );   // state → CHARGING

// 继续喂电压，即使电压波动也不会退出 CHARGING
mu_battery_feed_adc( &g_bat, adc_val );

// 充满后自动转 FULL
// percent >= 100 → state → FULL

// 充电器拔出
mu_battery_set_charging( &g_bat, false );  // 重新判断 NORMAL/LOW/FULL
```

## 6 设计说明

### 6.1 参数生命周期

`mu_battery_init` 只保存 `p_params` 指针，不做深拷贝。调用方必须保证 `mu_battery_params_t` 在 `mu_battery_t` 存活期间一直有效。推荐定义为 `static const`。

### 6.2 电压映射

- 线性模式：`percent = (cell_mv - empty) × 100 / (full - empty)`
- 曲线模式：分段线性插值，曲线表须从高到低排列且通过 `is_valid_curve` 校验
- 两种模式均在 `voltage >= full` 时返回 100%，`voltage <= empty` 时返回 0%

### 6.3 is_charging 与 state 的关系

`is_charging` 是独立标志位：

- `set_charging(true)` → `is_charging = true`，`state = CHARGING`
- 充电中电压回落后 `state` 保持在 `CHARGING` 或 `FULL`，不会跳 `NORMAL`
- `set_charging(false)` → `is_charging = false`，`state` 根据电压重新判断

### 6.4 滤波

滑动窗口均值滤波，窗口大小 1~16。窗口为 1 时不滤波（瞬时值）。

## 7 注意事项

1. 曲线表必须按 `voltage_mv` 从高到低排列，`percent` 不递增（允许相等），所有值在 0~100 内
2. 多电芯时 `feed_voltage` / `feed_adc` 传入整包总电压，内部自动除以 `cell_count` 转为单电芯电压
3. ADC 转换使用 `uint64_t` 中间变量防溢出，电压超过 65535mV 时饱和到 65535
4. `MU_BATTERY_INIT` 不做校验，非法参数可能在后续 `feed` 时行为异常
