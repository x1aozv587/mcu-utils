#include "mu_crc.h"

/* ==================== 静态函数声明 ==================== */

static uint32_t mu_crc_width_mask( uint8_t width );
static uint32_t mu_crc_top_bit( uint8_t width );
static uint32_t mu_crc_unfinalize_bitwise( const mu_crc_params_t *p_param, uint32_t crc );
static uint32_t mu_crc_unfinalize_tbl( const mu_crc_params_t *p_param, uint32_t crc );

/* ==================== 静态函数 ==================== */

static uint32_t mu_crc_width_mask( uint8_t width )
{
    if( width >= 32U )
    {
        return 0xFFFFFFFFUL;
    }

    return ( 1UL << width ) - 1UL;
}


static uint32_t mu_crc_top_bit( uint8_t width )
{
    return 1UL << ( width - 1U );
}


static uint32_t mu_crc_unfinalize_bitwise( const mu_crc_params_t *p_param,
                                           uint32_t crc )
{
    uint32_t mask = mu_crc_width_mask( p_param->width );

    crc ^= p_param->xor_out;

    if( p_param->ref_out == true )
    {
        crc = mu_crc_reflect( crc, p_param->width );
    }

    crc &= mask;

    return crc;
}


static uint32_t mu_crc_unfinalize_tbl( const mu_crc_params_t *p_param,
                                       uint32_t crc )
{
    uint32_t mask = mu_crc_width_mask( p_param->width );

    crc ^= p_param->xor_out;

    if( p_param->ref_in != p_param->ref_out )
    {
        crc = mu_crc_reflect( crc, p_param->width );
    }

    crc &= mask;

    return crc;
}

/* ==================== 对外接口 ==================== */

uint32_t mu_crc_reflect( uint32_t data, uint8_t width )
{
    uint8_t i = 0;
    uint32_t ret = 0;

    for( i = 0; i < width; i++ )
    {
        if( ( data & 0x01U ) != 0U )
        {
            ret |= ( 1UL << ( width - 1U - i ) );
        }

        data >>= 1U;
    }

    return ret;
}


uint32_t mu_crc_feed( const mu_crc_params_t *p_param,
                      uint32_t crc,
                      const uint8_t *p_data,
                      uint32_t len )
{
    uint32_t mask = 0;
    uint32_t top_bit = 0;
    uint32_t poly = 0;
    uint32_t i = 0;
    uint8_t bit = 0;
    uint8_t data = 0;

    if( p_param == NULL || p_data == NULL || len == 0U )
    {
        return crc;
    }

    mask = mu_crc_width_mask( p_param->width );
    top_bit = mu_crc_top_bit( p_param->width );
    poly = p_param->poly & mask;

    for( i = 0; i < len; i++ )
    {
        data = p_data[i];

        if( p_param->ref_in == true )
        {
            data = ( uint8_t )mu_crc_reflect( data, 8U );
        }

        crc ^= ( ( uint32_t )data << ( p_param->width - 8U ) );

        for( bit = 0; bit < 8U; bit++ )
        {
            if( ( crc & top_bit ) != 0U )
            {
                crc = ( crc << 1U ) ^ poly;
            }
            else
            {
                crc <<= 1U;
            }

            crc &= mask;
        }
    }

    return crc;
}


uint32_t mu_crc_feed_tbl( const mu_crc_params_t *p_param,
                          uint32_t crc,
                          const uint8_t *p_data,
                          uint32_t len,
                          const uint32_t *p_table )
{
    uint32_t mask = 0;
    uint32_t i = 0;
    uint8_t byte = 0;
    uint8_t shift = 0;

    if( p_param == NULL || p_data == NULL || len == 0U || p_table == NULL )
    {
        return crc;
    }

    mask  = mu_crc_width_mask( p_param->width );
    shift = p_param->width - 8U;

    for( i = 0; i < len; i++ )
    {
        byte = p_data[i];

        if( p_param->ref_in == true )
        {
            crc = ( crc >> 8U ) ^ p_table[( crc ^ byte ) & 0xFFU];
        }
        else
        {
            crc = ( crc << 8U ) ^
                  p_table[( ( crc >> shift ) ^ byte ) & 0xFFU];
        }

        crc &= mask;
    }

    return crc;
}


uint32_t mu_crc_calc( const mu_crc_params_t *p_param,
                      const uint8_t *p_data,
                      uint32_t len )
{
    uint32_t crc = 0;
    uint32_t mask = 0;

    if( p_param == NULL )
    {
        return 0;
    }

    if( p_data == NULL && len > 0U )
    {
        return 0;
    }

    if( p_param->width < 8U || p_param->width > 32U )
    {
        return 0;
    }

    mask = mu_crc_width_mask( p_param->width );
    crc  = p_param->init & mask;

    crc = mu_crc_feed( p_param, crc, p_data, len );

    if( p_param->ref_out == true )
    {
        crc = mu_crc_reflect( crc, p_param->width );
    }

    crc ^= p_param->xor_out;
    crc &= mask;

    return crc;
}


uint32_t mu_crc_calc_tbl( const mu_crc_params_t *p_param,
                          const uint8_t *p_data,
                          uint32_t len,
                          const uint32_t *p_table )
{
    uint32_t crc = 0;
    uint32_t mask = 0;

    if( p_param == NULL || p_table == NULL )
    {
        return 0;
    }

    if( p_data == NULL && len > 0U )
    {
        return 0;
    }

    if( p_param->width < 8U || p_param->width > 32U )
    {
        return 0;
    }

    mask = mu_crc_width_mask( p_param->width );
    crc  = p_param->init & mask;

    crc = mu_crc_feed_tbl( p_param, crc, p_data, len, p_table );

    if( p_param->ref_in != p_param->ref_out )
    {
        crc = mu_crc_reflect( crc, p_param->width );
    }

    crc ^= p_param->xor_out;
    crc &= mask;

    return crc;
}


uint32_t mu_crc_continue( const mu_crc_params_t *p_param,
                          uint32_t prev_crc,
                          const uint8_t *p_data,
                          uint32_t len )
{
    uint32_t crc = 0;
    uint32_t mask = 0;

    if( p_param == NULL )
    {
        return 0;
    }

    if( p_data == NULL && len > 0U )
    {
        return 0;
    }

    if( p_param->width < 8U || p_param->width > 32U )
    {
        return 0;
    }

    mask = mu_crc_width_mask( p_param->width );
    crc  = mu_crc_unfinalize_bitwise( p_param, prev_crc );
    crc  = mu_crc_feed( p_param, crc, p_data, len );

    if( p_param->ref_out == true )
    {
        crc = mu_crc_reflect( crc, p_param->width );
    }

    crc ^= p_param->xor_out;
    crc &= mask;

    return crc;
}


uint32_t mu_crc_continue_tbl( const mu_crc_params_t *p_param,
                              uint32_t prev_crc,
                              const uint8_t *p_data,
                              uint32_t len,
                              const uint32_t *p_table )
{
    uint32_t crc = 0;
    uint32_t mask = 0;

    if( p_param == NULL || p_table == NULL )
    {
        return 0;
    }

    if( p_data == NULL && len > 0U )
    {
        return 0;
    }

    if( p_param->width < 8U || p_param->width > 32U )
    {
        return 0;
    }

    mask = mu_crc_width_mask( p_param->width );
    crc  = mu_crc_unfinalize_tbl( p_param, prev_crc );
    crc  = mu_crc_feed_tbl( p_param, crc, p_data, len, p_table );

    if( p_param->ref_in != p_param->ref_out )
    {
        crc = mu_crc_reflect( crc, p_param->width );
    }

    crc ^= p_param->xor_out;
    crc &= mask;

    return crc;
}
