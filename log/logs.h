/**
 * @file    logs.h
 * @author  lrz
 * @date    2026-07-17
 * @brief   分级日志模块接口定义
 *
 * @details
 * 提供 DEBUG / INFO / WARN / ERROR 日志输出，
 * 支持根据不同的 bitmask 控制日志输出
 *
 * @note
 * 考虑到日志模块通常只会创建一个，这一版本中不存在 *obj 进行传参
 * 只全局维护一个结构体，在遍历模块时比较方便
 *
 * @history
 * 2026-07-17 lrz 创建文件
 */

#ifndef __LOGS_H__
#define __LOGS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "../mu_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 每条日志帧内容的最大大小（栈上）
 *
 * @note 在 RTOS 中使用需要考虑多线程，确保栈空间是否足够
 */
#define LOGS_FRAME_MAX_SIZE     (512)

/**
 * @brief 全局 / 模块控制日志的位宽
 *
 * @note bit[0] 被保留为全局使能位 `en`，bit[31:1] 为模块有效位
 *       可通过修改 `logs_mask_t` 为 `uint64_t` 扩展位宽
 */
typedef uint32_t  logs_mask_t;

/**< 全局使能位掩码（bit[0]） */
#define LOGS_EN_MASK  ((logs_mask_t)1<<0UL)

/**< 全模块掩码（不含 bit[0] 使能位） */
#define LOGS_ALL_MASK ((logs_mask_t)(~LOGS_EN_MASK))

/**
 * @brief 日志等级
 *
 * @note 数值越小重要性越高，`LOGS_LEVEL_ENABLED` 仅用于 logs_set_bitmap
 */
#define  LOGS_LEVEL_ENABLED   0
#define  LOGS_LEVEL_ERROR     1
#define  LOGS_LEVEL_WARN      2
#define  LOGS_LEVEL_INFO      3
#define  LOGS_LEVEL_DEBUG     4

/**< 等级位图配置，logs_set_bitmap 根据 level 参数操作对应字段 */
typedef struct
{
    logs_mask_t    enabled;     /**< 模块使能位（bit[0] 为全局使能） */
    logs_mask_t    error;       /**< ERROR 等级使能 */
    logs_mask_t    warning;     /**< WARN  等级使能 */
    logs_mask_t    info;        /**< INFO  等级使能 */
    logs_mask_t    debug;       /**< DEBUG 等级使能 */
} logs_level_t;

/**
 * @brief 日志输出回调函数类型
 *
 * @param flag   日志 FLAG (LOGS_FLAG_ERROR / WARN / INFO / DEBUG)
 * @param p_data 已完成格式化的日志数据
 * @param len    数据长度
 *
 * @return 实际写入的字节数
 *
 * @note
 * 数据已经由日志库完成格式化，实现者需要做的仅仅是将数据发送到
 * 对应的输出设备上。常见实现方式：
 *
 * - 单个 Backend：直接调用相应输出函数
 *   (UART / RTT / USB / BLE 等)，根据 flag 做等级相关处理
 *
 * - 多个 Backend：在回调内部依次调用多个输出函数，实现一份日志
 *   同时输出到多个设备
 *
 * - 注意，日志库不会在格式化完成后再次格式化，也不保留 va_list，
 *   因此不存在 va_copy 的问题
 */
typedef uint32_t  (*logs_write_t)(uint32_t flag, uint8_t *p_data, uint32_t len);

/**< 互斥锁回调（为 NULL 时不加锁） */
typedef void ( *logs_mutex_fn_t )( void );

/**< 日志配置选项 */
typedef struct
{
    logs_write_t    write;      /**< 输出回调（必选） */
    logs_mutex_fn_t lock;       /**< 加锁回调（可选，为 NULL 不加锁） */
    logs_mutex_fn_t unlock;     /**< 解锁回调（可选，为 NULL 不解锁） */
} logs_opt_t;

/* ==================== 对外接口 ==================== */

/**
 * @brief 初始化日志模块
 *
 * @param p_opt 日志配置选项（write 为必选，lock/unlock 为可选）
 *
 * @return MU_OK 成功，MU_ERR_NULL_POINT 表示 p_opt 或 write 为空
 */
mu_status_t logs_init( logs_opt_t *p_opt );

/**
 * @brief 输出 ERROR 等级日志
 *
 * @param mask   模块位掩码
 * @param p_tag  模块标签字符串，输出格式为 [TAG]（可为 NULL，输出 [NULL]）
 * @param p_fmt  格式化字符串（为 NULL 时静默返回）
 * @param ...    可变参数
 *
 * @note 日志未使能或该模块/等级未打开时，函数静默返回不做任何操作
 */
void logs_error( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );

/**
 * @brief 输出 WARN 等级日志
 *
 * @param mask   模块位掩码
 * @param p_tag  模块标签字符串
 * @param p_fmt  格式化字符串
 * @param ...    可变参数
 *
 * @note 同 logs_error
 */
void logs_warn( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );

/**
 * @brief 输出 INFO 等级日志
 *
 * @param mask   模块位掩码
 * @param p_tag  模块标签字符串
 * @param p_fmt  格式化字符串
 * @param ...    可变参数
 *
 * @note 同 logs_error
 */
void logs_info( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );

/**
 * @brief 输出 DEBUG 等级日志
 *
 * @param mask   模块位掩码
 * @param p_tag  模块标签字符串
 * @param p_fmt  格式化字符串
 * @param ...    可变参数
 *
 * @note 同 logs_error
 */
void logs_debug( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );

/**
 * @brief 设置指定模块的日志等级
 *
 * @param mask  模块位掩码
 * @param level 目标等级，取值 LOGS_LEVEL_ENABLED / ERROR / WARN / INFO / DEBUG
 *
 * @note 设置为某等级时，低于该等级的位也会被打开，高于该等级的位会被清除
 *       例如设为 WARN 时，ERROR 也会输出，但 INFO/DEBUG 不会输出
 */
void logs_set_bitmap( logs_mask_t mask, uint8_t level );

/**
 * @brief 清除指定模块的所有日志等级位
 *
 * @param mask 模块位掩码
 */
void logs_clear_bitmap( logs_mask_t mask );

/* ==================== LOG FLAG ==================== */

/**< 日志 FLAG（区分输出等级，传给 write 回调的第一个参数） */
#define  LOGS_FLAG_NONE      0
#define  LOGS_FLAG_ERROR     1
#define  LOGS_FLAG_WARN      2
#define  LOGS_FLAG_INFO      3
#define  LOGS_FLAG_DEBUG     4

#ifdef __cplusplus
}
#endif

#endif /* __LOGS_H__ */
