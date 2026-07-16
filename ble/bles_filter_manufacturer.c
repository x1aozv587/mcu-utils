#include "bles_filter_manufacturer.h"

#include <string.h>

typedef struct
{
    const uint8_t *p_data;
    uint16_t       data_len;
} bles_filter_manufacturer_ctx_t;

static bool bles_filter_manufacturer_cb( const uint8_t *p_data, uint8_t len, void *p_arg );

/**< 厂商数据匹配回调：在所有数据中查找 p_data 子序列 */
static bool bles_filter_manufacturer_cb( const uint8_t *p_data, uint8_t len, void *p_arg )
{
    bles_filter_manufacturer_ctx_t *p_ctx = ( bles_filter_manufacturer_ctx_t * )p_arg;
    uint16_t i = 0;

    if( len < p_ctx->data_len )
    {
        return false;
    }

    for( i = 0; i <= ( uint16_t )( len - p_ctx->data_len ); i++ )
    {
        if( memcmp( &p_data[i], p_ctx->p_data, p_ctx->data_len ) == 0 )
        {
            return true;
        }
    }

    return false;
}

bool bles_filter_manufacturer_match( const uint8_t *p_adv, uint16_t adv_len,
                                     const uint8_t *p_data, uint16_t data_len )
{
    bles_filter_manufacturer_ctx_t ctx;

    if( p_data == NULL || data_len == 0U )
    {
        return false;
    }

    ctx.p_data    = p_data;
    ctx.data_len  = data_len;

    return bles_filter_find( p_adv, adv_len, BLES_AD_TYPE_MANUFACTURER,
                             bles_filter_manufacturer_cb, &ctx );
}
