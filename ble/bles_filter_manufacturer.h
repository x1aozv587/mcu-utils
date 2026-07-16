/**
 * @file    bles_filter_manufacturer.h
 * @author  lrz
 * @date    2026-07-10
 * @brief   BLE 厂商数据过滤预设
 */

#ifndef __BLES_FILTER_MANUFACTURER_H__
#define __BLES_FILTER_MANUFACTURER_H__

#include <stdint.h>
#include <stdbool.h>

#include "bles_filter.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 按厂商自定义数据过滤（包含匹配）
 *
 * 在广播数据的 Manufacturer Specific Data 中查找 p_data 是否作为子序列存在。
 * 不指定偏移，不固定 company_id，调用方自行构造匹配数据。
 *
 * @param p_adv     广播数据
 * @param adv_len   广播数据长度
 * @param p_data    要查找的数据
 * @param data_len  数据长度
 *
 * @return true 匹配成功
 */
bool bles_filter_manufacturer_match( const uint8_t *p_adv, uint16_t adv_len,
                                     const uint8_t *p_data, uint16_t data_len );

#ifdef __cplusplus
}
#endif

#endif /* __BLES_FILTER_MANUFACTURER_H__ */
