/**
 * @file    mu_crc.h
 * @author  lrz
 * @date    2026-07-02
 * @brief   CRC 通用计算模块
 *
 * @details
 * 本文件定义 CRC 参数结构体和通用计算接口。
 * 各 CRC 算法通过预设参数 + 便捷 wrapper 实现，共享同一引擎。
 *
 * ## 接口分层
 *
 *  一次性（init + feed + finalize）
 *    mu_crc_calc()         逐位计算，不需要查表
 *    mu_crc_calc_tbl()     查表计算，外部提供 256 项表
 *
 *  裸 feed（手动控制寄存器）
 *    mu_crc_feed()         喂数据，纯计算，无 init 和 finalize
 *    mu_crc_feed_tbl()     喂数据查表版
 *
 *  续算（接收上次结果，追加数据）
 *    mu_crc_continue()      接收 finalized 值，自动反解再算
 *    mu_crc_continue_tbl()  续算查表版
 *
 * ## 场景选型
 *
 *  - 整包一次性计算           -> mu_crc_calc / wrapper 函数
 *  - 分片收包，手动管理寄存器   -> mu_crc_feed
 *  - 已有结果，追加新数据       -> mu_crc_continue / wrapper_continue
 */

#ifndef __MU_CRC_H__
#define __MU_CRC_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 计算模式宏 ==================== */

#define MU_CRC_MODE_TABLE       ( 0 )    /**< 查表法，速度快，占用 ROM */
#define MU_CRC_MODE_BITWISE     ( 1 )    /**< 逐位计算，速度慢，不占 ROM */

#ifndef MU_CRC_DEFAULT_MODE
#define MU_CRC_DEFAULT_MODE     MU_CRC_MODE_TABLE
#endif

/* ==================== CRC 参数结构体 ==================== */

/**< CRC 算法参数 */
typedef struct
{
    uint8_t     width;          /**< CRC 位宽，合法范围 8 ~ 32 */
    uint32_t    poly;           /**< 生成多项式（不含隐式最高位的 width 位值） */
    uint32_t    init;           /**< 寄存器初始值 */
    uint32_t    xor_out;        /**< 输出异或值，最终结果异或此值 */
    bool        ref_in;         /**< 输入字节是否按位反转（LSB-first 算法需设为 true） */
    bool        ref_out;        /**< 输出结果是否需要按位反转 */
} mu_crc_params_t;

/**< CRC 算法模型（参数 + 名称，用于预设算法注册） */
typedef struct
{
    const char      *p_name;    /**< 算法名称，仅用于调试和展示 */
    mu_crc_params_t param;      /**< 算法参数 */
} mu_crc_model_t;

/* ==================== 一次性计算 ==================== */

/**
 * @brief 通用 CRC 逐位计算（一次性）
 *
 * 内部流程: init 初始化 -> feed 数据处理 -> finalize 收尾
 * 不需要查表，不占用额外 ROM
 *
 * @param p_param 算法参数指针
 * @param p_data  数据指针 (NULL+len=0 返回 init 值)
 * @param len     数据长度（字节）
 *
 * @return CRC 计算结果，错误时返回 0
 */
uint32_t mu_crc_calc( const mu_crc_params_t *p_param,
                      const uint8_t *p_data,
                      uint32_t len );

/**
 * @brief 通用 CRC 查表计算（一次性）
 *
 * 与 mu_crc_calc 功能相同，通过外部 256 项 uint32_t 查表加速
 * 表由算法 wrapper 以 static const 方式提供，存放于 ROM
 *
 * @param p_param 算法参数指针
 * @param p_data  数据指针
 * @param len     数据长度（字节）
 * @param p_table 256 项查表指针 (NULL 时返回 0)
 *
 * @return CRC 计算结果，错误时返回 0
 */
uint32_t mu_crc_calc_tbl( const mu_crc_params_t *p_param,
                          const uint8_t *p_data,
                          uint32_t len,
                          const uint32_t *p_table );

/* ==================== 裸 feed ==================== */

/**
 * @brief 喂入数据（裸 feed）
 *
 * "feed" 是 CRC 哈希领域的通用术语，意为"将数据喂入计算引擎"。
 * 本函数是纯数据输入：给定当前寄存器值，处理后返回新的寄存器值。
 * 不执行 init，不执行 finalize（反射 异或输出）。
 *
 * 调用方负责管理寄存器生命周期：
 * - 首次：crc = init & mask
 * - 循环：crc = mu_crc_feed( param, crc, chunk, len )
 * - 收尾：手动 finalize（ref_out 反射 + xor_out 异或）
 *
 * @param p_param 算法参数
 * @param crc     当前寄存器值（首次调用应为 init & mask）
 * @param p_data  数据指针
 * @param len     数据长度
 *
 * @return 新的寄存器值
 */
uint32_t mu_crc_feed( const mu_crc_params_t *p_param,
                      uint32_t crc,
                      const uint8_t *p_data,
                      uint32_t len );

/**
 * @brief 喂入数据查表版（裸 feed）
 *
 * 与 mu_crc_feed 行为相同，通过外部查表加速。
 *
 * @param p_param 算法参数
 * @param crc     当前寄存器值
 * @param p_data  数据指针
 * @param len     数据长度
 * @param p_table 256 项 uint32_t 查表
 *
 * @return 新的寄存器值
 */
uint32_t mu_crc_feed_tbl( const mu_crc_params_t *p_param,
                          uint32_t crc,
                          const uint8_t *p_data,
                          uint32_t len,
                          const uint32_t *p_table );

/* ==================== 续算 ==================== */

/**
 * @brief 续算 CRC（追加数据）
 *
 * 场景：已有上一次 mu_crc_calc 的完整结果，现在收到更多数据，
 * 需要全部数据的 CRC 值，但不想重算已处理的部分。
 *
 * 内部：解除 finalize -> feed 新数据 -> 重新 finalize。
 *
 * @param p_param  算法参数
 * @param prev_crc 上次计算的 finalized 值
 * @param p_data   追加数据
 * @param len      追加数据长度
 *
 * @return 新的 CRC 值
 */
uint32_t mu_crc_continue( const mu_crc_params_t *p_param,
                          uint32_t prev_crc,
                          const uint8_t *p_data,
                          uint32_t len );

/**
 * @brief 续算 CRC 查表版（追加数据）
 *
 * 与 mu_crc_continue 行为相同，通过外部查表加速。
 */
uint32_t mu_crc_continue_tbl( const mu_crc_params_t *p_param,
                              uint32_t prev_crc,
                              const uint8_t *p_data,
                              uint32_t len,
                              const uint32_t *p_table );

/* ==================== 工具函数 ==================== */

/**
 * @brief 按位反转
 *
 * 将 data 的低 width 位按位反转（bit 0 和 bit width-1 交换）。
 * 手动 finalize 时可能需要调用。
 */
uint32_t mu_crc_reflect( uint32_t data, uint8_t width );

/* ================================================================
 * 使用例程
 * ================================================================ */

/**
 * @example 一次性计算 MODBUS CRC
 * @code
 *   uint16_t crc = mu_crc16_modbus( p_data, len );
 * @endcode
 *
 * @example 分片收包，手动 feed 累积
 * @code
 *   mu_crc_params_t param = { 16, 0x8005, 0xFFFF, 0x0000, true, true };
 *   uint32_t mask = 0xFFFF;
 *   uint8_t ch;
 *   uint32_t crc = param.init & mask;
 *
 *   while( get_byte( &ch ) == true )
 *   {
 *       crc = mu_crc_feed( &param, crc, &ch, 1 );
 *   }
 *
 *   if( param.ref_out ) crc = mu_crc_reflect( crc, param.width );
 *   crc ^= param.xor_out;
 *   uint16_t result = ( uint16_t )( crc & mask );
 * @endcode
 *
 * @example 已有结果追加新数据（wrapper 版本）
 * @code
 *   uint16_t prev = mu_crc16_modbus( data1, len1 );
 *   uint16_t crc  = mu_crc16_modbus_continue( prev, data2, len2 );
 *   // crc 等于 mu_crc16_modbus( data1+data2, len1+len2 )
 * @endcode
 *
 * @example 手动 init + feed + finalize（最灵活）
 * @code
 *   mu_crc_params_t param = { 16, 0x1021, 0x0000, 0x0000, false, false };
 *   uint32_t mask = 0xFFFF;
 *   uint32_t crc = param.init & mask;                // init
 *   crc = mu_crc_feed( &param, crc, data, len );     // feed
 *   // CCITT: ref_in=false, ref_out=false, 无需 reflect
 *   crc ^= param.xor_out;                            // finalize
 *   uint16_t result = ( uint16_t )( crc & mask );
 * @endcode
 */

#ifdef __cplusplus
}
#endif

#endif /* __MU_CRC_H__ */
