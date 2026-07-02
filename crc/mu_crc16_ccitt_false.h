/**
 * @file    mu_crc16_ccitt_false.h
 * @author  lrz
 * @date    2026-07-02
 * @brief   CRC-16 CCITT-FALSE 算法接口
 *
 * @note
 * 算法参数：poly=0x1021, init=0xFFFF, ref_in=false, ref_out=false
 */

#ifndef __MU_CRC16_CCITT_FALSE_H__
#define __MU_CRC16_CCITT_FALSE_H__

#include <stdint.h>

#include "mu_crc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 一次性计算 CRC-16 CCITT-FALSE
 *
 * @param p_data 数据指针
 * @param len    数据长度
 *
 * @return CRC-16 CCITT-FALSE 结果
 */
uint16_t mu_crc16_ccitt_false( const uint8_t *p_data, uint32_t len );

/**
 * @brief 续算 CRC-16 CCITT-FALSE（追加数据）
 *
 * 接收上次 mu_crc16_ccitt_false 的返回值，追加新数据后返回整体 CRC。
 *
 * @param prev_crc 上次计算结果
 * @param p_data   追加数据
 * @param len      追加数据长度
 *
 * @return CRC-16 CCITT-FALSE 结果
 */
uint16_t mu_crc16_ccitt_false_continue( uint16_t prev_crc,
                                        const uint8_t *p_data,
                                        uint32_t len );

/**
 * @brief 获取 CRC-16 CCITT-FALSE 预设算法模型
 *
 * @return 算法模型指针（指向 ROM 中的 const 变量）
 */
const mu_crc_model_t * mu_get_crc16_ccitt_false_model( void );

#ifdef __cplusplus
}
#endif

#endif /* __MU_CRC16_CCITT_FALSE_H__ */
