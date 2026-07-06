#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "mu_datetime.h"

static int g_passed = 0;
static int g_failed = 0;

#define RUN( func, expect, actual ) \
    do { \
        printf( "  %-44s  expect=%d  actual=%d", \
                #func, (int)(expect), (int)(actual) ); \
        if( (actual) == (expect) ) { printf( "  PASS\n" ); g_passed++; } \
        else { printf( "  FAIL\n" ); g_failed++; } \
    } while(0)

#define RUN_U32( func, expect, actual ) \
    do { \
        printf( "  %-44s  expect=%lu  actual=%lu", \
                #func, (unsigned long)(expect), (unsigned long)(actual) ); \
        if( (actual) == (expect) ) { printf( "  PASS\n" ); g_passed++; } \
        else { printf( "  FAIL\n" ); g_failed++; } \
    } while(0)

static void t_leap_true( void )
{
    RUN( t_leap_true, 1, mu_date_is_leap_year( 2000 ) );
}

static void t_leap_true2( void )
{
    RUN( t_leap_true2, 1, mu_date_is_leap_year( 2004 ) );
}

static void t_leap_false( void )
{
    RUN( t_leap_false, 0, mu_date_is_leap_year( 1900 ) );
}

static void t_leap_false2( void )
{
    RUN( t_leap_false2, 0, mu_date_is_leap_year( 2100 ) );
}

static void t_leap_false3( void )
{
    RUN( t_leap_false3, 0, mu_date_is_leap_year( 2023 ) );
}

static void t_dim_feb_leap( void )
{
    RUN( t_dim_feb_leap, 29, mu_date_days_in_month( 2000, 2 ) );
}

static void t_dim_feb_common( void )
{
    RUN( t_dim_feb_common, 28, mu_date_days_in_month( 2023, 2 ) );
}

static void t_dim_jan( void )
{
    RUN( t_dim_jan, 31, mu_date_days_in_month( 2023, 1 ) );
}

static void t_dim_apr( void )
{
    RUN( t_dim_apr, 30, mu_date_days_in_month( 2023, 4 ) );
}

static void t_dim_bad( void )
{
    RUN( t_dim_bad, 0, mu_date_days_in_month( 2023, 13 ) );
}

static void t_weekday( void )
{
    mu_date_t d = { 2026, 1, 1 };

    RUN( t_weekday, MU_WEEKDAY_THURSDAY, mu_date_weekday( &d ) );
}

static void t_weekday2( void )
{
    mu_date_t d = { 2000, 1, 1 };

    RUN( t_weekday2, MU_WEEKDAY_SATURDAY, mu_date_weekday( &d ) );
}

static void t_doy1( void )
{
    mu_date_t d = { 2026, 1, 1 };

    RUN( t_doy1, 1, mu_date_day_of_year( &d ) );
}

static void t_doy365( void )
{
    mu_date_t d = { 2023, 12, 31 };

    RUN( t_doy365, 365, mu_date_day_of_year( &d ) );
}

static void t_doy366( void )
{
    mu_date_t d = { 2000, 12, 31 };

    RUN( t_doy366, 366, mu_date_day_of_year( &d ) );
}

static void t_date_valid( void )
{
    mu_date_t d = { 2026, 7, 3 };

    RUN( t_date_valid, 1, mu_date_is_valid( &d ) );
}

static void t_date_bad_m( void )
{
    mu_date_t d = { 2026, 13, 1 };

    RUN( t_date_bad_m, 0, mu_date_is_valid( &d ) );
}

static void t_date_bad_m0( void )
{
    mu_date_t d = { 2026, 0, 1 };

    RUN( t_date_bad_m0, 0, mu_date_is_valid( &d ) );
}

static void t_date_bad_d( void )
{
    mu_date_t d = { 2026, 2, 30 };

    RUN( t_date_bad_d, 0, mu_date_is_valid( &d ) );
}

static void t_date_bad_d0( void )
{
    mu_date_t d = { 2026, 1, 0 };

    RUN( t_date_bad_d0, 0, mu_date_is_valid( &d ) );
}

static void t_date_bad_feb29( void )
{
    mu_date_t d = { 2023, 2, 29 };

    RUN( t_date_bad_feb29, 0, mu_date_is_valid( &d ) );
}

static void t_date_bad_feb29_ok( void )
{
    mu_date_t d = { 2024, 2, 29 };

    RUN( t_date_bad_feb29_ok, 1, mu_date_is_valid( &d ) );
}

static void t_date_bad_apr31( void )
{
    mu_date_t d = { 2023, 4, 31 };

    RUN( t_date_bad_apr31, 0, mu_date_is_valid( &d ) );
}

static void t_date_null( void )
{
    RUN( t_date_null, 0, mu_date_is_valid( NULL ) );
}

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970

static void t_date_1969( void )
{
    mu_date_t d = { 1969, 12, 31 };

    RUN( t_date_1969, 0, mu_date_is_valid( &d ) );
}

static void t_date_1970( void )
{
    mu_date_t d = { 1970, 1, 1 };

    RUN( t_date_1970, 1, mu_date_is_valid( &d ) );
}

#else

static void t_date_1999( void )
{
    mu_date_t d = { 1999, 12, 31 };

    RUN( t_date_1999, 0, mu_date_is_valid( &d ) );
}

static void t_date_2000( void )
{
    mu_date_t d = { 2000, 1, 1 };

    RUN( t_date_2000, 1, mu_date_is_valid( &d ) );
}

#endif

static void t_date_2100( void )
{
    mu_date_t d = { 2100, 1, 1 };

    RUN( t_date_2100, 0, mu_date_is_valid( &d ) );
}

static void t_time_valid( void )
{
    mu_time_t t = { 12, 30, 45 };

    RUN( t_time_valid, 1, mu_time_is_valid( &t ) );
}

static void t_time_bad_h( void )
{
    mu_time_t t = { 24, 0, 0 };

    RUN( t_time_bad_h, 0, mu_time_is_valid( &t ) );
}

static void t_time_bad_m( void )
{
    mu_time_t t = { 0, 60, 0 };

    RUN( t_time_bad_m, 0, mu_time_is_valid( &t ) );
}

static void t_time_bad_s( void )
{
    mu_time_t t = { 0, 0, 60 };

    RUN( t_time_bad_s, 0, mu_time_is_valid( &t ) );
}

static void t_time_null( void )
{
    RUN( t_time_null, 0, mu_time_is_valid( NULL ) );
}

static void t_time_ok_0( void )
{
    mu_time_t t = { 0, 0, 0 };

    RUN( t_time_ok_0, 1, mu_time_is_valid( &t ) );
}

static void t_time_ok_max( void )
{
    mu_time_t t = { 23, 59, 59 };

    RUN( t_time_ok_max, 1, mu_time_is_valid( &t ) );
}

static void t_dt_null( void )
{
    RUN( t_dt_null, 0, mu_datetime_is_valid( NULL ) );
}

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970

static void t_ts_1970( void )
{
    mu_datetime_t a = { { 1970, 1, 1 }, { 0, 0, 0 } };
    uint32_t ts = 0;
    bool ok = false;

    ok = mu_datetime_to_timestamp_ex( &a, &ts );
    RUN( t_ts_1970_ok, 1, ok );
    RUN_U32( t_ts_1970_v, 0, ts );
}

static void t_ts_1999_end( void )
{
    mu_datetime_t a = { { 1999, 12, 31 }, { 23, 59, 59 } };
    uint32_t ts = 0;
    bool ok = false;

    ok = mu_datetime_to_timestamp_ex( &a, &ts );
    RUN( t_ts_1999_end_ok, 1, ok );
    RUN_U32( t_ts_1999_end_v, 946684799UL, ts );
}

static void t_ts_2000_begin( void )
{
    mu_datetime_t a = { { 2000, 1, 1 }, { 0, 0, 0 } };
    uint32_t ts = 0;
    bool ok = false;

    ok = mu_datetime_to_timestamp_ex( &a, &ts );
    RUN( t_ts_2000_begin_ok, 1, ok );
    RUN_U32( t_ts_2000_begin_v, 946684800UL, ts );
}

#else

static void t_ts_2000( void )
{
    mu_datetime_t a = { { 2000, 1, 1 }, { 0, 0, 0 } };
    uint32_t ts = 0;
    bool ok = false;

    ok = mu_datetime_to_timestamp_ex( &a, &ts );
    RUN( t_ts_2000_ok, 1, ok );
    RUN_U32( t_ts_2000_v, 0, ts );
}

#endif

static void t_ts_null( void )
{
    RUN( t_ts_null, 0, mu_datetime_to_timestamp_ex( NULL, NULL ) );
}

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970

static void t_from_ts_0( void )
{
    mu_datetime_t dt;
    bool ok = false;

    ok = mu_timestamp_to_datetime( 0, &dt );
    RUN( t_from_ts_0_ok, 1, ok );
    RUN( t_from_ts_0_y, 1970, dt.date.year );
    RUN( t_from_ts_0_m, 1, dt.date.month );
    RUN( t_from_ts_0_d, 1, dt.date.day );
}

static void t_from_ts_1999_end( void )
{
    mu_datetime_t dt;
    bool ok = false;

    ok = mu_timestamp_to_datetime( 946684799UL, &dt );
    RUN( t_from_ts_1999_ok, 1, ok );
    RUN( t_from_ts_1999_y, 1999, dt.date.year );
}

#endif

static void t_from_ts_2000( void )
{
    mu_datetime_t dt;
    bool ok = false;

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    ok = mu_timestamp_to_datetime( 946684800UL, &dt );
#else
    ok = mu_timestamp_to_datetime( 0, &dt );
#endif
    RUN( t_from_ts_2000_ok, 1, ok );
    RUN( t_from_ts_2000_y, 2000, dt.date.year );
    RUN( t_from_ts_2000_m, 1, dt.date.month );
    RUN( t_from_ts_2000_d, 1, dt.date.day );
}

static void t_from_ts_null( void )
{
    RUN( t_from_ts_null, 0, mu_timestamp_to_datetime( 0, NULL ) );
}

static void t_rt_1( void )
{
    mu_datetime_t a = { { 2026, 7, 3 }, { 14, 30, 0 } };
    mu_datetime_t b;
    uint32_t ts = 0;

    mu_datetime_to_timestamp_ex( &a, &ts );
    mu_timestamp_to_datetime( ts, &b );
    RUN( t_rt_y, 2026, b.date.year );
    RUN( t_rt_m, 7, b.date.month );
    RUN( t_rt_d, 3, b.date.day );
    RUN( t_rt_h, 14, b.time.hour );
}

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970

static void t_rt_1970( void )
{
    mu_datetime_t a;
    mu_datetime_t b;
    uint32_t ts = 0;

    a.date.year  = 1970;
    a.date.month = 1;
    a.date.day   = 1;
    a.time.hour  = 0;
    a.time.minute = 0;
    a.time.second = 0;

    mu_datetime_to_timestamp_ex( &a, &ts );
    mu_timestamp_to_datetime( ts, &b );
    RUN( t_rt1970_y, 1970, b.date.year );
    RUN( t_rt1970_m, 1, b.date.month );
    RUN( t_rt1970_d, 1, b.date.day );
}

#endif

static void t_add_1day( void )
{
    mu_date_t d = { 2026, 1, 31 };
    bool ok = false;

    ok = mu_date_add_days( &d, 1 );
    RUN( t_add_1d_ok, 1, ok );
    RUN( t_add_1d_m, 2, d.month );
    RUN( t_add_1d_d, 1, d.day );
}

static void t_add_365day( void )
{
    mu_date_t d = { 2023, 1, 1 };
    bool ok = false;

    ok = mu_date_add_days( &d, 365 );
    RUN( t_add_365_ok, 1, ok );
    RUN( t_add_365_y, 2024, d.year );
}

static void t_sub_1day( void )
{
    mu_date_t d = { 2026, 1, 1 };
    bool ok = false;

    ok = mu_date_add_days( &d, -1 );
    RUN( t_sub_1d_ok, 1, ok );
    RUN( t_sub_1d_y, 2025, d.year );
    RUN( t_sub_1d_m, 12, d.month );
    RUN( t_sub_1d_d, 31, d.day );
}

static void t_add_leap( void )
{
    mu_date_t d = { 2024, 2, 28 };
    bool ok = false;

    ok = mu_date_add_days( &d, 1 );
    RUN( t_add_leap_ok, 1, ok );
    RUN( t_add_leap_d, 29, d.day );
}

static void t_add_leap2( void )
{
    mu_date_t d = { 2024, 2, 29 };
    bool ok = false;

    ok = mu_date_add_days( &d, 1 );
    RUN( t_add_leap2_ok, 1, ok );
    RUN( t_add_leap2_m, 3, d.month );
}

static void t_add_common( void )
{
    mu_date_t d = { 2023, 2, 28 };
    bool ok = false;

    ok = mu_date_add_days( &d, 1 );
    RUN( t_add_com_ok, 1, ok );
    RUN( t_add_com_m, 3, d.month );
}

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970

static void t_add_1999_to_2000( void )
{
    mu_date_t d = { 1999, 12, 31 };
    bool ok = false;

    ok = mu_date_add_days( &d, 1 );
    RUN( t_add_1999_ok, 1, ok );
    RUN( t_add_1999_y, 2000, d.year );
}

static void t_sub_2000_to_1999( void )
{
    mu_date_t d = { 2000, 1, 1 };
    bool ok = false;

    ok = mu_date_add_days( &d, -1 );
    RUN( t_sub_2000_ok, 1, ok );
    RUN( t_sub_2000_y, 1999, d.year );
}

#else

static void t_sub_2000_fail( void )
{
    mu_date_t d = { 2000, 1, 1 };
    bool ok = false;

    ok = mu_date_add_days( &d, -1 );
    RUN( t_sub_2000_fail, 0, ok );
}

#endif

static void t_diff( void )
{
    mu_date_t a = { 2026, 1, 10 };
    mu_date_t b = { 2026, 1, 1 };

    RUN( t_diff, 9, mu_date_diff_days( &a, &b ) );
}

static void t_diff_neg( void )
{
    mu_date_t a = { 2026, 1, 1 };
    mu_date_t b = { 2026, 1, 10 };

    RUN( t_diff_neg, -9, mu_date_diff_days( &a, &b ) );
}

static void t_time_add_cross( void )
{
    mu_time_t t = { 23, 59, 59 };
    bool ok = false;

    ok = mu_time_add_seconds( &t, 1 );
    RUN( t_time_add_cross, 0, ok );
}

static void t_time_add_ok( void )
{
    mu_time_t t = { 12, 30, 0 };
    bool ok = false;

    ok = mu_time_add_seconds( &t, 60 );
    RUN( t_time_add_ok_r, 1, ok );
    RUN( t_time_add_ok_m, 31, t.minute );
}

static void t_time_add_1s( void )
{
    mu_time_t t = { 23, 59, 58 };
    bool ok = false;

    ok = mu_time_add_seconds( &t, 1 );
    RUN( t_time_58_ok, 1, ok );
    RUN( t_time_58_s, 59, t.second );
}

static void t_time_sub_1s( void )
{
    mu_time_t t = { 0, 0, 1 };
    bool ok = false;

    ok = mu_time_add_seconds( &t, -1 );
    RUN( t_time_sub1_ok, 1, ok );
    RUN( t_time_sub1_s, 0, t.second );
}

static void t_time_sub_0( void )
{
    mu_time_t t = { 0, 0, 0 };
    bool ok = false;

    ok = mu_time_add_seconds( &t, -1 );
    RUN( t_time_sub0, 0, ok );
}

static void t_cmp_lt( void )
{
    mu_date_t a = { 2025, 1, 1 };
    mu_date_t b = { 2026, 1, 1 };

    RUN( t_cmp_lt, -1, mu_date_compare( &a, &b ) );
}

static void t_cmp_gt( void )
{
    mu_date_t a = { 2026, 1, 1 };
    mu_date_t b = { 2025, 1, 1 };

    RUN( t_cmp_gt, 1, mu_date_compare( &a, &b ) );
}

static void t_cmp_eq( void )
{
    mu_date_t a = { 2026, 7, 3 };
    mu_date_t b = { 2026, 7, 3 };

    RUN( t_cmp_eq, 0, mu_date_compare( &a, &b ) );
}

static void test_leap_year( void )
{
    t_leap_true();
    t_leap_true2();
    t_leap_false();
    t_leap_false2();
    t_leap_false3();
}

static void test_days_in_month( void )
{
    t_dim_feb_leap();
    t_dim_feb_common();
    t_dim_jan();
    t_dim_apr();
    t_dim_bad();
}

static void test_weekday( void )
{
    t_weekday();
    t_weekday2();
}

static void test_day_of_year( void )
{
    t_doy1();
    t_doy365();
    t_doy366();
}

static void test_validate( void )
{
    t_date_valid();
    t_date_bad_m();
    t_date_bad_m0();
    t_date_bad_d();
    t_date_bad_d0();
    t_date_bad_feb29();
    t_date_bad_feb29_ok();
    t_date_bad_apr31();
    t_date_null();
#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    t_date_1969();
    t_date_1970();
#else
    t_date_1999();
    t_date_2000();
#endif
    t_date_2100();
    t_time_valid();
    t_time_bad_h();
    t_time_bad_m();
    t_time_bad_s();
    t_time_null();
    t_time_ok_0();
    t_time_ok_max();
    t_dt_null();
}

static void test_timestamp_to( void )
{
#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    t_ts_1970();
    t_ts_1999_end();
    t_ts_2000_begin();
#else
    t_ts_2000();
#endif
    t_ts_null();
}

static void test_timestamp_from( void )
{
#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    t_from_ts_0();
    t_from_ts_1999_end();
#endif
    t_from_ts_2000();
    t_from_ts_null();
}

static void test_roundtrip( void )
{
    t_rt_1();
#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    t_rt_1970();
#endif
}

static void test_date_add( void )
{
    t_add_1day();
    t_add_365day();
    t_sub_1day();
    t_add_leap();
    t_add_leap2();
    t_add_common();
#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    t_add_1999_to_2000();
    t_sub_2000_to_1999();
#else
    t_sub_2000_fail();
#endif
}

static void test_diff( void )
{
    t_diff();
    t_diff_neg();
}

static void test_time_add( void )
{
    t_time_add_cross();
    t_time_add_ok();
    t_time_add_1s();
    t_time_sub_1s();
    t_time_sub_0();
}

static void test_compare( void )
{
    t_cmp_lt();
    t_cmp_gt();
    t_cmp_eq();
}

int main( int argc, char *argv[] )
{
    if( argc >= 2 && strcmp( argv[1], "list" ) == 0 )
    {
        printf( "Usage: %s\n", argv[0] );

        return 0;
    }

    printf( "\n=== leap year ===\n" );
    test_leap_year();

    printf( "\n=== days in month ===\n" );
    test_days_in_month();

    printf( "\n=== weekday ===\n" );
    test_weekday();

    printf( "\n=== day of year ===\n" );
    test_day_of_year();

    printf( "\n=== validate ===\n" );
    test_validate();

    printf( "\n=== timestamp to ===\n" );
    test_timestamp_to();

    printf( "\n=== timestamp from ===\n" );
    test_timestamp_from();

    printf( "\n=== roundtrip ===\n" );
    test_roundtrip();

    printf( "\n=== date add ===\n" );
    test_date_add();

    printf( "\n=== diff ===\n" );
    test_diff();

    printf( "\n=== time add ===\n" );
    test_time_add();

    printf( "\n=== compare ===\n" );
    test_compare();

    printf( "\n========================================\n" );
    printf( "  passed: %d  failed: %d\n", g_passed, g_failed );
    printf( "  RESULT: %s\n", g_failed > 0 ? "FAIL" : "PASS (all)" );
    printf( "========================================\n" );

    return g_failed > 0 ? 1 : 0;
}
