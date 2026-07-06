# mu_ringbuf — 通用环形缓冲区

## 0 修改记录

| 日期 | 版本 | 内容 |
|------|------|------|
| 2026-07-06 | v1.2 | 静态函数加 `mu_ringbuf_` 前缀、增加注释；调整函数编写顺序符合规范 |
| 2026-07-06 | v1.1 | 优化函数编写顺序（静态→初始化→外部→Get） |
| 2026-06-29 | v1.0 | 初始版本，包含读写/peek/drop/互斥锁完整功能 |

## 1 简介

`mu_ringbuf` 是一个面向 MCU 的通用环形缓冲区（Ring Buffer）库，支持多生产者/消费者场景下的线程安全操作。

特点：
- 零动态内存分配，buffer 由外部传入
- 可选的互斥锁回调，适配 FreeRTOS / RT-Thread / 裸机
- peek + drop 组合支持流式协议解析（帧边界检测、分包粘包）
- 静态初始化宏，无需运行时 init 也可使用

## 2 文件清单

| 文件 | 说明 |
|------|------|
| `mu_ringbuf.h` | 接口声明、结构体定义、初始化宏 |
| `mu_ringbuf.c` | 实现 |
| `test_mu_ringbuf.c` | 测试（支持分组运行） |
| `README.md` | 本文件 |

## 3 数据结构

```c
typedef struct {
    uint8_t              *buffer;       // 数据缓冲区，外部传入
    uint32_t              size;         // 缓冲区总大小
    uint32_t              head;         // 写指针
    uint32_t              tail;         // 读指针
    mu_ringbuf_mutex_fn_t enter_mutex;  // 加锁回调（可为 NULL）
    mu_ringbuf_mutex_fn_t exit_mutex;   // 解锁回调（可为 NULL）
} mu_ringbuf_t;
```

**容量说明**：实际可用容量为 `size - 1`，保留 1 字节用于区分空/满。

## 4 API 参考

### 4.1 初始化

| 宏/函数 | 说明 |
|------|------|
| `MU_RINGBUF_INIT(buf, sz, lock, unlock)` | 静态初始化宏，定义时赋值 |
| `MU_RINGBUF_DEFINE(name, buf, sz, lock, unlock)` | 定义 + 初始化一步完成 |
| `mu_ringbuf_init(p_rb, p_buf, size, enter, exit)` | 运行时初始化，返回 `mu_status_t` |

### 4.2 数据操作

| 函数 | 说明 |
|------|------|
| `mu_ringbuf_write(p_rb, p_data, len)` | 写入数据，空间不足自动截断，返回实际写入字节数 |
| `mu_ringbuf_read(p_rb, p_data, len)` | 读取数据，返回实际读取字节数 |
| `mu_ringbuf_write_byte(p_rb, data)` | 写入单字节，返回 1 成功 / 0 空间不足 |
| `mu_ringbuf_read_byte(p_rb, p_data)` | 读取单字节，返回 1 成功 / 0 空 |

### 4.3 协议解析（peek + drop）

| 函数 | 说明 |
|------|------|
| `mu_ringbuf_peek(p_rb, offset, p_data, len)` | 查看数据，不移动读指针，返回实际查看字节数 |
| `mu_ringbuf_drop(p_rb, len)` | 丢弃数据（移动读指针），返回实际丢弃字节数 |

### 4.4 状态查询

| 函数 | 说明 |
|------|------|
| `mu_ringbuf_get_free(p_rb)` | 剩余可写空间 |
| `mu_ringbuf_get_count(p_rb)` | 已存储数据量 |
| `mu_ringbuf_get_capacity(p_rb)` | 实际可用容量（size - 1） |
| `mu_ringbuf_is_empty(p_rb)` | 是否为空 |
| `mu_ringbuf_is_full(p_rb)` | 是否已满 |
| `mu_ringbuf_reset(p_rb)` | 清空队列，重置读写指针 |

## 5 使用例程

### 5.1 基本读写

```c
#include "mu_ringbuf.h"

uint8_t buf[128];
mu_ringbuf_t rb;
uint8_t data[32];

mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL );

// 写入
mu_ringbuf_write( &rb, ( uint8_t[] ){ 0x01, 0x02, 0x03 }, 3 );

// 读取
uint32_t n = mu_ringbuf_read( &rb, data, sizeof( data ) );
// n == 3, data = { 0x01, 0x02, 0x03 }
```

### 5.2 静态初始化

```c
uint8_t buf[128];
mu_ringbuf_t rb = MU_RINGBUF_INIT( buf, sizeof( buf ), NULL, NULL );

// 或一步定义
MU_RINGBUF_DEFINE( rb, buf, sizeof( buf ), NULL, NULL );
```

### 5.3 协议解析（peek + drop）

```c
// 等待完整帧
if( mu_ringbuf_get_count( &rb ) >= 4 )
{
    uint8_t header[4];
    mu_ringbuf_peek( &rb, 0, header, 4 );

    uint16_t payload_len = ( header[2] << 8 ) | header[3];
    uint32_t frame_len = 4 + payload_len + 2;  // header + payload + crc

    if( mu_ringbuf_get_count( &rb ) >= frame_len )
    {
        uint8_t frame[256];
        mu_ringbuf_read( &rb, frame, frame_len );
        // 处理完整帧...
    }
}
```

### 5.4 带互斥锁

**裸机场景**（关全局中断）：

```c
static void my_lock( void )
{
    __disable_irq();
}

static void my_unlock( void )
{
    __enable_irq();
}
```

**RTOS 场景**（使用互斥量，支持优先级继承）：

```c
static SemaphoreHandle_t g_mutex;

void app_init( void )
{
    g_mutex = xSemaphoreCreateMutex();  // 互斥量，非二值信号量
}

static void my_lock( void )
{
    // 有限超时，避免死等；ISR 中用 pdFALSE
    xSemaphoreTake( g_mutex, pdMS_TO_TICKS( 100 ) );
}

static void my_unlock( void )
{
    xSemaphoreGive( g_mutex );
}
```

> 注意：不要用 `xSemaphoreCreateBinary` + `xSemaphoreTake` 当锁，
> 二值信号量不具备优先级继承，可能导致优先级反转。

## 6 配置

无编译宏配置。互斥锁通过回调函数指针传入，灵活性由调用方控制。

## 7 注意事项

1. buffer 和 ringbuf 对象的生命周期由调用方管理，库不负责分配/释放
2. 多任务场景下必须传入互斥锁回调，否则数据可能损坏
3. 可用容量 = size - 1（需要 1 字节区分空/满）
4. peek 和 drop 组合使用时，中间不要穿插其他写入/读取操作
