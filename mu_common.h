#ifndef __MU_COMMON_H
#define __MU_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief 通用函数返回状态枚举
 *
 * @details
 * 本枚举定义了 mcu_utils 库中所有对外接口的统一返回状态。
 * 0 表示成功，非 0 表示各类失败原因。
 *
 * @note
 * 新增错误码时应明确描述触发条件，确保调用方能准确判断。
 */
typedef enum
{
    /**< 操作成功 */
    MU_OK = 0,

    /**< 参数无效（值越界、类型错误、格式不符等） */
    MU_ERR_PARAM = 1,

    /**< 空指针异常（传入的关键指针为 NULL） */
    MU_ERR_NULL_POINT,

    /**< 模块/功能全局未使能（整体功能未开启） */
    MU_ERR_CONFIG_NOT_ENABLED,

    /**< 子功能/开关未使能（模块已使能但具体选项未开） */
    MU_ERR_SWITCH_NOT_ENABLED,

    /**< 空间/资源不足（buffer 满、内存不够等） */
    MU_ERR_NO_SPACE,

    /**< 内部执行异常（不应出现的逻辑/状态错误） */
    MU_ERR_INTERNAL,
} mu_status_t;

#endif /**< __MU_COMMON_H */
