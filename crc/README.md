# mu_crc — 通用 CRC 计算模块

## 0 修改记录

| 日期 | 版本 | 内容 |
|------|------|------|
| 2026-07-06 | v1.5 | 静态函数增加注释；规范整理 |
| 2026-07-05 | v1.4 | 新增 CRC-32 算法 |
| 2026-07-02 | v1.3 | 新增 CRC-16 CCITT (KERMIT) / CCITT-FALSE |
| 2026-07-02 | v1.2 | 新增 CRC-8 算法 |
| 2026-07-01 | v1.1 | 新增 `mu_crc_calc_tbl` 查表引擎、`mu_crc_feed` / `mu_crc_continue` 系列 |
| 2026-07-01 | v1.0 | 初始版本，CRC-16 MODBUS / CRC-16 XMODEM + 逐位计算引擎 |

## 1 简介

`mu_crc` 是面向 MCU 的通用 CRC 计算库。核心是一个参数驱动的计算引擎，各算法通过预设参数 + 便捷 wrapper 实现，共享同一引擎。

特点：
- 参数驱动：一个引擎覆盖所有 CRC 算法，新增算法只需一条预设参数
- 查表/逐位双模式：编译期宏切换，查表快、逐位省 ROM
- 三类接口：一次性 `calc`、裸 `feed`（手动控制寄存器）、`continue`（追加续算）
- 文件独立：每个算法 `.h`/`.c` 独立，可按需编译链接

## 2 文件清单

```
crc/
  mu_crc.h                    # 核心引擎头文件（参数结构体 + 通用接口 + 工具函数）
  mu_crc.c                    # 核心引擎实现（逐位 feed / 查表 feed / continue）

  mu_crc8.h / .c              # CRC-8 (poly=0x07)
  mu_crc16_modbus.h / .c      # CRC-16 MODBUS (poly=0x8005, 反射)
  mu_crc16_xmodem.h / .c      # CRC-16 XMODEM (poly=0x1021)
  mu_crc16_ccitt.h / .c       # CRC-16 CCITT/KERMIT (poly=0x1021, 反射)
  mu_crc16_ccitt_false.h / .c # CRC-16 CCITT-FALSE (poly=0x1021, init=0xFFFF)
  mu_crc32.h / .c             # CRC-32 (poly=0x04C11DB7, 反射)

  test_mu_crc.c               # 测试
  README.md                   # 本文件
```

## 3 预设算法一览

| 算法 | 封装函数 | width | poly | init | ref_in/out | xor_out | check |
|------|------|------|------|------|------|------|------|
| CRC-8 | `mu_crc8_calc()` | 8 | 0x07 | 0x00 | F/F | 0x00 | 0xF4 |
| CRC-16 MODBUS | `mu_crc16_modbus()` | 16 | 0x8005 | 0xFFFF | T/T | 0x0000 | 0x4B37 |
| CRC-16 XMODEM | `mu_crc16_xmodem()` | 16 | 0x1021 | 0x0000 | F/F | 0x0000 | 0x31C3 |
| CRC-16 CCITT | `mu_crc16_ccitt()` | 16 | 0x1021 | 0x0000 | T/T | 0x0000 | 0x2189 |
| CRC-16 CCITT-FALSE | `mu_crc16_ccitt_false()` | 16 | 0x1021 | 0xFFFF | F/F | 0x0000 | 0x29B1 |
| CRC-32 | `mu_crc32_calc()` | 32 | 0x04C11DB7 | 0xFFFFFFFF | T/T | 0xFFFFFFFF | 0xCBF43926 |

> check 值为对标准测试串 `"123456789"` 的 CRC 计算结果。

## 4 核心 API

### 4.1 数据结构

```c
typedef struct {
    uint8_t  width;      // CRC 位宽 8~32
    uint32_t poly;       // 生成多项式（不含隐式最高位）
    uint32_t init;       // 寄存器初始值
    uint32_t xor_out;    // 输出异或值
    bool     ref_in;     // 输入字节是否按位反转（LSB-first 设为 true）
    bool     ref_out;    // 输出是否需要按位反转
} mu_crc_params_t;
```

### 4.2 一次性计算

| 函数 | 说明 |
|------|------|
| `mu_crc_calc(p_param, p_data, len)` | 逐位计算，不依赖查表 |
| `mu_crc_calc_tbl(p_param, p_data, len, p_table)` | 查表计算，需外部传入 256 项 `uint32_t` 表 |

### 4.3 裸 feed

| 函数 | 说明 |
|------|------|
| `mu_crc_feed(p_param, crc, p_data, len)` | 逐位 feed：给定寄存器值，处理数据后返回新寄存器值，不做 init/finalize |
| `mu_crc_feed_tbl(p_param, crc, p_data, len, p_table)` | 查表 feed |

### 4.4 续算

| 函数 | 说明 |
|------|------|
| `mu_crc_continue(p_param, prev_crc, p_data, len)` | 接收上次 `mu_crc_calc` 的 finalized 值，自动反解→feed→重新 finalize |
| `mu_crc_continue_tbl(p_param, prev_crc, p_data, len, p_table)` | 续算查表版 |

### 4.5 工具函数

| 函数 | 说明 |
|------|------|
| `mu_crc_reflect(data, width)` | 按位反转，手动 finalize 时可能需要 |

### 4.6 算法便捷函数

每个算法提供：

| 函数 | 说明 |
|------|------|
| `mu_crcXX_calc(p_data, len)` | 一次性计算（内部调用 `mu_crc_calc_tbl` 或 `mu_crc_calc`） |
| `mu_crcXX_continue(prev, p_data, len)` | 续算（内部调用 `mu_crc_continue_tbl` 或 `mu_crc_continue`） |
| `mu_get_crcXX_model()` | 获取算法模型指针 |

## 5 配置宏

| 宏 | 取值 | 说明 |
|------|------|------|
| `MU_CRC_DEFAULT_MODE` | `MU_CRC_MODE_TABLE (0)` / `MU_CRC_MODE_BITWISE (1)` | 全局默认计算模式 |
| `MU_CRC8_MODE` | 同上 | CRC-8 单独切换 |
| `MU_CRC16_MODBUS_MODE` | 同上 | MODBUS 单独切换 |
| `MU_CRC16_XMODEM_MODE` | 同上 | XMODEM 单独切换 |
| `MU_CRC16_CCITT_MODE` | 同上 | CCITT 单独切换 |
| `MU_CRC16_CCITT_FALSE_MODE` | 同上 | CCITT-FALSE 单独切换 |
| `MU_CRC32_MODE` | 同上 | CRC-32 单独切换 |

优先级：算法级 > 全局级。未定义算法级宏时 fallback 到 `MU_CRC_DEFAULT_MODE`。

示例——全局走逐位，仅 MODBUS 用查表：

```c
#define MU_CRC_DEFAULT_MODE     MU_CRC_MODE_BITWISE
#define MU_CRC16_MODBUS_MODE    MU_CRC_MODE_TABLE
#include "mu_crc.h"
```

## 6 使用例程

### 6.1 一次性计算（wrapper——最常用）

```c
#include "mu_crc16_modbus.h"

uint8_t frame[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01 };
uint16_t crc = mu_crc16_modbus( frame, sizeof( frame ) );
// crc == 0x0A84
```

### 6.2 自定义参数（通用引擎）

```c
#include "mu_crc.h"

// CRC-16 CCITT: poly=0x1021, init=0x0000, 非反射
mu_crc_params_t param = { 16, 0x1021, 0x0000, 0x0000, false, false };
uint32_t crc = mu_crc_calc( &param, data, len );
```

### 6.3 分片收包（手动 feed）

```c
mu_crc_params_t param = { 16, 0x8005, 0xFFFF, 0x0000, true, true };
uint32_t mask = 0xFFFF;
uint8_t ch;
uint32_t crc = param.init & mask;       // init

while( uart_recv( &ch, 1 ) == 1 )
{
    crc = mu_crc_feed( &param, crc, &ch, 1 );  // feed 逐字节
}

if( param.ref_out ) crc = mu_crc_reflect( crc, param.width );  // finalize
crc ^= param.xor_out;
uint16_t result = ( uint16_t )( crc & mask );
```

### 6.4 已有结果追加续算

```c
uint16_t prev = mu_crc16_modbus( chunk1, len1 );           // 第一段结果
uint16_t crc  = mu_crc16_modbus_continue( prev, chunk2, len2 );  // 续算第二段
// crc == mu_crc16_modbus( chunk1+chunk2, len1+len2 )
```

## 7 注意事项

1. `mu_crc_params_t` 传入后只保存指针不复制，调用方需保证参数生命周期
2. 查表法占 ROM（每算法 256×4=1KB），逐位法不占但速度慢
3. `mu_crc_calc` (逐位) 和 `mu_crc_calc_tbl` (查表) 的 finalize 规则不同，不要混用。分别使用对应的 `mu_crc_feed`/`mu_crc_feed_tbl` 和 `mu_crc_continue`/`mu_crc_continue_tbl`
4. CRC 结果 0 不代表错误（如 CRC-8 空数据结果为 0x00），校验输入参数用返回值或其他方式
