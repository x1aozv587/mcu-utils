#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "..\mu_common.h"
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

static void test_feed_voltage_linear_1s( void )
{
    mu_battery_t battery;
    mu_battery_params_t params = make_linear_params();

    ( void )mu_battery_init( &battery, &params );

    mu_battery_feed_voltage( &battery, 4300 );
    EXPECT_EQ_U32( "linear 4300mV percent", 100, mu_battery_get_percent( &battery ) );
    EXPECT_EQ_STATE( "linear 4300mV state", MU_BATTERY_STATE_FULL, mu_battery_get_state( &battery ) );

    mu_battery_feed_voltage( &battery, 3800 );
    EXPECT_EQ_U32( "linear 3800mV percent", 50, mu_battery_get_percent( &battery ) );
    EXPECT_EQ_STATE( "linear 3800mV state", MU_BATTERY_STATE_NORMAL, mu_battery_get_state( &battery ) );

    mu_battery_feed_voltage( &battery, 3300 );
    EXPECT_EQ_U32( "linear 3300mV percent", 0, mu_battery_get_percent( &battery ) );
    EXPECT_EQ_STATE( "linear 3300mV state", MU_BATTERY_STATE_LOW, mu_battery_get_state( &battery ) );
}

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

int main( void )
{
    printf( "\n=== init ===\n" );
    test_init_valid();
    test_init_invalid_params();

    printf( "\n=== linear percent ===\n" );
    test_feed_voltage_linear_1s();

    printf( "\n=== curve percent ===\n" );
    test_feed_voltage_curve_1s();
    test_feed_voltage_curve_2s();

    printf( "\n=== filter ===\n" );
    test_filter_average();

    printf( "\n=== adc ===\n" );
    test_adc_to_voltage_via_feed_adc();

    printf( "\n=== charging ===\n" );
    test_charging_state();

    printf( "\n========================================\n" );
    printf( "passed: %d failed: %d\n", g_passed, g_failed );
    printf( "RESULT: %s\n", g_failed == 0 ? "PASS" : "FAIL" );
    printf( "========================================\n" );

    return g_failed == 0 ? 0 : 1;
}
