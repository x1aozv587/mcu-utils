#include "mu_battery.h"

#include <string.h>

static uint8_t mu_battery_clamp_percent( uint32_t percent );
static bool mu_battery_is_valid_curve( const mu_battery_curve_point_t *p_curve, uint8_t count );
static bool mu_battery_is_valid_params( const mu_battery_params_t *p_params );
static uint16_t mu_battery_adc_to_voltage( const mu_battery_params_t *p_params, uint16_t adc_raw );
static uint16_t mu_battery_filter_voltage( mu_battery_t *p_battery, uint16_t voltage_mv );
static uint8_t mu_battery_calc_percent_linear_by_cell( const mu_battery_params_t *p_params, uint16_t cell_mv );
static uint8_t mu_battery_calc_percent_curve( const mu_battery_params_t *p_params, uint16_t cell_mv );
static uint8_t mu_battery_calc_percent( const mu_battery_params_t *p_params, uint16_t voltage_mv );
static void mu_battery_update_state( mu_battery_t *p_battery );

/**< 钳位百分比到 0~100 */
static uint8_t mu_battery_clamp_percent( uint32_t percent )
{
    if( percent > 100U )
    {
        return 100;
    }

    return ( uint8_t )percent;
}

/**
 * @brief 校验曲线表合法性
 *
 * 检查电压严格从高到低、百分比不递增、百分比在 0~100 范围内。
 */
static bool mu_battery_is_valid_curve( const mu_battery_curve_point_t *p_curve,
                                       uint8_t count )
{
    uint8_t i = 0;

    if( p_curve == NULL || count < 2U )
    {
        return false;
    }

    for( i = 0; i < ( uint8_t )( count - 1U ); i++ )
    {
        if( p_curve[i].voltage_mv <= p_curve[i + 1U].voltage_mv )
        {
            return false;
        }

        if( p_curve[i].percent < p_curve[i + 1U].percent )
        {
            return false;
        }

        if( p_curve[i].percent > 100U ||
            p_curve[i + 1U].percent > 100U )
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief 校验电池参数合法性
 *
 * 检查电芯数、满/亏电压、ADC 参数、分压比、低电阈值、滤波窗口、
 * 曲线表是否完整和合法。
 */
static bool mu_battery_is_valid_params( const mu_battery_params_t *p_params )
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
        if( mu_battery_is_valid_curve( p_params->p_curve,
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

/**< ADC 原始值转电压（mV），使用 uint64_t 防溢出 */
static uint16_t mu_battery_adc_to_voltage( const mu_battery_params_t *p_params,
                                           uint16_t adc_raw )
{
    uint64_t voltage = 0;

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

/**< 滑动窗口滤波，环形缓冲 + 均值 */
static uint16_t mu_battery_filter_voltage( mu_battery_t *p_battery,
                                           uint16_t voltage_mv )
{
    uint16_t result = 0;
    uint8_t i = 0;
    uint32_t sum = 0;
    uint8_t count = 0;

    p_battery->filter_buf[p_battery->filter_idx] = voltage_mv;
    p_battery->filter_idx = ( p_battery->filter_idx + 1U ) %
                            p_battery->p_params->filter_window;

    if( p_battery->filter_cnt < p_battery->p_params->filter_window )
    {
        p_battery->filter_cnt++;
    }

    count = p_battery->filter_cnt;

    for( i = 0; i < count; i++ )
    {
        sum += p_battery->filter_buf[i];
    }

    result = ( uint16_t )( sum / count );

    return result;
}

/**< 线性电压-百分比（基于单电芯电压，四舍五入） */
static uint8_t mu_battery_calc_percent_linear_by_cell(
    const mu_battery_params_t *p_params,
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

    percent = ( percent + denominator / 2U ) / denominator;

    return mu_battery_clamp_percent( percent );
}

/**< 曲线电压-百分比（分段线性插值，曲线从高到低排列） */
static uint8_t mu_battery_calc_percent_curve( const mu_battery_params_t *p_params,
                                              uint16_t cell_mv )
{
    const mu_battery_curve_point_t *p_curve = p_params->p_curve;
    uint8_t count = p_params->curve_point_count;
    uint8_t i = 0;

    if( p_curve == NULL || count < 2U )
    {
        return mu_battery_calc_percent_linear_by_cell( p_params, cell_mv );
    }

    if( cell_mv >= p_curve[0].voltage_mv )
    {
        return mu_battery_clamp_percent( p_curve[0].percent );
    }

    if( cell_mv <= p_curve[count - 1U].voltage_mv )
    {
        return mu_battery_clamp_percent( p_curve[count - 1U].percent );
    }

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
                return mu_battery_clamp_percent( p_low );
            }

            percent = p_low +
                      ( v_offset * p_range + v_range / 2U ) /
                      v_range;

            return mu_battery_clamp_percent( percent );
        }
    }

    return 0;
}

/**< 整包电压转百分比，自动除以电芯数 */
static uint8_t mu_battery_calc_percent( const mu_battery_params_t *p_params,
                                        uint16_t voltage_mv )
{
    uint16_t cell_mv = 0;

    if( p_params == NULL || p_params->cell_count == 0U )
    {
        return 0;
    }

    cell_mv = ( uint16_t )( ( ( uint32_t )voltage_mv +
                              p_params->cell_count / 2U ) /
                            p_params->cell_count );

    if( p_params->p_curve != NULL && p_params->curve_point_count >= 2U )
    {
        return mu_battery_calc_percent_curve( p_params, cell_mv );
    }

    return mu_battery_calc_percent_linear_by_cell( p_params, cell_mv );
}

/**
 * @brief 根据当前电压和充电状态更新电池状态
 *
 * 充电中（is_charging=true）：仅判断是否满电。
 * 非充电：根据百分比和低电阈值判断 NORMAL/LOW/FULL。
 */
static void mu_battery_update_state( mu_battery_t *p_battery )
{
    uint32_t total_low = 0;

    total_low = ( uint32_t )p_battery->p_params->voltage_low_mv *
                p_battery->p_params->cell_count;

    if( p_battery->voltage_mv == 0U )
    {
        return;
    }

    if( p_battery->is_charging == true )
    {
        if( p_battery->percent >= 100U )
        {
            p_battery->state = MU_BATTERY_STATE_FULL;
        }

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

mu_status_t mu_battery_init( mu_battery_t *p_battery,
                             const mu_battery_params_t *p_params )
{
    if( p_battery == NULL || mu_battery_is_valid_params( p_params ) == false )
    {
        return MU_ERR_PARAM;
    }

    memset( p_battery, 0, sizeof( mu_battery_t ) );

    p_battery->p_params = p_params;
    p_battery->state    = MU_BATTERY_STATE_UNKNOWN;

    return MU_OK;
}

void mu_battery_feed_adc( mu_battery_t *p_battery, uint16_t adc_raw )
{
    uint16_t voltage = 0;

    if( p_battery == NULL || p_battery->p_params == NULL )
    {
        return;
    }

    voltage = mu_battery_adc_to_voltage( p_battery->p_params, adc_raw );

    p_battery->voltage_raw_mv = voltage;
    p_battery->voltage_mv      = mu_battery_filter_voltage( p_battery, voltage );
    p_battery->percent         = mu_battery_calc_percent( p_battery->p_params,
                                                          p_battery->voltage_mv );

    mu_battery_update_state( p_battery );
}

void mu_battery_feed_voltage( mu_battery_t *p_battery, uint16_t voltage_mv )
{
    if( p_battery == NULL || p_battery->p_params == NULL )
    {
        return;
    }

    p_battery->voltage_raw_mv = voltage_mv;
    p_battery->voltage_mv      = mu_battery_filter_voltage( p_battery, voltage_mv );
    p_battery->percent         = mu_battery_calc_percent( p_battery->p_params,
                                                          p_battery->voltage_mv );

    mu_battery_update_state( p_battery );
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

    p_battery->is_charging = charging;

    if( charging == true )
    {
        p_battery->state = MU_BATTERY_STATE_CHARGING;

        return;
    }

    if( p_battery->state == MU_BATTERY_STATE_CHARGING ||
        p_battery->state == MU_BATTERY_STATE_FULL )
    {
        mu_battery_update_state( p_battery );
    }
}
