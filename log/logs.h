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
#define LOGS_ALL_MASK (0xFFFFFFFFUL)

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

/**< 日志输出回调
 *
 * @param flag   日志 FLAG (LOGS_FLAG_ERROR / WARN / INFO / DEBUG)
 * @param p_data 格式化后的日志数据
 * @param len    数据长度
 *
 * @return 实际写入的字节数
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

mu_status_t logs_init( logs_opt_t *p_opt );
void logs_error( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
void logs_warn( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
void logs_info( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
void logs_debug( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
void logs_set_bitmap( logs_mask_t mask, uint8_t level );
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
