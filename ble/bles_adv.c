#include "bles_adv.h"

#include <string.h>

/* ==================== 静态函数声明 ==================== */

static bool bles_adv_is_room( const bles_adv_builder_t *p_builder, uint16_t need );

/* ==================== 静态函数实现 ==================== */

/**< 检查缓冲区剩余空间（length + type + data） */
static bool bles_adv_is_room( const bles_adv_builder_t *p_builder, uint16_t need )
{
    if( ( uint32_t )p_builder->pos + 2U + need > p_builder->capacity )
    {
        return false;
    }

    return true;
}

/* ==================== 核心组装接口 ==================== */

mu_status_t bles_adv_builder_init( bles_adv_builder_t *p_builder,
                                   uint8_t *p_buf,
                                   uint16_t buf_size )
{
    if( p_builder == NULL || p_buf == NULL )
    {
        return MU_ERR_PARAM;
    }

    p_builder->p_buf    = p_buf;
    p_builder->capacity = buf_size;
    p_builder->pos      = 0;

    memset( p_buf, 0, buf_size );

    return MU_OK;
}

bool bles_adv_append( bles_adv_builder_t *p_builder,
                      bles_ad_type_t type,
                      const uint8_t *p_data,
                      uint16_t data_len )
{
    if( p_builder == NULL || p_builder->p_buf == NULL )
    {
        return false;
    }

    if( data_len > 0U && p_data == NULL )
    {
        return false;
    }

    if( bles_adv_is_room( p_builder, data_len ) == false )
    {
        return false;
    }

    /**< AD Structure: [length][type][data...] */
    p_builder->p_buf[p_builder->pos] = ( uint8_t )( data_len + 1U );
    p_builder->pos++;

    p_builder->p_buf[p_builder->pos] = ( uint8_t )type;
    p_builder->pos++;

    if( data_len > 0U )
    {
        memcpy( &p_builder->p_buf[p_builder->pos], p_data, data_len );
        p_builder->pos += data_len;
    }

    return true;
}

uint16_t bles_adv_get_len( const bles_adv_builder_t *p_builder )
{
    if( p_builder == NULL )
    {
        return 0;
    }

    return p_builder->pos;
}

/* ==================== 常用便捷接口 ==================== */

bool bles_adv_append_flags( bles_adv_builder_t *p_builder, uint8_t flags )
{
    return bles_adv_append( p_builder, BLES_AD_TYPE_FLAGS, &flags, 1 );
}

bool bles_adv_append_name( bles_adv_builder_t *p_builder,
                           const char *p_name,
                           bool complete )
{
    bles_ad_type_t type;

    if( complete == true )
    {
        type = BLES_AD_TYPE_COMPLETE_NAME;
    }
    else
    {
        type = BLES_AD_TYPE_SHORT_NAME;
    }

    return bles_adv_append( p_builder, type,
                            ( const uint8_t * )p_name,
                            ( uint16_t )strlen( p_name ) );
}

bool bles_adv_append_uuid16( bles_adv_builder_t *p_builder,
                             uint16_t uuid,
                             bool complete )
{
    bles_ad_type_t type;
    uint8_t buf[2];

    if( complete == true )
    {
        type = BLES_AD_TYPE_UUID16_COMPLETE;
    }
    else
    {
        type = BLES_AD_TYPE_UUID16_INCOMPLETE;
    }

    buf[0] = ( uint8_t )( uuid & 0xFFU );
    buf[1] = ( uint8_t )( ( uuid >> 8U ) & 0xFFU );

    return bles_adv_append( p_builder, type, buf, 2 );
}

bool bles_adv_append_tx_power( bles_adv_builder_t *p_builder, int8_t tx_power )
{
    return bles_adv_append( p_builder, BLES_AD_TYPE_TX_POWER,
                            ( const uint8_t * )&tx_power, 1 );
}

bool bles_adv_append_manufacturer( bles_adv_builder_t *p_builder,
                                   const uint8_t *p_data,
                                   uint16_t data_len )
{
    return bles_adv_append( p_builder, BLES_AD_TYPE_MANUFACTURER,
                            p_data, data_len );
}
