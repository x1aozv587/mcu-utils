/**
 * @file    bles_filter.h
 * @author  lrz
 * @date    2026-07-09
 * @brief   BLE 广播过滤器核心
 *
 * @details
 * 核心提供 AD Structure 遍历能力，不包含具体匹配逻辑。
 * 各预设文件（名称、UUID、厂商等）调用 bles_filter_find 并传入回调。
 *
 * @note
 * 1. 依赖 bles_adv.h 中的 AD Type 定义。
 * 2. callback 为 NULL 时仅判断 type 是否存在。
 */

#ifndef __BLES_FILTER_H__
#define __BLES_FILTER_H__

#include <stdint.h>
#include <stdbool.h>

#include "bles_adv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool ( *bles_filter_ad_callback_t )( const uint8_t *p_data,
                                            uint8_t len,
                                            void *p_arg );

/**
 * @brief 在广播数据中查找指定类型的 AD Structure
 *
 * @param p_adv     广播数据指针
 * @param adv_len   广播数据长度
 * @param type      要查找的 AD Type
 * @param callback  匹配回调（NULL 表示仅判断是否存在）
 * @param p_arg     回调透传参数
 *
 * @return true 表示找到匹配项
 */
bool bles_filter_find( const uint8_t *p_adv, uint16_t adv_len,
                       bles_ad_type_t type,
                       bles_filter_ad_callback_t callback,
                       void *p_arg );

#ifdef __cplusplus
}
#endif

#endif /* __BLES_FILTER_H__ */
