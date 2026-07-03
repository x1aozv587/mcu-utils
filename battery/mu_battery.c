#include "mu_battery.h"

#include <string.h>

/* ==================== 工具函数 ==================== */

static uint8_t clamp_percent( uint32_t percent )
{
    if( percent > 100U )
    {
        return 100;
    }

    return ( uint8_t )percent;
}

/* ==================== 曲线表校验 ==================== */

static bool is_valid_curve( const mu_battery_curve_point_t *p_curve,
                            uint8_t count )
{
    uint8_t i = 0;

    if( p_curve == NULL || count < 2U )
    {
        return false;
    }

    for( i = 0; i < ( uint8_t )( count - 1U ); i++ )
    {
        /**< 电压必须严格从高到低 */
        if( p_curve[i].voltage_mv <= p_curve[i + 1U].voltage_mv )
        {
            return false;
        }

        /**< 百分比必须不递增（允许相等） */
        if( p_curve[i].percent < p_curve[i + 1U].percent )
        {
            return false;
        }

        /**< 百分比必须在合法范围内 */
        if( p_curve[i].percent > 100U ||
            p_curve[i + 1U].percent > 100U )
        {
            return false;
        }
    }

    return true;
}

/* ==================== 参数校验 ==================== */

static bool is_valid_params( const mu_battery_params_t *p_params )
{
    if( p_params == NULL )
    {
        return false;
    }

    if( p_params->cell_count == 0U || p_params->cell_count > 4U )
    {
        return false;
    }

    if( p_params->voltage_full_mv <= p_params->voltage_empty_mv )
    {
        return false;
    }

    if( p_params->adc_ref_mv == 0U )
    {
        return false;
    }

    if( p_params->adc_resolution == 0U )
    {
        return false;
    }

    if( p_params->adc_divider_ratio == 0U )
    {
        return false;
    }

    if( p_params->voltage_low_mv < p_params->voltage_empty_mv ||
        p_params->voltage_low_mv > p_params->voltage_full_mv )
    {
        return false;
    }

    if( p_params->filter_window == 0U || p_params->filter_window > 16U )
    {
        return false;
    }

    if( p_params->p_curve != NULL )
    {
        if( is_valid_curve( p_params->p_curve,
                            p_params->curve_point_count ) == false )
        {
            return false;
        }
    }
    else
    {
        if( p_params->curve_point_count != 0U )
        {
            return false;
        }
    }

    return true;
}

/* ==================== 初始化 ==================== */

mu_status_t mu_battery_init( mu_battery_t *p_battery,
                             const mu_battery_params_t *p_params )
{
    if( p_battery == NULL || is_valid_params( p_params ) == false )
    {
        return MU_ERR_PARAM;
    }

    memset( p_battery, 0, sizeof( mu_battery_t ) );

    p_battery->p_params = p_params;
    p_battery->state    = MU_BATTERY_STATE_UNKNOWN;

    return MU_OK;
}

/* ==================== 电压转换 ==================== */

static uint16_t adc_to_voltage( const mu_battery_params_t *p_params,
                                uint16_t adc_raw )
{
    uint64_t voltage = 0;

    /**< voltage_mv = adc_raw * adc_ref_mv * divider_ratio / (resolution * 100) */
    voltage = ( uint64_t )adc_raw *
              p_params->adc_ref_mv *
              p_params->adc_divider_ratio;

    voltage = voltage /
              ( ( uint64_t )p_params->adc_resolution * 100U );

    if( voltage > 0xFFFFU )
    {
        voltage = 0xFFFFU;
    }

    return ( uint16_t )voltage;
}

/* ==================== 滑动滤波 ==================== */

static uint16_t filter_voltage( mu_battery_t *p_battery, uint16_t voltage_mv )
{
    uint16_t result = 0;
    uint8_t i = 0;
    uint32_t sum = 0;
    uint8_t count = 0;

    /**< 存入环形缓冲 */
    p_battery->filter_buf[p_battery->filter_idx] = voltage_mv;
    p_battery->filter_idx = ( p_battery->filter_idx + 1U ) %
                            p_battery->p_params->filter_window;

    if( p_battery->filter_cnt < p_battery->p_params->filter_window )
    {
        p_battery->filter_cnt++;
    }

    /**< 计算均值 */
    count = p_battery->filter_cnt;

    for( i = 0; i < count; i++ )
    {
        sum += p_battery->filter_buf[i];
    }

    result = ( uint16_t )( sum / count );

    return result;
}

/* ==================== 电量百分比 ==================== */

static uint8_t calc_percent_linear_by_cell( const mu_battery_params_t *p_params,
                                            uint16_t cell_mv )
{
    uint32_t percent = 0;
    uint32_t denominator = 0;

    if( cell_mv >= p_params->voltage_full_mv )
    {
        return 100;
    }

    if( cell_mv <= p_params->voltage_empty_mv )
    {
        return 0;
    }

    denominator = p_params->voltage_full_mv - p_params->voltage_empty_mv;
    percent    = ( uint32_t )( cell_mv - p_params->voltage_empty_mv ) * 100U;

    /**< 四舍五入 */
    percent = ( percent + denominator / 2U ) / denominator;

    return clamp_percent( percent );
}

static uint8_t calc_percent_curve( const mu_battery_params_t *p_params,
                                   uint16_t cell_mv )
{
    const mu_battery_curve_point_t *p_curve = p_params->p_curve;
    uint8_t count = p_params->curve_point_count;
    uint8_t i = 0;

    if( p_curve == NULL || count < 2U )
    {
        return calc_percent_linear_by_cell( p_params, cell_mv );
    }

    /**< 高于最高电压点 */
    if( cell_mv >= p_curve[0].voltage_mv )
    {
        return clamp_percent( p_curve[0].percent );
    }

    /**< 低于最低电压点 */
    if( cell_mv <= p_curve[count - 1U].voltage_mv )
    {
        return clamp_percent( p_curve[count - 1U].percent );
    }

    /**< 分段线性插值（曲线从高到低排列） */
    for( i = 0; i < ( uint8_t )( count - 1U ); i++ )
    {
        uint32_t v_high = p_curve[i].voltage_mv;
        uint32_t v_low  = p_curve[i + 1U].voltage_mv;
        uint32_t p_high = p_curve[i].percent;
        uint32_t p_low  = p_curve[i + 1U].percent;

        if( cell_mv <= v_high && cell_mv >= v_low )
        {
            uint32_t v_range  = v_high - v_low;
            uint32_t p_range  = p_high - p_low;
            uint32_t v_offset = cell_mv - v_low;
            uint32_t percent  = 0;

            if( v_range == 0U )
            {
                return clamp_percent( p_low );
            }

            percent = p_low +
                      ( v_offset * p_range + v_range / 2U ) /
                      v_range;

            return clamp_percent( percent );
        }
    }

    return 0;
}

static uint8_t calc_percent( const mu_battery_params_t *p_params,
                             uint16_t voltage_mv )
{
    uint16_t cell_mv = 0;

    if( p_params == NULL || p_params->cell_count == 0U )
    {
        return 0;
    }

    /**< 整包电压转单电芯电压 */
    cell_mv = voltage_mv / ( uint16_t )p_params->cell_count;

    if( p_params->p_curve != NULL && p_params->curve_point_count >= 2U )
    {
        return calc_percent_curve( p_params, cell_mv );
    }

    return calc_percent_linear_by_cell( p_params, cell_mv );
}

/* ==================== 状态更新 ==================== */

static void update_state( mu_battery_t *p_battery )
{
    uint32_t total_low = 0;

    total_low = ( uint32_t )p_battery->p_params->voltage_low_mv *
                p_battery->p_params->cell_count;

    /**< 充电中状态由 set_charging 管理，此处仅更新 FULL */
    if( p_battery->state == MU_BATTERY_STATE_CHARGING )
    {
        if( p_battery->percent >= 100U )
        {
            p_battery->state = MU_BATTERY_STATE_FULL;
        }

        return;
    }

    if( p_battery->voltage_mv == 0U )
    {
        return;
    }

    if( p_battery->percent >= 100U )
    {
        p_battery->state = MU_BATTERY_STATE_FULL;
    }
    else if( p_battery->voltage_mv <= total_low )
    {
        p_battery->state = MU_BATTERY_STATE_LOW;
    }
    else
    {
        p_battery->state = MU_BATTERY_STATE_NORMAL;
    }
}

/* ==================== 对外接口 ==================== */

void mu_battery_feed_adc( mu_battery_t *p_battery, uint16_t adc_raw )
{
    uint16_t voltage = 0;

    if( p_battery == NULL || p_battery->p_params == NULL )
    {
        return;
    }

    voltage = adc_to_voltage( p_battery->p_params, adc_raw );

    p_battery->voltage_raw_mv = voltage;
    p_battery->voltage_mv      = filter_voltage( p_battery, voltage );
    p_battery->percent         = calc_percent( p_battery->p_params,
                                               p_battery->voltage_mv );

    update_state( p_battery );
}

void mu_battery_feed_voltage( mu_battery_t *p_battery, uint16_t voltage_mv )
{
    if( p_battery == NULL || p_battery->p_params == NULL )
    {
        return;
    }

    p_battery->voltage_raw_mv = voltage_mv;
    p_battery->voltage_mv      = filter_voltage( p_battery, voltage_mv );
    p_battery->percent         = calc_percent( p_battery->p_params,
                                               p_battery->voltage_mv );

    update_state( p_battery );
}

uint16_t mu_battery_get_voltage( const mu_battery_t *p_battery )
{
    if( p_battery == NULL )
    {
        return 0;
    }

    return p_battery->voltage_mv;
}

uint8_t mu_battery_get_percent( const mu_battery_t *p_battery )
{
    if( p_battery == NULL )
    {
        return 0;
    }

    return p_battery->percent;
}

mu_battery_state_t mu_battery_get_state( const mu_battery_t *p_battery )
{
    if( p_battery == NULL )
    {
        return MU_BATTERY_STATE_UNKNOWN;
    }

    return p_battery->state;
}

void mu_battery_set_charging( mu_battery_t *p_battery, bool charging )
{
    if( p_battery == NULL )
    {
        return;
    }

    if( charging == true )
    {
        p_battery->state = MU_BATTERY_STATE_CHARGING;

        return;
    }

    /**< 退出充电：先清状态再重新判断 */
    if( p_battery->state == MU_BATTERY_STATE_CHARGING ||
        p_battery->state == MU_BATTERY_STATE_FULL )
    {
        p_battery->state = MU_BATTERY_STATE_UNKNOWN;
        update_state( p_battery );
    }
}
