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

    if( p_builder == NULL || p_params == NULL )
    {
        return false;
    }

    /**< 1. Flags */
    if( bles_adv_append_flags( p_builder, flags ) == false )
    {
        return false;
    }

    /**< 2. Manufacturer: Company ID + iBeacon 子类型 + 数据长度 + 有效载荷 */
    buf[0]  = ( uint8_t )( APPLE_COMPANY_ID & 0xFFU );
    buf[1]  = ( uint8_t )( ( APPLE_COMPANY_ID >> 8U ) & 0xFFU );
    buf[2]  = IBEACON_SUBTYPE;
    buf[3]  = IBEACON_DATA_LEN;
    memcpy( &buf[4],  p_params->uuid, 16 );
    buf[20] = ( uint8_t )( ( p_params->major >> 8U ) & 0xFFU );
    buf[21] = ( uint8_t )( p_params->major & 0xFFU );
    buf[22] = ( uint8_t )( ( p_params->minor >> 8U ) & 0xFFU );
    buf[23] = ( uint8_t )( p_params->minor & 0xFFU );
    buf[24] = ( uint8_t )p_params->tx_power;

    return bles_adv_append_manufacturer( p_builder, buf, 25 );
}
