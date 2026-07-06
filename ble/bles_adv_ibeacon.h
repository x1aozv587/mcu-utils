/**
 * @file    bles_adv_ibeacon.h
 * @author  lrz
 * @date    2026-07-06
 * @brief   Apple iBeacon 广播预设
 *
 * @details
 * 基于核心引擎 bles_adv_append 组装标准的 iBeacon 广播包。
 *
 * iBeacon 广播格式:
 *   Flags:
 *     02 01 06
 *
 *   Manufacturer:
 *     1A FF 4C 00 02 15
 *     [UUID:16]
 *     [Major:2, big endian]
 *     [Minor:2, big endian]
 *     [TX Power:1]
 *
 * @note
 * 1. UUID 按 UUID 字符串显示顺序填入。
 * 2. Major / Minor 为数值，组包时内部按大端序写入。
 * 3. tx_power 为 1 米处校准 RSSI，单位 dBm。
 * 4. iBeacon 共占 30 字节，在传统广播 31 字节限制内。
 */

#ifndef __BLES_ADV_IBEACON_H__
#define __BLES_ADV_IBEACON_H__

#include <stdint.h>
#include <stdbool.h>

#include "bles_adv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== iBeacon 参数 ==================== */

/**< iBeacon 配置参数 */
typedef struct
{
    uint8_t   uuid[16];     /**< Proximity UUID，按 UUID 字符串显示顺序填入 */
    uint16_t  major;        /**< Major 数值，组包时按大端写入 */
    uint16_t  minor;        /**< Minor 数值，组包时按大端写入 */
    int8_t    tx_power;     /**< 1 米处校准 RSSI，单位 dBm */
} bles_adv_ibeacon_params_t;

/* ==================== 组装接口 ==================== */

/**
 * @brief 组装 iBeacon 广播包
 *
 * 内部调用 bles_adv_append 写入 Flags 和 Manufacturer 两条 AD Structure。
 *
 * @param p_builder 组装器指针（需已初始化）
 * @param p_params  iBeacon 参数
 *
 * @return true 成功，false 空间不足或参数非法
 */
bool bles_adv_ibeacon_build( bles_adv_builder_t *p_builder,
                             const bles_adv_ibeacon_params_t *p_params );

#ifdef __cplusplus
}
#endif

#endif /* __BLES_ADV_IBEACON_H__ */
