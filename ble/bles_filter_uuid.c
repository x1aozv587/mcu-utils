#include "bles_filter_uuid.h"

#include <string.h>

typedef struct
{
    const uint8_t *p_uuid;
    uint8_t        uuid_len;
} bles_filter_uuid_ctx_t;

static bool bles_filter_uuid_cb( const uint8_t *p_data, uint8_t len, void *p_arg );

/**< UUID 匹配回调：遍历 UUID 列表逐个比对 */
static bool bles_filter_uuid_cb( const uint8_t *p_data, uint8_t len, void *p_arg )
{
    bles_filter_uuid_ctx_t *p_ctx = ( bles_filter_uuid_ctx_t * )p_arg;
    uint8_t i = 0;

    for( i = 0; i + p_ctx->uuid_len <= len; i += p_ctx->uuid_len )
    {
        if( memcmp( &p_data[i], p_ctx->p_uuid, p_ctx->uuid_len ) == 0 )
        {
            return true;
        }
    }

    return false;
}

bool bles_filter_uuid_match( const uint8_t *p_adv, uint16_t adv_len,
                             const uint8_t *p_uuid, uint8_t uuid_len )
{
    bles_filter_uuid_ctx_t ctx;
    bles_ad_type_t type_complete;
    bles_ad_type_t type_incomplete;

    if( p_uuid == NULL )
    {
        return false;
    }

    if( uuid_len == 2U )
    {
        type_complete   = BLES_AD_TYPE_UUID16_COMPLETE;
        type_incomplete = BLES_AD_TYPE_UUID16_INCOMPLETE;
    }
    else if( uuid_len == 16U )
    {
        type_complete   = BLES_AD_TYPE_UUID128_COMPLETE;
        type_incomplete = BLES_AD_TYPE_UUID128_INCOMPLETE;
    }
    else
    {
        return false;
    }

    ctx.p_uuid    = p_uuid;
    ctx.uuid_len  = uuid_len;

    if( bles_filter_find( p_adv, adv_len, type_complete,
                          bles_filter_uuid_cb, &ctx ) == true )
    {
        return true;
    }

    return bles_filter_find( p_adv, adv_len, type_incomplete,
                             bles_filter_uuid_cb, &ctx );
}
