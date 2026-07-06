# mu_datetime — 日期时间计算模块

## 0 修改记录

| 日期 | 版本 | 内容 |
|------|------|------|
| 2026-07-06 | v1.1 | 静态函数加 `mu_datetime_` 前缀、增加注释； 规范整理 |
| 2026-07-03 | v1.0 | 初始版本，校验/查询/时间戳互转/运算/比较 |

## 1 简介

`mu_datetime` 是面向 MCU 的轻量级日期时间计算库，不依赖 RTOS 和动态内存。

特点：
- 时间戳 epoch 可配置（1970 Unix 纪元 / 2000 纪元），编译期宏切换
- 内部以 2000-01-01 为基准计算，1970 epoch 仅加减常量偏移
- 支持 2000 年以前的日期（1970 epoch 下 1970~1999 全部正确）
- `mu_datetime_to_timestamp_ex` 安全版避免 0 值歧义
- 所有有返回值函数均通过 `bool` 明确区分成功/失败

## 2 文件清单

| 文件 | 说明 |
|------|------|
| `mu_datetime.h` | 接口声明、结构体定义、epoch 宏 |
| `mu_datetime.c` | 实现 |
| `test_mu_datetime.c` | 测试（支持 1970/2000 epoch 两套编译） |
| `README.md` | 本文件 |

## 3 数据结构

### 3.1 日期

```c
typedef struct {
    uint16_t year;    // 年，1970~2099 或 2000~2099
    uint8_t  month;   // 月，1~12
    uint8_t  day;     // 日，1~31
} mu_date_t;
```

### 3.2 时间

```c
typedef struct {
    uint8_t hour;     // 0~23
    uint8_t minute;   // 0~59
    uint8_t second;   // 0~59
} mu_time_t;
```

### 3.3 日期时间

```c
typedef struct {
    mu_date_t date;
    mu_time_t time;
} mu_datetime_t;
```

### 3.4 星期

```c
typedef enum {
    MU_WEEKDAY_SUNDAY    = 0,
    MU_WEEKDAY_MONDAY    = 1,
    MU_WEEKDAY_TUESDAY   = 2,
    MU_WEEKDAY_WEDNESDAY = 3,
    MU_WEEKDAY_THURSDAY  = 4,
    MU_WEEKDAY_FRIDAY    = 5,
    MU_WEEKDAY_SATURDAY  = 6,
} mu_weekday_t;
```

## 4 API 参考

### 4.1 校验

| 函数 | 说明 |
|------|------|
| `mu_date_is_valid(p_date)` | 校验年月日合法性（含 epoch 年份范围） |
| `mu_time_is_valid(p_time)` | 校验时分秒范围 |
| `mu_datetime_is_valid(p_dt)` | 日期+时间组合校验 |

### 4.2 查询

| 函数 | 说明 |
|------|------|
| `mu_date_is_leap_year(year)` | 闰年判断 |
| `mu_date_days_in_month(year, month)` | 获取某月天数 |
| `mu_date_weekday(p_date)` | 计算星期几 |
| `mu_date_day_of_year(p_date)` | 年内第几天（1~366） |

### 4.3 时间戳互转

| 函数 | 说明 |
|------|------|
| `mu_datetime_to_timestamp(p_dt)` | 转时间戳，非法返回 0（有歧义） |
| `mu_datetime_to_timestamp_ex(p_dt, p_ts)` | 安全版，bool 返回 + 输出参数 |
| `mu_timestamp_to_datetime(ts, p_dt)` | 时间戳转日期时间，返回 bool |

### 4.4 运算

| 函数 | 说明 |
|------|------|
| `mu_date_add_days(p_date, days)` | 日期加/减天数（原地修改），返回 bool 表示是否越界 |
| `mu_date_diff_days(p_date1, p_date2)` | 两个日期间隔天数（date1-date2，可为负） |
| `mu_time_add_seconds(p_time, seconds)` | 时间加/减秒数（原地修改），不允许跨天 |

### 4.5 比较

| 函数 | 说明 |
|------|------|
| `mu_date_compare(p_date1, p_date2)` | 日期比较，返回 -1/0/1 |
| `mu_time_compare(p_time1, p_time2)` | 时间比较，返回 -1/0/1 |

## 5 配置宏

| 宏 | 取值 | 说明 |
|------|------|------|
| `MU_DATETIME_EPOCH` | `MU_DATETIME_EPOCH_1970 (0)` / `MU_DATETIME_EPOCH_2000 (1)` | 时间戳纪元 |

```c
// 默认 1970
#define MU_DATETIME_EPOCH  MU_DATETIME_EPOCH_1970

// 或切到 2000（编译参数）
// gcc -DMU_DATETIME_EPOCH=MU_DATETIME_EPOCH_2000
```

两种 epoch 下年份范围：

| epoch | 年份范围 | 1970-01-01 时间戳 | 2000-01-01 时间戳 |
|------|------|------|------|
| 1970 | 1970~2099 | 0 | 946684800 |
| 2000 | 2000~2099 | 非法 | 0 |

## 6 使用例程

### 6.1 基本使用

```c
#include "mu_datetime.h"

mu_date_t d = { 2026, 7, 3 };
mu_weekday_t wd = mu_date_weekday( &d );  // MU_WEEKDAY_FRIDAY

bool leap = mu_date_is_leap_year( 2024 );  // true
uint8_t dim = mu_date_days_in_month( 2024, 2 );  // 29
uint16_t doy = mu_date_day_of_year( &d );  // 184
```

### 6.2 时间戳互转

```c
// datetime → timestamp（安全版）
mu_datetime_t dt = { { 2026, 7, 3 }, { 14, 30, 0 } };
uint32_t ts;
if( mu_datetime_to_timestamp_ex( &dt, &ts ) )
{
    printf( "timestamp = %lu\n", ts );
}

// timestamp → datetime
mu_datetime_t out;
if( mu_timestamp_to_datetime( ts, &out ) )
{
    // out.date.year == 2026, out.time.hour == 14, ...
}
```

### 6.3 日期运算

```c
mu_date_t d = { 2026, 1, 31 };

mu_date_add_days( &d, 1 );
// d = { 2026, 2, 1 }

mu_date_add_days( &d, -1 );
// d = { 2026, 1, 31 }

mu_date_t a = { 2026, 1, 10 };
mu_date_t b = { 2026, 1, 1 };
int32_t diff = mu_date_diff_days( &a, &b );  // 9
```

### 6.4 时间运算

```c
mu_time_t t = { 23, 59, 58 };

bool ok = mu_time_add_seconds( &t, 1 );
// ok=true, t={23,59,59}

ok = mu_time_add_seconds( &t, 1 );
// ok=false，跨天不允许

ok = mu_time_add_seconds( &t, -1 );
// ok=true, t={23,59,58}
```

### 6.5 1970 epoch 下处理 1970~1999 的日期

```c
// 默认 MU_DATETIME_EPOCH_1970

mu_datetime_t dt = { { 1999, 12, 31 }, { 23, 59, 59 } };
uint32_t ts;
mu_datetime_to_timestamp_ex( &dt, &ts );
// ts == 946684799

mu_timestamp_to_datetime( 0, &dt );
// dt = { 1970, 1, 1, 0, 0, 0 }

mu_date_t d = { 1999, 12, 31 };
mu_date_add_days( &d, 1 );
// d = { 2000, 1, 1 }
```

## 7 设计说明

### 7.1 内部基准

所有计算以 2000-01-01 为内部基准天数 0。1970 epoch 仅在对外接口加/减常量 `MU_SECONDS_1970_TO_2000 = 946684800`。`days_from_2000` 和 `date_from_days` 都支持前 2000 年的负值。

### 7.2 C99 除法

时间戳反转换时，前 2000 年的时间戳需 floor 除法。代码已处理 `ts < 0` 时的向零取整问题。

### 7.3 范围边界

`days_min` / `days_max` 根据 epoch 动态计算，而非硬编码常数。只要 epoch 切到 2000，范围自动收窄。

## 8 注意事项

1. `mu_datetime_to_timestamp` (非 ex 版) 非法输入返回 0，但 1970-01-01 的合法时间戳也是 0，无法区分。**推荐始终用 `_ex` 版本**
2. `mu_time_add_seconds` 不允许跨天（返回 false），如需跨天请升级到 `mu_datetime_add_seconds`（未来提供）
3. `mu_date_compare` / `mu_time_compare` 不校验合法性，需调用方保证参数有效
4. 年份上限硬编码为 2099，2100-01-01 之后视为非法
