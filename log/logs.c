#include "logs.h"
#include <stdio.h>

typedef struct
{
    logs_level_t  levels;
    logs_opt_t    opt;
} logs_t;

static logs_t logs;

/**< 加锁，lock 为 NULL 时等效空操作 */
static void logs_lock(void)
{
    if( logs.opt.lock )
    {
        logs.opt.lock();
    }
}

/**< 解锁，unlock 为 NULL 时等效空操作 */
static void logs_unlock(void)
{
    if( logs.opt.unlock )
    {
        logs.opt.unlock();
    }
}

/**
 * @brief 格式化日志并输出
 *
 * @param flag           日志 FLAG，透传给 write 回调
 * @param p_tag          模块标签（为 NULL 时使用 "NULL"）
 * @param p_fmt          格式化字符串（必须非 NULL，调用方已检查）
 * @param p_params_list  可变参数列表
 *
 * @return vsnprintf 返回值（不包含 tag 部分的长度），<0 表示格式化失败
 */
static int logs_printf( uint32_t flag, const char * p_tag, const char* p_fmt, va_list p_params_list )
{
    char buf[LOGS_FRAME_MAX_SIZE + 1] = {0};
    int ret = 0, offset = 0;
    int total = 0;

    memset( buf, 0, LOGS_FRAME_MAX_SIZE + 1);

    logs_lock();

    /**< 填写 `tag` 并检查是否溢出，tag 为 NULL 时显示 "NULL" */
    if( p_tag == NULL )
    {
        p_tag = "NULL";
    }

    offset = snprintf( buf, LOGS_FRAME_MAX_SIZE, "[%s] ", p_tag );
    if( (offset < 0) || (offset > LOGS_FRAME_MAX_SIZE) )
    {
        /**< 出错 返回 0 字节长度 */
        logs_unlock();
        return 0;
    }

    ret = vsnprintf( buf + offset, LOGS_FRAME_MAX_SIZE - offset, p_fmt, p_params_list );
    if( ret < 0 )
    {
        /**< 格式化失败（编码异常等），静默返回 */
        logs_unlock();
        return 0;
    }

    if( logs.opt.write )
    {
        total = offset + ret;
        /**< 强制截断 保证字符串 `\0` 结尾 */
        if( total >= LOGS_FRAME_MAX_SIZE )
        {
            total = LOGS_FRAME_MAX_SIZE;
        }
        logs.opt.write( flag, (uint8_t*)buf, total );
    }

    logs_unlock();

    /**< 返回日志内容（不包含 `tag` 在内的总长度） */
    return ret;
}

/**
 * @brief 日志输出核心：全局使能检查 → 模块/等级过滤 → 格式化输出
 *
 * @param mask        模块位掩码
 * @param flag        日志 FLAG
 * @param level_bits  对应等级的位掩码字段
 * @param p_tag       模块标签（可为 NULL）
 * @param p_fmt       格式化字符串（为 NULL 时静默返回）
 * @param p_args      可变参数列表
 */
static void logs_output( logs_mask_t mask, uint32_t flag, logs_mask_t level_bits,
                         const char *p_tag, const char *p_fmt, va_list p_args )
{
    /**< fmt 为 NULL 时无内容可输出 */
    if( p_fmt == NULL )
    {
        return;
    }

    /**< 日志未使能 */
    if( ( logs.levels.enabled & LOGS_EN_MASK ) != LOGS_EN_MASK )
    {
        return;
    }

    /**< 当前模块没有打开 或者 当前等级未打开 */
    if( (( logs.levels.enabled & mask ) == 0) || (( level_bits & mask ) == 0) )
    {
        return;
    }

    logs_printf( flag, p_tag, p_fmt, p_args );
}

/* ==================== 初始化函数 ==================== */

mu_status_t logs_init( logs_opt_t *p_opt )
{
    if( (p_opt == NULL) || (p_opt->write == NULL) )
    {
        return MU_ERR_NULL_POINT;
    }

    /**< 允许 lock 选项为可选 */
    memcpy( &logs.opt, p_opt, sizeof(logs_opt_t) );

    return MU_OK;
}

/* ==================== 对外接口 ==================== */

void logs_error( logs_mask_t mask, const char * p_tag, const char *p_fmt, ... )
{
    va_list params_list;
    va_start(params_list, p_fmt);
    logs_output( mask, LOGS_FLAG_ERROR, logs.levels.error, p_tag, p_fmt, params_list );
    va_end(params_list);
}

void logs_warn( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... )
{
    va_list params_list;
    va_start(params_list, p_fmt);
    logs_output( mask, LOGS_FLAG_WARN, logs.levels.warning, p_tag, p_fmt, params_list );
    va_end(params_list);
}

void logs_info( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... )
{
    va_list params_list;
    va_start(params_list, p_fmt);
    logs_output( mask, LOGS_FLAG_INFO, logs.levels.info, p_tag, p_fmt, params_list );
    va_end(params_list);
}

void logs_debug( logs_mask_t mask, const char *p_tag, const char *p_fmt, ... )
{
    va_list params_list;
    va_start(params_list, p_fmt);
    logs_output( mask, LOGS_FLAG_DEBUG, logs.levels.debug, p_tag, p_fmt, params_list );
    va_end(params_list);
}

/* ==================== bitmap 配置 ==================== */

void logs_set_bitmap( logs_mask_t mask, uint8_t level )
{
    switch( level )
    {
        case LOGS_LEVEL_ENABLED:
        {
            logs.levels.enabled |= mask;
        }
        break;

        case LOGS_LEVEL_ERROR:
        {
            logs.levels.enabled    |= mask;
            logs.levels.error      |= mask;
            logs.levels.warning    &= ~(mask);
            logs.levels.info       &= ~(mask);
            logs.levels.debug      &= ~(mask);
        }
        break;

        case LOGS_LEVEL_WARN:
        {
            logs.levels.enabled    |= mask;
            logs.levels.error      |= mask;
            logs.levels.warning    |= mask;
            logs.levels.info       &= ~(mask);
            logs.levels.debug      &= ~(mask);
        }
        break;

        case LOGS_LEVEL_INFO:
        {
            logs.levels.enabled    |= mask;
            logs.levels.error      |= mask;
            logs.levels.warning    |= mask;
            logs.levels.info       |= mask;
            logs.levels.debug      &= ~(mask);
        }
        break;

        case LOGS_LEVEL_DEBUG:
        {
            logs.levels.enabled    |= mask;
            logs.levels.error      |= mask;
            logs.levels.warning    |= mask;
            logs.levels.info       |= mask;
            logs.levels.debug      |= mask;
        }
        break;

        default:
        {
        } break;
    }
}

void logs_clear_bitmap( logs_mask_t mask )
{
    logs.levels.enabled  &= ~mask;
    logs.levels.error    &= ~mask;
    logs.levels.warning  &= ~mask;
    logs.levels.info     &= ~mask;
    logs.levels.debug    &= ~mask;
}
