#include "bles_filter_name.h"

#include <string.h>

typedef struct
{
    const char               *p_name;
    uint8_t                   name_len;
    bles_filter_name_mode_t   mode;
} bles_filter_name_ctx_t;

static bool bles_filter_name_cb( const uint8_t *p_data, uint8_t len, void *p_arg );

/**< 名称匹配回调 */
static bool bles_filter_name_cb( const uint8_t *p_data, uint8_t len, void *p_arg )
{
    bles_filter_name_ctx_t *p_ctx = ( bles_filter_name_ctx_t * )p_arg;

    if( len < p_ctx->name_len )
    {
        return false;
    }

    switch( p_ctx->mode )
    {
        case BLES_FILTER_NAME_EXACT:
        {
            if( len == p_ctx->name_len &&
                memcmp( p_data, p_ctx->p_name, p_ctx->name_len ) == 0 )
            {
                return true;
            }
        } break;

        case BLES_FILTER_NAME_PREFIX:
        {
            if( memcmp( p_data, p_ctx->p_name, p_ctx->name_len ) == 0 )
            {
                return true;
            }
        } break;

        /**< todo 这里可能的优化就是检查首位并快速跳过 */
        /**< 因为 BLE ADV 里的数据都很少 采用朴素的比较并不会影响太多速度 */
        /**< 若有可行的优化方案后续再优化 */
        case BLES_FILTER_NAME_CONTAINS:
        {
            uint8_t i = 0;

            for( i = 0; i <= ( uint8_t )( len - p_ctx->name_len ); i++ )
            {
                if( memcmp( &p_data[i], p_ctx->p_name, p_ctx->name_len ) == 0 )
                {
                    return true;
                }
            }
        } break;

        default:
        {
            return false;
        } break;
    }

    return false;
}

bool bles_filter_name_match( const uint8_t *p_adv, uint16_t adv_len,
                             const char *p_name,
                             bles_filter_name_mode_t mode )
{
    bles_filter_name_ctx_t ctx;

    if( p_name == NULL )
    {
        return false;
    }

    size_t name_len;

    name_len = strlen( p_name );

    if( name_len == 0U || name_len > 255U )
    {
        return false;
    }

    ctx.name_len = ( uint8_t )name_len;

    ctx.p_name = p_name;
    ctx.mode   = mode;

    /**< 先查完整名称，再查短名称 */
    if( bles_filter_find( p_adv, adv_len, BLES_AD_TYPE_COMPLETE_NAME,
                          bles_filter_name_cb, &ctx ) == true )
    {
        return true;
    }

    return bles_filter_find( p_adv, adv_len, BLES_AD_TYPE_SHORT_NAME,
                             bles_filter_name_cb, &ctx );
}
