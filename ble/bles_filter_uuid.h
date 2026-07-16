/**
 * @file    bles_filter_uuid.h
 * @author  lrz
 * @date    2026-07-10
 * @brief   BLE UUID 过滤预设
 */

#ifndef __BLES_FILTER_UUID_H__
#define __BLES_FILTER_UUID_H__

#include <stdint.h>
#include <stdbool.h>

#include "bles_filter.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 按 UUID 过滤
 *
 * 在广播数据的 UUID16/UUID128 AD Structure 中查找匹配的 UUID。
 * uuid_len=2 时搜索 UUID16 列表，uuid_len=16 时搜索 UUID128 列表。
 *
 * @param p_adv     广播数据
 * @param adv_len   广播数据长度
 * @param p_uuid    目标 UUID（小端字节序）
 * @param uuid_len  UUID 长度（2 或 16）
 *
 * @return true 匹配成功

 * @note UUID32 (uuid_len=4) is not supported yet. Only 2 or 16 accepted.
 *       UUID uses little-endian byte order (BLE on-air format).
 */
bool bles_filter_uuid_match( const uint8_t *p_adv, uint16_t adv_len,
                             const uint8_t *p_uuid, uint8_t uuid_len );

#ifdef __cplusplus
}
#endif

#endif /* __BLES_FILTER_UUID_H__ */
