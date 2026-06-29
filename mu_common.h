#ifndef __MU_COMMON_H
#define __MU_COMMON_H

#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"

/**
 * @brief 通用返回状态枚举
 */
typedef enum
{
    MU_OK = 0,
    MU_ERR_PARAM = -1,
} mu_status_t;

#endif /**< __MU_COMMON_H */
