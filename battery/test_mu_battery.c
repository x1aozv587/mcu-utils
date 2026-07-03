#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "mu_battery.h"

static int g_passed = 0;
static int g_failed = 0;

#define EXPECT_EQ_U32(name, expect, actual)                                      \
    do                                                                          \
    {                                                                           \
        uint32_t e = ( uint32_t )( expect );                                     \
        uint32_t a = ( uint32_t )( actual );                                     \
        printf( "%-42s expect=%lu actual=%lu", name,                            \
                ( unsigned long )e, ( unsigned long )a );                        \
        if( e == a )                                                            \
        {                                                                       \
            printf( " PASS\n" );                                                \
            g_passed++;                                                         \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            printf( " FAIL\n" );                                                \
            g_failed++;                                                         \
        }                                                                       \
    } while( 0 )

#define EXPECT_EQ_STATE(name, expect, actual)                                    \
    EXPECT_EQ_U32(name, ( uint32_t )( expect ), ( uint32_t )( actual ))

#define EXPECT_TRUE(name, expr)                                                  \
    EXPECT_EQ_U32(name, 1, ( expr ) ? 1 : 0 )

static const mu_battery_curve_point_t g_curve_desc[] =
{
    { 4300, 100 },
    { 4000, 75  },
    { 3900, 60  },
    { 3800, 40  },
    { 3300, 0   },
};

static mu_battery_params_t make_linear_params( void )
{
    mu_battery_params_t params =
    {
        .cell_count = 1,
        .voltage_full_mv = 4300,
        .voltage_empty_mv = 3300,
        .voltage_low_mv = 3500,
        .adc_ref_mv = 3300,
        .adc_resolution = 4096,
        .adc_divider_ratio = 200,
        .filter_window = 1,
        .reserve = 0,
        .p_curve = NULL,
        .curve_point_count = 0,
    };

    return params;
}

static mu_battery_params_t make_curve_params( void )
{
    mu_battery_params_t params = make_linear_params();

    params.p_curve = g_curve_desc;
    params.curve_point_count = ( uint8_t )( sizeof( g_curve_desc ) / sizeof( g_curve_desc[0] ) );

    return params;
}

/* ==================== 初始化测试 ==================== */

static void test_init_valid( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    EXPECT_EQ_U32( "init valid", MU_OK, mu_battery_init( &battery, &params ) );
    EXPECT_EQ_STATE( "state after init", MU_BATTERY_STATE_UNKNOWN, mu_battery_get_state( &battery ) );
    EXPECT_EQ_U32( "voltage after init", 0, mu_battery_get_voltage( &battery ) );
    EXPECT_EQ_U32( "percent after init", 0, mu_battery_get_percent( &battery ) );
}

static void test_init_invalid_params( void )
{
    mu_battery_t battery;
    mu_battery_params_t params;

    params = make_linear_params();
    EXPECT_EQ_U32( "init null battery", MU_ERR_PARAM, mu_battery_init( NULL, &params ) );
    EXPECT_EQ_U32( "init null params", MU_ERR_PARAM, mu_battery_init( &battery, NULL ) );

    params = make_linear_params();
    params.cell_count = 0;
    EXPECT_EQ_U32( "invalid cell_count 0", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.cell_count = 5;
    EXPECT_EQ_U32( "invalid cell_count > 4", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.voltage_full_mv = 3300;
    params.voltage_empty_mv = 3300;
    EXPECT_EQ_U32( "invalid full <= empty", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.adc_ref_mv = 0;
    EXPECT_EQ_U32( "invalid adc_ref 0", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.adc_resolution = 0;
    EXPECT_EQ_U32( "invalid adc_resolution 0", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.adc_divider_ratio = 0;
    EXPECT_EQ_U32( "invalid divider 0", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.voltage_low_mv = 3200;
    EXPECT_EQ_U32( "invalid low < empty", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.voltage_low_mv = 4400;
    EXPECT_EQ_U32( "invalid low > full", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.filter_window = 0;
    EXPECT_EQ_U32( "invalid filter 0", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.filter_window = 17;
    EXPECT_EQ_U32( "invalid filter > 16", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.p_curve = g_curve_desc;
    params.curve_point_count = 1;
    EXPECT_EQ_U32( "invalid curve count 1", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params = make_linear_params();
    params.p_curve = NULL;
    params.curve_point_count = 2;
    EXPECT_EQ_U32( "invalid null curve nonzero count", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );
}

/* ==================== 曲线表非法测试 ==================== */

static void test_init_invalid_curve( void )
{
    mu_battery_t battery;
    mu_battery_params_t params;
    mu_battery_curve_point_t bad_voltage_order[] = { {3300,0}, {4300,100} };
    mu_battery_curve_point_t bad_voltage_equal[] = { {4300,100}, {4300,90} };
    mu_battery_curve_point_t bad_percent_order[] = { {4300,80}, {4000,90} };
    mu_battery_curve_point_t bad_percent_over[] = { {4300,120}, {4000,80} };

    params = make_linear_params();
    params.p_curve = bad_voltage_order;
    params.curve_point_count = 2;
    EXPECT_EQ_U32( "bad curve voltage ascending", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params.p_curve = bad_voltage_equal;
    EXPECT_EQ_U32( "bad curve voltage equal", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params.p_curve = bad_percent_order;
    EXPECT_EQ_U32( "bad curve percent ascending", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );

    params.p_curve = bad_percent_over;
    EXPECT_EQ_U32( "bad curve percent > 100", MU_ERR_PARAM, mu_battery_init( &battery, &params ) );
}

/* ==================== 线性百分比测试 ==================== */

static void test_feed_voltage_linear_1s( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 4300 );
    EXPECT_EQ_U32( "linear 1S 4300mV percent", 100, mu_battery_get_percent( &battery ) );
    EXPECT_EQ_STATE( "linear 1S 4300mV state", MU_BATTERY_STATE_FULL, mu_battery_get_state( &battery ) );

    mu_battery_feed_voltage( &battery, 3800 );
    EXPECT_EQ_U32( "linear 1S 3800mV percent", 50, mu_battery_get_percent( &battery ) );
    EXPECT_EQ_STATE( "linear 1S 3800mV state", MU_BATTERY_STATE_NORMAL, mu_battery_get_state( &battery ) );

    mu_battery_feed_voltage( &battery, 3300 );
    EXPECT_EQ_U32( "linear 1S 3300mV percent", 0, mu_battery_get_percent( &battery ) );
    EXPECT_EQ_STATE( "linear 1S 3300mV state", MU_BATTERY_STATE_LOW, mu_battery_get_state( &battery ) );
}

static void test_feed_voltage_linear_multi( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    params.cell_count = 2;
    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 8600 );
    EXPECT_EQ_U32( "linear 2S 8600mV percent", 100, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 7600 );
    EXPECT_EQ_U32( "linear 2S 7600mV percent", 50, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 6600 );
    EXPECT_EQ_U32( "linear 2S 6600mV percent", 0, mu_battery_get_percent( &battery ) );

    params.cell_count = 3;
    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 12900 );
    EXPECT_EQ_U32( "linear 3S 12900mV percent", 100, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 11400 );
    EXPECT_EQ_U32( "linear 3S 11400mV percent", 50, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 9900 );
    EXPECT_EQ_U32( "linear 3S 9900mV percent", 0, mu_battery_get_percent( &battery ) );

    params.cell_count = 4;
    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 17200 );
    EXPECT_EQ_U32( "linear 4S 17200mV percent", 100, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 15200 );
    EXPECT_EQ_U32( "linear 4S 15200mV percent", 50, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 13200 );
    EXPECT_EQ_U32( "linear 4S 13200mV percent", 0, mu_battery_get_percent( &battery ) );
}

/* ==================== 曲线百分比测试 ==================== */

static void test_feed_voltage_curve_1s( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_curve_params();

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 4300 );
    EXPECT_EQ_U32( "curve 4300mV percent", 100, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 4000 );
    EXPECT_EQ_U32( "curve 4000mV percent", 75, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 3850 );
    EXPECT_EQ_U32( "curve 3850mV percent", 50, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 3200 );
    EXPECT_EQ_U32( "curve 3200mV percent", 0, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 4400 );
    EXPECT_EQ_U32( "curve 4400mV percent", 100, mu_battery_get_percent( &battery ) );
}

static void test_feed_voltage_curve_2s( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_curve_params();

    params.cell_count = 2;

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 8600 );
    EXPECT_EQ_U32( "curve 2S 8600mV percent", 100, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 7700 );
    EXPECT_EQ_U32( "curve 2S 7700mV percent", 50, mu_battery_get_percent( &battery ) );

    mu_battery_feed_voltage( &battery, 6600 );
    EXPECT_EQ_U32( "curve 2S 6600mV percent", 0, mu_battery_get_percent( &battery ) );
}

/* ==================== 滤波测试 ==================== */

static void test_filter_average( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    params.filter_window = 4;

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 4000 );
    EXPECT_EQ_U32( "filter sample 1", 4000, mu_battery_get_voltage( &battery ) );

    mu_battery_feed_voltage( &battery, 4100 );
    EXPECT_EQ_U32( "filter sample 2", 4050, mu_battery_get_voltage( &battery ) );

    mu_battery_feed_voltage( &battery, 4200 );
    EXPECT_EQ_U32( "filter sample 3", 4100, mu_battery_get_voltage( &battery ) );

    mu_battery_feed_voltage( &battery, 4300 );
    EXPECT_EQ_U32( "filter sample 4", 4150, mu_battery_get_voltage( &battery ) );

    mu_battery_feed_voltage( &battery, 4300 );
    EXPECT_EQ_U32( "filter sample 5 wrap", 4225, mu_battery_get_voltage( &battery ) );
}

/* ==================== ADC 测试 ==================== */

static void test_adc_to_voltage_via_feed_adc( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    params.adc_ref_mv = 3300;
    params.adc_resolution = 4096;
    params.adc_divider_ratio = 200;
    params.filter_window = 1;

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_adc( &battery, 0 );
    EXPECT_EQ_U32( "adc 0 -> voltage", 0, mu_battery_get_voltage( &battery ) );

    mu_battery_feed_adc( &battery, 2048 );
    EXPECT_EQ_U32( "adc 2048 -> voltage", 3300, mu_battery_get_voltage( &battery ) );

    mu_battery_feed_adc( &battery, 4095 );
    EXPECT_EQ_U32( "adc 4095 -> voltage", 6598, mu_battery_get_voltage( &battery ) );
}

static void test_adc_saturation( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    params.adc_ref_mv = 5000;
    params.adc_resolution = 4096;
    params.adc_divider_ratio = 2000;
    params.filter_window = 1;

    ( void )mu_battery_init( &battery, &params );

    /**< 4095 * 5000 * 2000 / (4096 * 100) = 99975, 饱和到 65535 */
    mu_battery_feed_adc( &battery, 4095 );
    EXPECT_EQ_U32( "adc saturation 65535", 65535, mu_battery_get_voltage( &battery ) );
}

/* ==================== 充电状态测试 ==================== */

static void test_charging_state( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 3800 );
    EXPECT_EQ_STATE( "before charging state", MU_BATTERY_STATE_NORMAL, mu_battery_get_state( &battery ) );

    mu_battery_set_charging( &battery, true );
    EXPECT_EQ_STATE( "set charging true", MU_BATTERY_STATE_CHARGING, mu_battery_get_state( &battery ) );

    mu_battery_set_charging( &battery, false );
    EXPECT_EQ_STATE( "set charging false", MU_BATTERY_STATE_NORMAL, mu_battery_get_state( &battery ) );

    mu_battery_set_charging( &battery, true );
    mu_battery_feed_voltage( &battery, 4300 );
    EXPECT_EQ_STATE( "charging full", MU_BATTERY_STATE_FULL, mu_battery_get_state( &battery ) );

    mu_battery_set_charging( &battery, false );
    EXPECT_EQ_STATE( "exit full charging", MU_BATTERY_STATE_FULL, mu_battery_get_state( &battery ) );
}

/**< 充电中电压波动不应退出充电状态 */
static void test_charging_persistence( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 3600 );
    mu_battery_set_charging( &battery, true );
    EXPECT_EQ_STATE( "charging start", MU_BATTERY_STATE_CHARGING, mu_battery_get_state( &battery ) );

    mu_battery_feed_voltage( &battery, 3800 );
    EXPECT_EQ_STATE( "charging keep after feed", MU_BATTERY_STATE_CHARGING, mu_battery_get_state( &battery ) );

    mu_battery_feed_voltage( &battery, 4300 );
    EXPECT_EQ_STATE( "charging to full", MU_BATTERY_STATE_FULL, mu_battery_get_state( &battery ) );

    /**< 充电中电压回落后仍应保持 FULL 或 CHARGING，不应跳 NORMAL */
    mu_battery_feed_voltage( &battery, 3800 );
    EXPECT_EQ_STATE( "charging full hold after dip", MU_BATTERY_STATE_FULL, mu_battery_get_state( &battery ) );
}

/* ==================== NULL 安全测试 ==================== */

static void test_null_safety( void )
{
    mu_battery_feed_adc( NULL, 1000 );
    mu_battery_feed_voltage( NULL, 4000 );
    mu_battery_set_charging( NULL, true );
    mu_battery_set_charging( NULL, false );

    EXPECT_EQ_U32( "get_voltage NULL", 0, mu_battery_get_voltage( NULL ) );
    EXPECT_EQ_U32( "get_percent NULL", 0, mu_battery_get_percent( NULL ) );
    EXPECT_EQ_STATE( "get_state NULL", MU_BATTERY_STATE_UNKNOWN, mu_battery_get_state( NULL ) );
}

/* ==================== main ==================== */

int main( void )
{
    printf( "\n=== init ===\n" );
    test_init_valid();
    test_init_invalid_params();
    test_init_invalid_curve();

    printf( "\n=== linear percent ===\n" );
    test_feed_voltage_linear_1s();
    test_feed_voltage_linear_multi();

    printf( "\n=== curve percent ===\n" );
    test_feed_voltage_curve_1s();
    test_feed_voltage_curve_2s();

    printf( "\n=== filter ===\n" );
    test_filter_average();

    printf( "\n=== adc ===\n" );
    test_adc_to_voltage_via_feed_adc();
    test_adc_saturation();

    printf( "\n=== charging ===\n" );
    test_charging_state();
    test_charging_persistence();

    printf( "\n=== NULL safety ===\n" );
    test_null_safety();

    printf( "\n========================================\n" );
    printf( "passed: %d failed: %d\n", g_passed, g_failed );
    printf( "RESULT: %s\n", g_failed == 0 ? "PASS" : "FAIL" );
    printf( "========================================\n" );

    return g_failed == 0 ? 0 : 1;
}
