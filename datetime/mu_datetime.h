/**
 * @file    mu_datetime.h
 * @author  lrz
 * @date    2026-07-03
 * @brief   日期时间计算模块
 *
 * @details
 * 本模块提供轻量级日期时间处理能力，适合 MCU 裸机或 RTOS 环境使用。
 * 支持日期合法性校验、闰年判断、星期计算、年内天数计算、
 * 时间戳互转、日期时间加减、比较、简单格式化和解析。
 *
 * @note
 * 1. 本库不依赖 malloc。
 * 2. 本库不依赖 RTOS。
 * 3. 本库不处理复杂时区和夏令时。
 * 4. 时间戳 epoch 可通过 MCUS_DATETIME_EPOCH 配置。
 * 5. 默认支持年份范围为 1970~2099。
 */

#ifndef __MU_DATETIME_H__
#define __MU_DATETIME_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== epoch 配置 ==================== */

#define MU_DATETIME_EPOCH_1970       ( 0 )   /**< Unix 纪元：1970-01-01 00:00:00 */
#define MU_DATETIME_EPOCH_2000       ( 1 )   /**< 2000 纪元：2000-01-01 00:00:00 */

#ifndef MU_DATETIME_EPOCH
#define MU_DATETIME_EPOCH            MU_DATETIME_EPOCH_1970
#endif

/**< 1970-01-01 到 2000-01-01 的秒数 */
#define MU_SECONDS_1970_TO_2000      946684800UL

/* ==================== 星期枚举 ==================== */

/**< 星期 */
typedef enum
{
    MU_WEEKDAY_SUNDAY    = 0,   /**< 周日 */
    MU_WEEKDAY_MONDAY    = 1,   /**< 周一 */
    MU_WEEKDAY_TUESDAY   = 2,   /**< 周二 */
    MU_WEEKDAY_WEDNESDAY = 3,   /**< 周三 */
    MU_WEEKDAY_THURSDAY  = 4,   /**< 周四 */
    MU_WEEKDAY_FRIDAY    = 5,   /**< 周五 */
    MU_WEEKDAY_SATURDAY  = 6,   /**< 周六 */
} mu_weekday_t;

/* ==================== 结构体 ==================== */

/**< 日期 */
typedef struct
{
    uint16_t    year;       /**< 年（1970~2099 或 2000~2099） */
    uint8_t     month;      /**< 月（1~12） */
    uint8_t     day;        /**< 日（1~31） */
} mu_date_t;

/**< 时间 */
typedef struct
{
    uint8_t     hour;       /**< 时（0~23） */
    uint8_t     minute;     /**< 分（0~59） */
    uint8_t     second;     /**< 秒（0~59） */
} mu_time_t;

/**< 日期时间组合 */
typedef struct
{
    mu_date_t   date;       /**< 日期 */
    mu_time_t   time;       /**< 时间 */
} mu_datetime_t;

/* ==================== 校验 ==================== */

bool mu_date_is_valid( const mu_date_t *p_date );
bool mu_time_is_valid( const mu_time_t *p_time );
bool mu_datetime_is_valid( const mu_datetime_t *p_dt );

/* ==================== 查询 ==================== */

bool mu_date_is_leap_year( uint16_t year );
uint8_t mu_date_days_in_month( uint16_t year, uint8_t month );
mu_weekday_t mu_date_weekday( const mu_date_t *p_date );
uint16_t mu_date_day_of_year( const mu_date_t *p_date );

/* ==================== 时间戳互转 ==================== */

/**
 * @brief 日期时间转时间戳
 *
 * @param p_dt 日期时间指针
 *
 * @return 时间戳，非法输入返回 0
 * @note 1970 epoch 下 1970-01-01 返回 0，与非法输入无法区分，推荐用 _ex 版本
 */
uint32_t mu_datetime_to_timestamp( const mu_datetime_t *p_dt );

/**
 * @brief 日期时间转时间戳（安全版）
 *
 * @param p_dt        日期时间指针
 * @param p_timestamp 输出时间戳
 *
 * @return true 成功，false 非法输入
 */
bool mu_datetime_to_timestamp_ex( const mu_datetime_t *p_dt, uint32_t *p_timestamp );

/**
 * @brief 时间戳转日期时间
 *
 * @param timestamp 时间戳
 * @param p_dt      输出日期时间
 *
 * @return true 成功，false 时间戳超出范围
 */
bool mu_timestamp_to_datetime( uint32_t timestamp, mu_datetime_t *p_dt );

/* ==================== 运算 ==================== */

bool mu_date_add_days( mu_date_t *p_date, int32_t days );
int32_t mu_date_diff_days( const mu_date_t *p_date1, const mu_date_t *p_date2 );

/**
 * @brief 时间加秒数
 *
 * @param p_time  输入时间，结果原地修改
 * @param seconds 增加秒数（可为负数）
 *
 * @return true 结果在 00:00:00 ~ 23:59:59 内，false 跨天或非法
 */
bool mu_time_add_seconds( mu_time_t *p_time, int32_t seconds );

/* ==================== 比较 ==================== */

int8_t mu_date_compare( const mu_date_t *p_date1, const mu_date_t *p_date2 );
int8_t mu_time_compare( const mu_time_t *p_time1, const mu_time_t *p_time2 );

#ifdef __cplusplus
}
#endif

#endif /* __MU_DATETIME_H__ */
