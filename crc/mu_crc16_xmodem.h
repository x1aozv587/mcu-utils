/**
 * @file    mu_crc16_xmodem.h
 * @author  lrz
 * @date    2026-07-02
 * @brief   CRC-16 XMODEM 算法接口
 *
 * @note
 * 本文件提供 CRC-16 XMODEM (CCITT) 的一次性计算接口和模型获取接口。
 */

#ifndef __MU_CRC16_XMODEM_H__
#define __MU_CRC16_XMODEM_H__

#include <stdint.h>

#include "mu_crc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 一次性计算 CRC-16 XMODEM
 *
 * @param p_data 数据指针
 * @param len    数据长度
 *
 * @return CRC-16 XMODEM 结果
 */
uint16_t mu_crc16_xmodem( const uint8_t *p_data, uint32_t len );

/**
 * @brief 续算 CRC-16 XMODEM（追加数据）
 *
 * 接收上次 mu_crc16_xmodem 的返回值，追加新数据后返回整体 CRC。
 * 无需重算已处理的数据。
 *
 * @param prev_crc 上次计算结果
 * @param p_data   追加数据
 * @param len      追加数据长度
 *
 * @return CRC-16 XMODEM 结果
 */
uint16_t mu_crc16_xmodem_continue( uint16_t prev_crc, const uint8_t *p_data, uint32_t len );

/**
 * @brief 获取 CRC-16 XMODEM 预设算法模型
 *
 * @return 算法模型指针（指向 ROM 中的 const 变量）
 */
const mu_crc_model_t * mu_get_crc16_xmodem_model( void );

#ifdef __cplusplus
}
#endif

#endif /* __MU_CRC16_XMODEM_H__ */
