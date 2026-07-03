/**
 * @file    mu_battery.h
 * @author  lrz
 * @date    2026-07-03
 * @brief   电池计算模块
 *
 * @details
 * 本文件定义电池参数结构体、状态枚举和计算接口。
 * 支持 ADC 原始值到电压的转换、电压到电量的映射、滑动窗口滤波。
 *
 * @note
 * 1. 电压计算依赖外部传入的 ADC 值，不直接操作硬件。
 * 2. 电量映射支持分段线性插值。
 * 3. 所有电压参数（voltage_full_mv / voltage_empty_mv / voltage_low_mv）均为单电芯电压。
 *    曲线表 p_curve 中的 voltage_mv 也是单电芯电压，且必须从高到低排列。
 */

#ifndef __MU_BATTERY_H__
#define __MU_BATTERY_H__

#include <stdint.h>
#include <stdbool.h>

#include "..\mu_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 电池状态 ==================== */

/**< 电池状态 */
typedef enum
{
    MU_BATTERY_STATE_UNKNOWN = 0,   /**< 未知（初始化后尚未获取有效电压） */
    MU_BATTERY_STATE_LOW,           /**< 低电量 */
    MU_BATTERY_STATE_NORMAL,        /**< 正常 */
    MU_BATTERY_STATE_CHARGING,      /**< 充电中 */
    MU_BATTERY_STATE_FULL,          /**< 已充满 */
} mu_battery_state_t;

/* ==================== 电量映射表项 ==================== */

/**
 * @brief 电压-电量映射点（用于分段线性插值）
 *
 * @note
 * voltage_mv 为单电芯电压。
 * 曲线表必须按 voltage_mv 从高到低排列，percent 建议也从高到低。
 */
typedef struct
{
    uint16_t    voltage_mv;     /**< 电压，单位 mV（单电芯） */
    uint8_t     percent;        /**< 对应电量百分比 0~100 */
} mu_battery_curve_point_t;

/* ==================== 电池参数 ==================== */

/**< 电池配置参数 */
typedef struct
{
    uint8_t     cell_count;             /**< 电芯串联数（1S ~ 4S） */
    uint16_t    voltage_full_mv;        /**< 满电电压，单位 mV（单电芯） */
    uint16_t    voltage_empty_mv;       /**< 亏电电压，单位 mV（单电芯） */
    uint16_t    voltage_low_mv;         /**< 低电量告警阈值，单位 mV（单电芯） */
    uint16_t    adc_ref_mv;             /**< ADC 参考电压，单位 mV */
    uint16_t    adc_resolution;         /**< ADC 分辨率（如 4096 表示 12bit） */
    uint16_t    adc_divider_ratio;      /**< 分压比（R1+R2）/R2，放大 100 倍存储 */
    uint8_t     filter_window;          /**< 滑动滤波窗口大小（1~16，1 表示无滤波） */
    uint8_t     reserve;                /**< 预留 */
    const mu_battery_curve_point_t *p_curve;    /**< 自定义电量曲线表（NULL 则用线性），单电芯电压，从高到低排列 */
    uint8_t     curve_point_count;      /**< 曲线表点数 */
} mu_battery_params_t;

/* ==================== 电池上下文 ==================== */

/**< 电池运行上下文 */
typedef struct
{
    const mu_battery_params_t *p_params;    /**< 指向配置参数（必须在上下文存活期间保持有效，建议定义为 static const） */
    uint16_t    voltage_mv;                 /**< 当前滤波后电压，单位 mV */
    uint16_t    voltage_raw_mv;             /**< 当前原始电压，单位 mV */
    uint8_t     percent;                    /**< 当前电量百分比 0~100 */
    mu_battery_state_t state;              /**< 当前状态 */
    uint16_t    filter_buf[16];             /**< 滤波窗口缓冲 */
    uint8_t     filter_idx;                 /**< 滤波缓冲写入索引 */
    uint8_t     filter_cnt;                 /**< 已累积采样数 */
} mu_battery_t;

/* ==================== 宏 ==================== */

/**< 静态初始化宏 */
#define MU_BATTERY_INIT( params_ptr ) \
    { ( params_ptr ), 0, 0, 0, MU_BATTERY_STATE_UNKNOWN, {0}, 0, 0 }

/* ==================== 接口 ==================== */

/**
 * @brief 初始化电池上下文
 *
 * @param p_battery 电池上下文
 * @param p_params  电池配置参数
 *
 * @return MU_OK 成功，MU_ERR_PARAM 参数非法
 *
 * @note
 * p_params 指针仅在 init 时保存地址，不会复制参数内容。
 * 调用方必须保证 p_params 指向的内存在 p_battery 存活期间一直有效。
 * MCU 场景建议将参数定义为 static const。
 */
mu_status_t mu_battery_init( mu_battery_t *p_battery,
                             const mu_battery_params_t *p_params );

/**
 * @brief 喂入 ADC 原始值
 *
 * 将 ADC 读数转换为电压（mV）并做滑动滤波。
 * 每次调用后更新内部电压和电量百分比。
 *
 * @param p_battery 电池上下文
 * @param adc_raw   ADC 原始值
 */
void mu_battery_feed_adc( mu_battery_t *p_battery, uint16_t adc_raw );

/**
 * @brief 直接喂入电压值（跳过 ADC 转换）
 *
 * @param p_battery  电池上下文
 * @param voltage_mv 电压，单位 mV（整包总电压）
 */
void mu_battery_feed_voltage( mu_battery_t *p_battery, uint16_t voltage_mv );

/**
 * @brief 获取滤波后的电压
 *
 * @param p_battery 电池上下文
 *
 * @return 电压，单位 mV
 */
uint16_t mu_battery_get_voltage( const mu_battery_t *p_battery );

/**
 * @brief 获取电量百分比
 *
 * @param p_battery 电池上下文
 *
 * @return 百分比 0~100
 */
uint8_t mu_battery_get_percent( const mu_battery_t *p_battery );

/**
 * @brief 获取电池状态
 *
 * @param p_battery 电池上下文
 *
 * @return 电池状态，见 mu_battery_state_t
 */
mu_battery_state_t mu_battery_get_state( const mu_battery_t *p_battery );

/**
 * @brief 设置充电状态
 *
 * 充电状态通常由充电 IC 或 GPIO 检测，由调用方主动设置。
 *
 * @param p_battery 电池上下文
 * @param charging  true=充电中，false=未充电
 */
void mu_battery_set_charging( mu_battery_t *p_battery, bool charging );

#ifdef __cplusplus
}
#endif

#endif /* __MU_BATTERY_H__ */
