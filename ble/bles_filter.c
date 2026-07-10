#include "bles_filter.h"

bool bles_filter_find( const uint8_t *p_adv, uint16_t adv_len,
                       bles_ad_type_t type,
                       bles_filter_ad_callback_t callback,
                       void *p_arg )
{
    uint16_t pos = 0;

    if( p_adv == NULL )
    {
        return false;
    }

    while( pos < adv_len )
    {
        uint8_t field_len = p_adv[pos];
        uint8_t field_type;
        uint8_t data_len;

        if( field_len == 0U )
        {
            break;
        }

        if( ( uint32_t )pos + 1U + field_len > adv_len )
        {
            break;
        }

        pos++;
        field_type = p_adv[pos];
        pos++;
        data_len = field_len - 1U;

        if( field_type == ( uint8_t )type )
        {
            if( callback == NULL )
            {
                return true;
            }

            if( callback( &p_adv[pos], data_len, p_arg ) == true )
            {
                return true;
            }
        }

        pos += data_len;
    }

    return false;
}
