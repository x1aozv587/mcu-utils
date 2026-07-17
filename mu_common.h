#ifndef __MU_COMMON_H
#define __MU_COMMON_H

#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"

/**
 * @brief 通用返回状态枚举
 */
typedef enum
{
    MU_OK = 0,
    MU_ERR_PARAM = 1,                          /**< 参数错误   @ref 一般指参数不符合接口要求等 */
    MU_ERR_NULL_POINT,                         /**< 空指针异常 @ref 一般指参数中存在空指针 或者运行时产生了空指针 */
    MU_ERR_CONFIG_NOT_ENABLED,                 /**< 条件未使能 @ref 一般指当前库某些开关没有打开 */
    MU_ERR_SWITCH_NOT_ENABLED,                 /**< 开关未使能 @ref 一般指当前函数某些开关没有打开 */
    MU_ERR_NO_SPACE,                           /**< 空间不足   @ref 见名知意 */
    MU_ERR_INTERNAL,                           /**< 内部错误   @ref 一般指函数执行过程中出错返回 */
} mu_status_t;

#endif /**< __MU_COMMON_H */
