/**
 * @file    bles_adv.h
 * @author  lrz
 * @date    2026-07-06
 * @brief   BLE 广播数据核心引擎
 *
 * @details
 * 本文件定义 AD Structure 的通用组装器。
 * 具体广播类型（外设、iBeacon 等）由预设文件提供，预设内部调用 bles_adv_append。
 *
 * @note
 * 1. 不涉及 BLE 协议栈，仅处理数据层面的构造。
 * 2. 全部使用外部传入的 buffer，无动态内存分配。
 * 3. capacity/pos 使用 uint16_t，为扩展广播预留空间。
 */

#ifndef __BLES_ADV_H__
#define __BLES_ADV_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../mu_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== AD Type 枚举 ==================== */

/**< AD Structure 类型（常用子集） */
typedef enum
{
    BLES_AD_TYPE_FLAGS                  = 0x01, /**< Flags */
    BLES_AD_TYPE_UUID16_INCOMPLETE      = 0x02, /**< 16-bit UUID 列表（不完整） */
    BLES_AD_TYPE_UUID16_COMPLETE        = 0x03, /**< 16-bit UUID 列表（完整） */
    BLES_AD_TYPE_UUID32_INCOMPLETE      = 0x04, /**< 32-bit UUID 列表（不完整） */
    BLES_AD_TYPE_UUID32_COMPLETE        = 0x05, /**< 32-bit UUID 列表（完整） */
    BLES_AD_TYPE_UUID128_INCOMPLETE     = 0x06, /**< 128-bit UUID 列表（不完整） */
    BLES_AD_TYPE_UUID128_COMPLETE       = 0x07, /**< 128-bit UUID 列表（完整） */
    BLES_AD_TYPE_SHORT_NAME             = 0x08, /**< 短名称 */
    BLES_AD_TYPE_COMPLETE_NAME          = 0x09, /**< 完整名称 */
    BLES_AD_TYPE_TX_POWER               = 0x0A, /**< 发射功率 */
    BLES_AD_TYPE_APPEARANCE             = 0x19, /**< 外观 */
    BLES_AD_TYPE_MANUFACTURER           = 0xFF, /**< 厂商自定义数据 */
} bles_ad_type_t;

/* ==================== Flags 值 ==================== */

#define BLES_ADV_FLAG_LE_LIMITED           ( 1 << 0 )  /**< LE 有限可发现模式 */
#define BLES_ADV_FLAG_LE_GENERAL           ( 1 << 1 )  /**< LE 一般可发现模式 */
#define BLES_ADV_FLAG_BR_EDR_NOT_SUP       ( 1 << 2 )  /**< 不支持 BR/EDR */
#define BLES_ADV_FLAG_LE_BR_EDR_SAME       ( 1 << 3 )  /**< LE + BR/EDR 同时支持（控制器） */
#define BLES_ADV_FLAG_LE_BR_EDR_SAME_HOST  ( 1 << 4 )  /**< LE + BR/EDR 同时支持（主机） */

/**< 通用外设广播 Flags 组合：可发现 + 不支持 BR/EDR */
#define BLES_ADV_FLAGS_GENERAL_NO_BREDR \
    ( BLES_ADV_FLAG_LE_GENERAL | BLES_ADV_FLAG_BR_EDR_NOT_SUP )

/* ==================== 组装器上下文 ==================== */

/**< AD 数据组装器 */
typedef struct
{
    uint16_t  capacity;      /**< 缓冲区容量 */
    uint16_t  pos;           /**< 当前写入位置 */
    uint8_t  *p_buf;         /**< 缓冲区指针 */
} bles_adv_builder_t;

/* ==================== 核心接口 ==================== */

/**
 * @brief 初始化组装器
 *
 * @param p_builder 组装器指针
 * @param p_buf     外部缓冲区
 * @param buf_size  缓冲区大小
 *
 * @return MU_OK 成功，MU_ERR_PARAM 参数非法
 */
mu_status_t bles_adv_builder_init( bles_adv_builder_t *p_builder,
                                   uint8_t *p_buf,
                                   uint16_t buf_size );

/**
 * @brief 追加一条 AD Structure（核心写入入口）
 *
 * 格式：[length:1][type:1][data:length-1]。
 * 所有预设文件通过本函数组合出完整广播包。
 *
 * @param p_builder 组装器指针
 * @param type      AD Type
 * @param p_data    数据指针（可为 NULL，需 data_len == 0）
 * @param data_len  数据长度
 *
 * @return true 成功，false 空间不足或参数非法
 */
bool bles_adv_append( bles_adv_builder_t *p_builder,
                      bles_ad_type_t type,
                      const uint8_t *p_data,
                      uint16_t data_len );

/**
 * @brief 获取当前已组装的数据长度
 *
 * @param p_builder 组装器指针
 *
 * @return 字节数
 */
uint16_t bles_adv_get_len( const bles_adv_builder_t *p_builder );

/* ==================== 常用便捷接口 ==================== */

/**
 * @brief 追加 Flags AD Structure
 *
 * @param p_builder 组装器指针
 * @param flags     Flags 值
 *
 * @return true 成功
 */
bool bles_adv_append_flags( bles_adv_builder_t *p_builder, uint8_t flags );

/**
 * @brief 追加设备名称 AD Structure
 *
 * @param p_builder 组装器指针
 * @param p_name    名称字符串（不允许空字符串）
 * @param complete  true=完整名称，false=短名称
 *
 * @return true 成功
 */
bool bles_adv_append_name( bles_adv_builder_t *p_builder,
                           const char *p_name,
                           bool complete );

/**
 * @brief 追加单个 16-bit UUID AD Structure
 *
 * @param p_builder 组装器指针
 * @param uuid      16-bit UUID（小端）
 * @param complete  true=完整列表，false=不完整列表
 *
 * @return true 成功
 */
bool bles_adv_append_uuid16( bles_adv_builder_t *p_builder,
                             uint16_t uuid,
                             bool complete );

/**
 * @brief 追加发射功率 AD Structure
 *
 * @param p_builder 组装器指针
 * @param tx_power  发射功率 dBm
 *
 * @return true 成功
 */
bool bles_adv_append_tx_power( bles_adv_builder_t *p_builder, int8_t tx_power );

/**
 * @brief 追加厂商自定义数据 AD Structure
 *
 * @param p_builder 组装器指针
 * @param p_data    自定义数据（含 Company ID 等，由调用方自行组装）
 * @param data_len  数据长度
 *
 * @return true 成功
 */
bool bles_adv_append_manufacturer( bles_adv_builder_t *p_builder,
                                   const uint8_t *p_data,
                                   uint16_t data_len );

#ifdef __cplusplus
}
#endif

#endif /* __BLES_ADV_H__ */
