/**
 * @file    bles_filter_name.h
 * @author  lrz
 * @date    2026-07-10
 * @brief   BLE 名称过滤预设
 */

#ifndef __BLES_FILTER_NAME_H__
#define __BLES_FILTER_NAME_H__

#include <stdint.h>
#include <stdbool.h>

#include "bles_filter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BLES_FILTER_NAME_EXACT    = 0,
    BLES_FILTER_NAME_PREFIX   = 1,
    BLES_FILTER_NAME_CONTAINS = 2,
} bles_filter_name_mode_t;

/**
 * @brief 按设备名称过滤
 *
 * @param p_adv   广播数据
 * @param adv_len 广播数据长度
 * @param p_name  目标名称
 * @param mode    匹配模式
 *
 * @return true 匹配成功
 */
bool bles_filter_name_match( const uint8_t *p_adv, uint16_t adv_len,
                             const char *p_name,
                             bles_filter_name_mode_t mode );

#ifdef __cplusplus
}
#endif

#endif /* __BLES_FILTER_NAME_H__ */
