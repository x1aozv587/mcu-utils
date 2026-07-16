# bles — BLE 广播数据构造与过滤

## 0 修改记录

| 日期 | 版本 | 内容 |
|------|------|------|
| 2026-07-10 | v1.4 | 新增 manufacturer 过滤预设；新增 UUID128 过滤支持 |
| 2026-07-10 | v1.3 | 新增 UUID 过滤预设、name 过滤预设 |
| 2026-07-09 | v1.2 | 新增 `bles_filter` 核心引擎，回调模式遍历 AD Structure |
| 2026-07-07 | v1.1 | 新增 iBeacon 预设；新增便捷接口（append_flags/name/uuid16/tx/manufacturer）；单元测试 |
| 2026-07-06 | v1.0 | 初始版本，AD Structure 通用组装器，支持扩展广播容量 |

## 1 简介

`bles` 是 BLE 广播数据的构造和过滤工具集，不涉及协议栈，仅处理数据层面。

两个子模块：

- **AD Builder** (`bles_adv`) — 构造广播包，核心引擎 + 预设模式
- **Filter** (`bles_filter`) — 过滤广播包，核心引擎 + 回调模式 + 三个预设

特点：
- 零动态内存，buffer 全部外部传入
- AD Builder 支持 `uint16_t` 容量，为扩展广播预留
- Filter 支持 NULL 回调（仅判断 AD Type 是否存在）
- 所有预设共享同一个核心引擎

## 2 文件清单

```
ble/
  bles_adv.h / .c                       -- AD Builder 核心引擎
  bles_adv_ibeacon.h / .c               -- iBeacon 预设

  bles_filter.h / .c                    -- Filter 核心引擎
  bles_filter_name.h / .c               -- 名称过滤预设
  bles_filter_uuid.h / .c               -- UUID 过滤预设
  bles_filter_manufacturer.h / .c       -- 厂商数据过滤预设

  test_bles_adv.c                       -- 测试
  README.md                             -- 本文件
```

## 3 数据结构

### 3.1 AD Builder

```c
typedef struct {
    uint16_t  capacity;   // 缓冲区容量
    uint16_t  pos;        // 当前写入位置
    uint8_t  *p_buf;      // 缓冲区指针
} bles_adv_builder_t;
```

### 3.2 AD Type 枚举

```c
BLES_AD_TYPE_FLAGS            = 0x01  // Flags
BLES_AD_TYPE_UUID16_COMPLETE  = 0x03  // 16-bit UUID 完整列表
BLES_AD_TYPE_UUID16_INCOMPLETE= 0x02  // 16-bit UUID 不完整列表
BLES_AD_TYPE_UUID128_COMPLETE = 0x07  // 128-bit UUID 完整列表
BLES_AD_TYPE_SHORT_NAME       = 0x08  // 短名称
BLES_AD_TYPE_COMPLETE_NAME    = 0x09  // 完整名称
BLES_AD_TYPE_TX_POWER         = 0x0A  // 发射功率
BLES_AD_TYPE_MANUFACTURER     = 0xFF  // 厂商自定义
```

## 4 API 参考

### 4.1 AD Builder 核心

| 函数 | 说明 |
|------|------|
| `bles_adv_builder_init(p_builder, p_buf, buf_size)` | 初始化组装器，返回 `mu_status_t` |
| `bles_adv_append(p_builder, type, p_data, data_len)` | 追加一条 AD Structure（核心写入入口） |
| `bles_adv_get_len(p_builder)` | 获取当前已组装数据长度 |

### 4.2 AD Builder 便捷接口

| 函数 | 说明 |
|------|------|
| `bles_adv_append_flags(p_builder, flags)` | 追加 Flags |
| `bles_adv_append_name(p_builder, p_name, complete)` | 追加设备名称 |
| `bles_adv_append_uuid16(p_builder, uuid, complete)` | 追加单个 16-bit UUID |
| `bles_adv_append_tx_power(p_builder, tx_power)` | 追加发射功率 |
| `bles_adv_append_manufacturer(p_builder, p_data, data_len)` | 追加厂商数据 |

### 4.3 iBeacon 预设

| 函数 | 说明 |
|------|------|
| `bles_adv_ibeacon_build(p_builder, p_params)` | 完整组装 iBeacon 广播包（30 字节） |

参数结构体：UUID[16]、major、minor、tx_power。

### 4.4 Filter 核心

| 函数 | 说明 |
|------|------|
| `bles_filter_find(p_adv, adv_len, type, callback, p_arg)` | 遍历 AD Structure，匹配 type 时调用 callback。callback=NULL 时仅判断是否存在 |

回调类型：
```c
typedef bool (*bles_filter_ad_callback_t)(const uint8_t *p_data, uint8_t len, void *p_arg);
```

### 4.5 Filter 预设

| 函数 | 说明 |
|------|------|
| `bles_filter_name_match(p_adv, adv_len, p_name, mode)` | 名称过滤（完全/前缀/包含） |
| `bles_filter_uuid_match(p_adv, adv_len, p_uuid, uuid_len)` | UUID 过滤（2=16bit, 16=128bit） |
| `bles_filter_manufacturer_match(p_adv, adv_len, p_data, data_len)` | 厂商数据包含匹配 |

## 5 使用例程

### 5.1 构造通用外设广播

```c
#include "bles_adv.h"

uint8_t buf[31];
bles_adv_builder_t b;
bles_adv_builder_init( &b, buf, sizeof(buf) );

bles_adv_append_flags( &b, BLES_ADV_FLAG_LE_GENERAL | BLES_ADV_FLAG_BR_EDR_NOT_SUP );
bles_adv_append_name( &b, "MyDevice", true );
bles_adv_append_uuid16( &b, 0xFFE0, true );

uint16_t len = bles_adv_get_len( &b );
// 将 buf 写入 BLE 广播
```

### 5.2 构造 iBeacon

```c
#include "bles_adv_ibeacon.h"

bles_adv_ibeacon_params_t p = {
    .uuid     = { 0xE2,0xC5,...,0xE0 },
    .major    = 1,
    .minor    = 2,
    .tx_power = -59,
};
bles_adv_builder_t b;
uint8_t buf[31];

bles_adv_builder_init( &b, buf, sizeof(buf) );
bles_adv_ibeacon_build( &b, &p );
```

### 5.3 按名称过滤

```c
#include "bles_filter_name.h"

void on_adv_report( const uint8_t *p_data, uint8_t len )
{
    if( bles_filter_name_match( p_data, len, "MySensor", BLES_FILTER_NAME_PREFIX ) )
    {
        // 匹配到 MySensor* 设备
    }
}
```

### 5.4 判断是否存在 UUID

```c
// 仅判断，不需要回调
bool has_uuid = bles_filter_find( adv, len, BLES_AD_TYPE_UUID16_COMPLETE, NULL, NULL );
```

### 5.5 按厂商数据过滤

```c
// 查找 Manufacturer Data 中是否包含 55AA
uint8_t key[] = { 0x55, 0xAA };
if( bles_filter_manufacturer_match( adv, len, key, 2 ) )
{
    // 匹配
}

// 在厂商数据中查找字符串
bles_filter_manufacturer_match( adv, len, (const uint8_t*)"MyDev", 5 );
```

## 6 设计说明

### 6.1 AD Builder 分层

```
bles_adv               -- 通用组装器（append 是唯一写入入口）
bles_adv_ibeacon       -- iBeacon 预设（内部调用 append 两次）
bles_adv_common        -- 通用外设预设（计划中）
```

### 6.2 Filter 分层

```
bles_filter                  -- 通用遍历引擎（find）
bles_filter_name             -- 名称预设（内部调 find ×2）
bles_filter_uuid             -- UUID 预设（内部调 find ×2）
bles_filter_manufacturer     -- 厂商数据预设（内部调 find ×1）
```

预设内部：定义 ctx 结构体 + 回调函数 + 包装函数。回调透过 `void *p_arg` 接收参数。

## 7 注意事项

1. `bles_adv_append` 中 data_len 最大 254（AD Structure length 字段仅 1 字节）
2. `bles_filter_find` 的 callback 返回 true 时终止遍历；callback=NULL 时找到即返回 true
3. `bles_adv_ibeacon_build` 要求空 builder（不允许追加到已有数据），失败时回滚 pos
4. 厂商过滤是字节级别的包含搜索，不关心语义，可搜索任意字节序列或 ASCII 字符串
5. 名称过滤需区分完整/短名称，内部自动查两种 type
6. UUID 过滤需指定长度 2 或 16，内部自动查 COMPLETE 和 INCOMPLETE
