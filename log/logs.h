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

/**< 每条日志中的帧内容的最大大小
 * 注意该变量是在栈上，如果在 RTOS 中使用需要考虑多线程，确保栈空间是否足够
*/
#define LOGS_FRAME_MAX_SIZE     (512)

/**< 用于指示全局/模块控制日志的位宽 `32bits` 也可以修改成 `64bits`
 * 注意其中的 bit[0] 被作为 `en` 位，只有剩下的 31 位 bit[31:1] 是有效位
*/
typedef uint32_t  logs_mask_t;
#define LOGS_EN_MASK  ((logs_mask_t)1<<0UL)
#define LOGS_ALL_MASK (0xFFFFFFFFUL)

/**< 日志等级
 * 越重要的数值越小
*/
#define  LOGS_LEVEL_ENABLED   0
#define  LOGS_LEVEL_ERROR     1
#define  LOGS_LEVEL_WARN      2
#define  LOGS_LEVEL_INFO      3
#define  LOGS_LEVEL_DEBUG     4

typedef struct
{
    logs_mask_t    enabled;
    logs_mask_t    error;
    logs_mask_t    warning;
    logs_mask_t    info;
    logs_mask_t    debug;
} logs_level_t;

/**< 输出接口 */
typedef uint32_t  (*logs_write_t)(uint32_t flag, uint8_t *p_data, uint32_t len);
typedef void ( *logs_mutex_fn_t )( void );

typedef struct
{
    logs_write_t    write;
    logs_mutex_fn_t lock;
    logs_mutex_fn_t unlock;
} logs_opt_t;

/**< 对外接口 */
mu_status_t logs_init( logs_opt_t *p_opt );
mu_status_t logs_error( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
mu_status_t logs_warn( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
mu_status_t logs_info( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
mu_status_t logs_debug( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... );
void logs_set_bitmap( logs_mask_t mask, uint8_t level );
void logs_clear_bitmap( logs_mask_t mask );

/**< FLAG */
#define  LOGS_FLAG_NONE      0
#define  LOGS_FLAG_ERROR     1
#define  LOGS_FLAG_WARN      2
#define  LOGS_FLAG_INFO      3
#define  LOGS_FLAG_DEBUG     4

#ifdef __cplusplus
}
#endif

#endif /* __LOGS_H__ */
