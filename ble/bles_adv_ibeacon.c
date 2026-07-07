#include "bles_adv_ibeacon.h"

#include <string.h>

/**< iBeacon Flags: LE General Discoverable + BR/EDR Not Supported */
#define IBEACON_FLAGS \
    ( BLES_ADV_FLAG_LE_GENERAL | BLES_ADV_FLAG_BR_EDR_NOT_SUP )

/**< Apple Company ID */
#define APPLE_COMPANY_ID    0x004C

/**< iBeacon 子类型 */
#define IBEACON_SUBTYPE     0x02

/**< iBeacon 数据长度（UUID 16 + Major 2 + Minor 2 + TX 1 = 21） */
#define IBEACON_DATA_LEN    0x15

bool bles_adv_ibeacon_build( bles_adv_builder_t *p_builder,
                             const bles_adv_ibeacon_params_t *p_params )
{
    uint8_t buf[25];
    uint8_t flags = IBEACON_FLAGS;
    uint16_t old_pos = 0;

    if( p_builder == NULL || p_params == NULL )
    {
        return false;
    }

    /**< 防止未初始化的 builder */
    if( p_builder->p_buf == NULL || p_builder->capacity == 0U )
    {
        return false;
    }

    /**< iBeacon 占 30 字节，不允许追加到非空 builder */
    if( bles_adv_get_len( p_builder ) != 0U )
    {
        return false;
    }

    old_pos = p_builder->pos;

    /**< 1. Flags */
    if( bles_adv_append_flags( p_builder, flags ) == false )
    {
        p_builder->pos = old_pos;

        return false;
    }

    /**< 2. Manufacturer Data */
    buf[0]  = ( uint8_t )( APPLE_COMPANY_ID & 0xFFU );
    buf[1]  = ( uint8_t )( ( APPLE_COMPANY_ID >> 8U ) & 0xFFU );
    buf[2]  = IBEACON_SUBTYPE;
    buf[3]  = IBEACON_DATA_LEN;
    memcpy( &buf[4], p_params->uuid, 16 );
    buf[20] = ( uint8_t )( ( p_params->major >> 8U ) & 0xFFU );
    buf[21] = ( uint8_t )( p_params->major & 0xFFU );
    buf[22] = ( uint8_t )( ( p_params->minor >> 8U ) & 0xFFU );
    buf[23] = ( uint8_t )( p_params->minor & 0xFFU );
    buf[24] = ( uint8_t )p_params->tx_power;

    if( bles_adv_append_manufacturer( p_builder, buf, 25 ) == false )
    {
        p_builder->pos = old_pos;

        return false;
    }

    return true;
}
