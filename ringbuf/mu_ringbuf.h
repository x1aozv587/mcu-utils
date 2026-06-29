/**
 * @file    mu_ringbuf.h
 * @author  lrz
 * @date    2026-06-29
 * @brief   通用 ring buffer 接口定义
 *
 * @note
 * 本文件只声明 ring buffer 对外接口，内部实现细节放在 .c 文件中。
 */

#ifndef __MU_RINGBUF_H__
#define __MU_RINGBUF_H__

#include <stdint.h>
#include <stdbool.h>

#include "..\mu_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**< 互斥锁回调函数类型，由外部注册 */
typedef void ( *mu_ringbuf_mutex_fn_t )( void );

/**< 环形队列控制块 */
typedef struct
{
    uint8_t                *buffer;        /**< 数据缓冲区，外部传入 */
    uint32_t                size;          /**< 缓冲区总大小 */
    uint32_t                head;          /**< 写指针 */
    uint32_t                tail;          /**< 读指针 */
    mu_ringbuf_mutex_fn_t   enter_mutex;   /**< 加锁回调，可为 NULL */
    mu_ringbuf_mutex_fn_t   exit_mutex;    /**< 解锁回调，可为 NULL */
} mu_ringbuf_t;

/**< 静态初始化宏，用于在定义时直接赋值 */
#define MU_RINGBUF_INIT( buf, sz, lock, unlock ) \
    { ( buf ), ( sz ), 0, 0, ( lock ), ( unlock ) }

/**< 定义 + 初始化一步完成 */
#define MU_RINGBUF_DEFINE( name, buf, sz, lock, unlock ) \
    mu_ringbuf_t name = MU_RINGBUF_INIT( buf, sz, lock, unlock )

/**
 * @brief 运行时初始化，p_buffer 由外部传入，size 为缓冲区总大小
 *
 * @param p_rb ring buffer 对象
 * @param p_buffer 外部传入的缓存
 * @param size 缓存大小，必须 >= 2
 * @param enter_mutex 加锁回调
 * @param exit_mutex 解锁回调
 *
 * @return MU_OK 成功，MU_ERR_PARAM 参数非法
 */
mu_status_t mu_ringbuf_init( mu_ringbuf_t *p_rb,
                             uint8_t *p_buffer,
                             uint32_t size,
                             mu_ringbuf_mutex_fn_t enter_mutex,
                             mu_ringbuf_mutex_fn_t exit_mutex );

/**
 * @brief 写入数据，空间不足时自动截断
 *
 * @param p_rb ring buffer 对象
 * @param p_data 待写入数据
 * @param len 待写入长度
 *
 * @return 实际写入字节数
 */
uint32_t mu_ringbuf_write( mu_ringbuf_t *p_rb, const uint8_t *p_data, uint32_t len );

/**
 * @brief 读取数据
 *
 * @param p_rb ring buffer 对象
 * @param p_data 读取缓冲区
 * @param len 期望读取长度
 *
 * @return 实际读取字节数
 */
uint32_t mu_ringbuf_read( mu_ringbuf_t *p_rb, uint8_t *p_data, uint32_t len );

/**
 * @brief 获取剩余可写空间（总大小 - 已用 - 1）
 *
 * @param p_rb ring buffer 对象
 *
 * @return 剩余可写字节数
 */
uint32_t mu_ringbuf_get_free( mu_ringbuf_t *p_rb );

/**
 * @brief 获取当前已存储的数据量
 *
 * @param p_rb ring buffer 对象
 *
 * @return 已存储字节数
 */
uint32_t mu_ringbuf_get_count( mu_ringbuf_t *p_rb );

/**
 * @brief 获取实际可用容量（size - 1）
 *
 * @param p_rb ring buffer 对象
 *
 * @return 实际可用容量，非法对象返回 0
 */
uint32_t mu_ringbuf_get_capacity( mu_ringbuf_t *p_rb );

/**
 * @brief 判断队列是否为空
 *
 * @param p_rb ring buffer 对象
 *
 * @return true 表示空，false 表示非空
 */
bool mu_ringbuf_is_empty( mu_ringbuf_t *p_rb );

/**
 * @brief 判断队列是否已满（已用空间 >= size - 1）
 *
 * @param p_rb ring buffer 对象
 *
 * @return true 表示满，false 表示未满
 */
bool mu_ringbuf_is_full( mu_ringbuf_t *p_rb );

/**
 * @brief 清空队列，重置读写指针
 *
 * @param p_rb ring buffer 对象
 */
void mu_ringbuf_reset( mu_ringbuf_t *p_rb );

/**
 * @brief 查看数据但不移动读指针（协议解析用）
 *
 * @param p_rb ring buffer 对象
 * @param offset 从 tail 起的偏移量
 * @param p_data 读取缓冲区
 * @param len 期望读取长度
 *
 * @return 实际查看字节数
 */
uint32_t mu_ringbuf_peek( mu_ringbuf_t *p_rb,
                          uint32_t offset,
                          uint8_t *p_data,
                          uint32_t len );

/**
 * @brief 丢弃指定长度的数据（协议解析用）
 *
 * @param p_rb ring buffer 对象
 * @param len 期望丢弃长度，超过已有数据时丢弃所有
 *
 * @return 实际丢弃字节数
 */
uint32_t mu_ringbuf_drop( mu_ringbuf_t *p_rb, uint32_t len );

/**
 * @brief 写入单字节
 *
 * @param p_rb ring buffer 对象
 * @param data 待写入字节
 *
 * @return 1 表示写入成功，0 表示空间不足
 */
uint32_t mu_ringbuf_write_byte( mu_ringbuf_t *p_rb, uint8_t data );

/**
 * @brief 读取单字节
 *
 * @param p_rb ring buffer 对象
 * @param p_data 读取缓冲区
 *
 * @return 1 表示读取成功，0 表示队列为空
 */
uint32_t mu_ringbuf_read_byte( mu_ringbuf_t *p_rb, uint8_t *p_data );

#ifdef __cplusplus
}
#endif

#endif /* __MU_RINGBUF_H__ */
