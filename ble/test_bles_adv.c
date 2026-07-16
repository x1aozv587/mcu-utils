#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bles_adv.h"
#include "bles_adv_ibeacon.h"
#include "bles_filter_name.h"
#include "bles_filter_uuid.h"
#include "bles_filter_manufacturer.h"

static int g_pass = 0;
static int g_fail = 0;

#define RUN( name, expect, actual ) \
    do { \
        int e = (int)(expect); int a = (int)(actual); \
        printf( "%-42s expect=%d actual=%d %s\n", name, e, a, e==a?"PASS":"FAIL" ); \
        if( e == a ) g_pass++; else g_fail++; \
    } while(0)

#define RUN_MEM( name, expect, actual, len ) \
    do { \
        bool ok = memcmp( expect, actual, len ) == 0; \
        printf( "%-42s %s\n", name, ok ? "PASS" : "FAIL" ); \
        if( ok ) { g_pass++; } else { \
            g_fail++; \
            printf( "  expect:" ); \
            for( uint16_t i = 0; i < (len); i++ ) printf( " %02X", ((const uint8_t*)expect)[i] ); \
            printf( "\n  actual:" ); \
            for( uint16_t i = 0; i < (len); i++ ) printf( " %02X", ((const uint8_t*)actual)[i] ); \
            printf( "\n" ); \
        } \
    } while(0)

/* ==================== builder ==================== */

static void test_builder_init_ok( void )
{
    uint8_t buf[31];
    bles_adv_builder_t b;

    RUN( "init ok", MU_OK, bles_adv_builder_init( &b, buf, sizeof( buf ) ) );
    RUN( "init pos", 0, bles_adv_get_len( &b ) );
}

static void test_builder_init_param( void )
{
    uint8_t buf[31];
    bles_adv_builder_t b;

    RUN( "init null builder", MU_ERR_PARAM, bles_adv_builder_init( NULL, buf, sizeof(buf) ) );
    RUN( "init null buffer", MU_ERR_PARAM, bles_adv_builder_init( &b, NULL, sizeof(buf) ) );
    RUN( "init buf_size 0", MU_ERR_PARAM, bles_adv_builder_init( &b, buf, 0 ) );
}

static void test_append_flags( void )
{
    uint8_t buf[31];
    uint8_t expect[] = { 0x02, 0x01, 0x06 };
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "flags ret", true, bles_adv_append_flags( &b, BLES_ADV_FLAG_LE_GENERAL | BLES_ADV_FLAG_BR_EDR_NOT_SUP ) );
    RUN( "flags len", 3, bles_adv_get_len( &b ) );
    RUN_MEM( "flags bytes", expect, buf, sizeof( expect ) );
}

static void test_append_name_complete( void )
{
    uint8_t buf[31];
    uint8_t expect[] = { 0x05, 0x09, 'T', 'E', 'S', 'T' };
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "name complete ret", true, bles_adv_append_name( &b, "TEST", true ) );
    RUN( "name complete len", 6, bles_adv_get_len( &b ) );
    RUN_MEM( "name complete bytes", expect, buf, sizeof( expect ) );
}

static void test_append_name_short( void )
{
    uint8_t buf[31];
    uint8_t expect[] = { 0x05, 0x08, 'T', 'E', 'S', 'T' };
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "name short ret", true, bles_adv_append_name( &b, "TEST", false ) );
    RUN_MEM( "name short bytes", expect, buf, sizeof( expect ) );
}

static void test_append_name_null( void )
{
    uint8_t buf[31];
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "name null", false, bles_adv_append_name( &b, NULL, true ) );
}

static void test_append_name_empty( void )
{
    uint8_t buf[31];
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "name empty", false, bles_adv_append_name( &b, "", true ) );
}

static void test_append_uuid16( void )
{
    uint8_t buf[31];
    uint8_t expect[] = { 0x03, 0x03, 0x0D, 0x18 };
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "uuid16 ret", true, bles_adv_append_uuid16( &b, 0x180D, true ) );
    RUN_MEM( "uuid16 bytes", expect, buf, sizeof( expect ) );
}

static void test_append_tx_power( void )
{
    uint8_t buf[31];
    uint8_t expect[] = { 0x02, 0x0A, 0xC5 };
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "tx ret", true, bles_adv_append_tx_power( &b, -59 ) );
    RUN_MEM( "tx bytes", expect, buf, sizeof( expect ) );
}

static void test_append_no_room( void )
{
    uint8_t buf[3];
    uint8_t data[] = { 0x01, 0x02 };
    bles_adv_builder_t b;

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "no room ret", false, bles_adv_append( &b, BLES_AD_TYPE_MANUFACTURER, data, sizeof(data) ) );
    RUN( "no room len", 0, bles_adv_get_len( &b ) );
}

static void test_append_255( void )
{
    uint8_t buf[300];
    uint8_t data[255];
    bles_adv_builder_t b;

    memset( data, 0xAA, sizeof( data ) );
    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "append 255", false, bles_adv_append( &b, BLES_AD_TYPE_MANUFACTURER, data, 255 ) );
}

static void test_append_254( void )
{
    uint8_t buf[300];
    uint8_t data[254];
    bles_adv_builder_t b;

    memset( data, 0xBB, sizeof( data ) );
    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "append 254", true, bles_adv_append( &b, BLES_AD_TYPE_MANUFACTURER, data, 254 ) );
    RUN( "append 254 len", 256, bles_adv_get_len( &b ) );
}

/* ==================== uuid filter ==================== */

static void test_filter_uuid16( void )
{
    uint8_t adv[] = { 0x05, 0x03, 0x0D, 0x18, 0x0A, 0x18 };
    uint8_t u1[] = { 0x0D, 0x18 };
    uint8_t u2[] = { 0x0A, 0x18 };
    uint8_t u3[] = { 0xFF, 0xFF };

    RUN( "uuid16 ok", true, bles_filter_uuid_match( adv, sizeof(adv), u1, 2 ) );
    RUN( "uuid16 multi", true, bles_filter_uuid_match( adv, sizeof(adv), u2, 2 ) );
    RUN( "uuid16 fail", false, bles_filter_uuid_match( adv, sizeof(adv), u3, 2 ) );
}

static void test_filter_uuid_incomplete( void )
{
    uint8_t adv[] = { 0x05, 0x02, 0x0D, 0x18, 0xEE, 0xFF };
    uint8_t u[] = { 0xEE, 0xFF };

    RUN( "uuid16 incomplete", true, bles_filter_uuid_match( adv, sizeof(adv), u, 2 ) );
}

static void test_filter_uuid_null( void )
{
    uint8_t adv[] = { 0x05, 0x03, 0x0D, 0x18, 0x0A, 0x18 };

    RUN( "uuid null", false, bles_filter_uuid_match( adv, sizeof(adv), NULL, 2 ) );
}

static void test_filter_uuid_bad_len( void )
{
    uint8_t adv[] = { 0x05, 0x03, 0x0D, 0x18, 0x0A, 0x18 };
    uint8_t u[4] = { 0 };

    RUN( "uuid bad len 4", false, bles_filter_uuid_match( adv, sizeof(adv), u, 4 ) );
}

/* ---------- uuid128 ---------- */

static void test_filter_uuid128_complete( void )
{
    uint8_t uuid[] =
    {
        0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C, 0x0D,0x0E,0x0F,0x10,
    };
    uint8_t adv[] =
    {
        0x11, 0x07,
        0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C, 0x0D,0x0E,0x0F,0x10,
    };

    RUN( "uuid128 complete ok", true, bles_filter_uuid_match( adv, sizeof(adv), uuid, 16 ) );
}

static void test_filter_uuid128_incomplete( void )
{
    uint8_t uuid[] =
    {
        0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C, 0x0D,0x0E,0x0F,0x10,
    };
    uint8_t adv[] =
    {
        0x11, 0x06,
        0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C, 0x0D,0x0E,0x0F,0x10,
    };

    RUN( "uuid128 incomplete ok", true, bles_filter_uuid_match( adv, sizeof(adv), uuid, 16 ) );
}

static void test_filter_uuid128_miss( void )
{
    uint8_t uuid[] =
    {
        0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
    };
    uint8_t adv[] =
    {
        0x11, 0x07,
        0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C, 0x0D,0x0E,0x0F,0x10,
    };

    RUN( "uuid128 miss", false, bles_filter_uuid_match( adv, sizeof(adv), uuid, 16 ) );
}

static void test_filter_uuid128_multi( void )
{
    uint8_t uuid_a[] =
    {
        0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C, 0x0D,0x0E,0x0F,0x10,
    };
    uint8_t uuid_b[] =
    {
        0xAA,0xBB,0xCC,0xDD, 0xEE,0xFF,0x00,0x11,
        0x22,0x33,0x44,0x55, 0x66,0x77,0x88,0x99,
    };
    uint8_t adv[] =
    {
        0x21, 0x07,
        0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C, 0x0D,0x0E,0x0F,0x10,
        0xAA,0xBB,0xCC,0xDD, 0xEE,0xFF,0x00,0x11,
        0x22,0x33,0x44,0x55, 0x66,0x77,0x88,0x99,
    };

    RUN( "uuid128 multi 1st", true, bles_filter_uuid_match( adv, sizeof(adv), uuid_a, 16 ) );
    RUN( "uuid128 multi 2nd", true, bles_filter_uuid_match( adv, sizeof(adv), uuid_b, 16 ) );
}

/* ==================== filter core ==================== */

static void test_filter_find_exist( void )
{
    uint8_t adv[] = { 0x02, 0x01, 0x06, 0x05, 0x09, 'T', 'E', 'S', 'T' };

    RUN( "find flags ok", true,
         bles_filter_find( adv, sizeof(adv), BLES_AD_TYPE_FLAGS, NULL, NULL ) );
    RUN( "find name ok", true,
         bles_filter_find( adv, sizeof(adv), BLES_AD_TYPE_COMPLETE_NAME, NULL, NULL ) );
    RUN( "find no type", false,
         bles_filter_find( adv, sizeof(adv), BLES_AD_TYPE_TX_POWER, NULL, NULL ) );
}

static void test_filter_malformed_trunc( void )
{
    uint8_t adv[] = { 0x05, 0x09, 'A' };

    RUN( "malform trunc", false,
         bles_filter_find( adv, sizeof(adv), BLES_AD_TYPE_COMPLETE_NAME, NULL, NULL ) );
}

static void test_filter_malformed_zero_len( void )
{
    uint8_t adv[] = { 0x02, 0x01, 0x06, 0x00, 0xFF, 0xFF };

    RUN( "malform zero len flags ok", true,
         bles_filter_find( adv, sizeof(adv), BLES_AD_TYPE_FLAGS, NULL, NULL ) );
    RUN( "malform zero len mfr fail", false,
         bles_filter_find( adv, sizeof(adv), BLES_AD_TYPE_MANUFACTURER, NULL, NULL ) );
}

/* ==================== name filter ==================== */

static void test_filter_name_exact( void )
{
    uint8_t adv[] = { 0x06, 0x09, 'M', 'y', 'D', 'e', 'v' };

    RUN( "name exact ok", true, bles_filter_name_match( adv, sizeof(adv), "MyDev", BLES_FILTER_NAME_EXACT ) );
    RUN( "name exact fail", false, bles_filter_name_match( adv, sizeof(adv), "MyDe", BLES_FILTER_NAME_EXACT ) );
    RUN( "name exact longer", false, bles_filter_name_match( adv, sizeof(adv), "MyDevice", BLES_FILTER_NAME_EXACT ) );
}

static void test_filter_name_prefix( void )
{
    uint8_t adv[] = { 0x06, 0x09, 'M', 'y', 'D', 'e', 'v' };

    RUN( "name prefix ok", true, bles_filter_name_match( adv, sizeof(adv), "My", BLES_FILTER_NAME_PREFIX ) );
    RUN( "name prefix exact", true, bles_filter_name_match( adv, sizeof(adv), "MyDev", BLES_FILTER_NAME_PREFIX ) );
    RUN( "name prefix fail", false, bles_filter_name_match( adv, sizeof(adv), "You", BLES_FILTER_NAME_PREFIX ) );
}

static void test_filter_name_contains( void )
{
    uint8_t adv[] = { 0x06, 0x09, 'M', 'y', 'D', 'e', 'v' };

    RUN( "name contains ok", true, bles_filter_name_match( adv, sizeof(adv), "yDe", BLES_FILTER_NAME_CONTAINS ) );
    RUN( "name contains fail", false, bles_filter_name_match( adv, sizeof(adv), "zzz", BLES_FILTER_NAME_CONTAINS ) );
}

static void test_filter_name_short( void )
{
    uint8_t adv[] = { 0x04, 0x08, 'A', 'B', 'C' };
    uint8_t adv2[] = { 0x06, 0x09, 'A', 'B', 'C', 'D', 'E' };

    RUN( "name short ok", true, bles_filter_name_match( adv, sizeof(adv), "ABC", BLES_FILTER_NAME_EXACT ) );
    RUN( "name short+complete", true, bles_filter_name_match( adv2, sizeof(adv2), "ABCDE", BLES_FILTER_NAME_EXACT ) );
}

static void test_filter_name_null( void )
{
    uint8_t adv[] = { 0x06, 0x09, 'M', 'y', 'D', 'e', 'v' };

    RUN( "name null", false, bles_filter_name_match( adv, sizeof(adv), NULL, BLES_FILTER_NAME_EXACT ) );
}

static void test_filter_name_empty( void )
{
    uint8_t adv[] = { 0x06, 0x09, 'M', 'y', 'D', 'e', 'v' };

    RUN( "name empty", false, bles_filter_name_match( adv, sizeof(adv), "", BLES_FILTER_NAME_EXACT ) );
}

/* ==================== manufacturer filter ==================== */

static void test_filter_manu_exact( void )
{
    uint8_t adv[] = { 0x07, 0xFF, 0x4C, 0x00, 0x02, 0x15, 0xAA, 0xBB };
    uint8_t apple[] = { 0x4C, 0x00 };
    uint8_t ibeacon[] = { 0x4C, 0x00, 0x02, 0x15 };
    uint8_t custom[] = { 0xAA, 0xBB };
    uint8_t no[] = { 0xFF, 0xFF };

    RUN( "manu apple", true, bles_filter_manufacturer_match( adv, sizeof(adv), apple, 2 ) );
    RUN( "manu ibeacon", true, bles_filter_manufacturer_match( adv, sizeof(adv), ibeacon, 4 ) );
    RUN( "manu tail", true, bles_filter_manufacturer_match( adv, sizeof(adv), custom, 2 ) );
    RUN( "manu fail", false, bles_filter_manufacturer_match( adv, sizeof(adv), no, 2 ) );
}

static void test_filter_manu_too_long( void )
{
    uint8_t adv[] = { 0x04, 0xFF, 0xAA, 0xBB, 0xCC };
    uint8_t big[10] = { 0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00,0x11,0x22,0x33 };

    RUN( "manu too long", false, bles_filter_manufacturer_match( adv, sizeof(adv), big, 10 ) );
}

static void test_filter_manu_null( void )
{
    uint8_t adv[] = { 0x04, 0xFF, 0xAA, 0xBB, 0xCC };

    RUN( "manu null", false, bles_filter_manufacturer_match( adv, sizeof(adv), NULL, 2 ) );
}

static void test_filter_manu_len0( void )
{
    uint8_t adv[] = { 0x04, 0xFF, 0xAA, 0xBB, 0xCC };
    uint8_t d[] = { 0xAA };

    RUN( "manu len 0", false, bles_filter_manufacturer_match( adv, sizeof(adv), d, 0 ) );
}

static void test_filter_manu_string( void )
{
    uint8_t adv[] = { 0x0B, 0xFF, 0x4C, 0x00, 'M', 'y', 'D', 'e', 'v', 0x00, 0x01, 0x02 };
    const char *p_name = "MyDev";

    RUN( "manu string", true, bles_filter_manufacturer_match( adv, sizeof(adv), (const uint8_t*)p_name, 5 ) );
}

/* ==================== ibeacon ==================== */

static void test_ibeacon_build( void )
{
    uint8_t buf[31];
    bles_adv_builder_t b;
    bles_adv_ibeacon_params_t p =
    {
        .uuid     = { 0xE2,0xC5,0x6D,0xB5, 0xDF,0xFB, 0x48,0xD2, 0xB0,0x60, 0xD0,0xF5,0xA7,0x10,0x96,0xE0 },
        .major    = 1,
        .minor    = 2,
        .tx_power = -59,
    };
    uint8_t expect[] =
    {
        0x02, 0x01, 0x06,
        0x1A, 0xFF, 0x4C, 0x00, 0x02, 0x15,
        0xE2,0xC5,0x6D,0xB5, 0xDF,0xFB, 0x48,0xD2, 0xB0,0x60, 0xD0,0xF5,0xA7,0x10,0x96,0xE0,
        0x00, 0x01, 0x00, 0x02, 0xC5,
    };

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "ib ret", true, bles_adv_ibeacon_build( &b, &p ) );
    RUN( "ib len", 30, bles_adv_get_len( &b ) );
    RUN_MEM( "ib bytes", expect, buf, sizeof( expect ) );
}

static void test_ibeacon_null( void )
{
    uint8_t buf[31];
    bles_adv_builder_t b;
    bles_adv_ibeacon_params_t p = { 0 };

    RUN( "ib null builder", false, bles_adv_ibeacon_build( NULL, &p ) );

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "ib null params", false, bles_adv_ibeacon_build( &b, NULL ) );
    RUN( "ib null params len", 0, bles_adv_get_len( &b ) );
}

static void test_ibeacon_non_empty( void )
{
    uint8_t buf[31];
    bles_adv_builder_t b;
    bles_adv_ibeacon_params_t p = { 0 };

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    bles_adv_append_flags( &b, 0x06 );
    RUN( "ib non-empty builder", false, bles_adv_ibeacon_build( &b, &p ) );
}

static void test_ibeacon_uninit( void )
{
    bles_adv_builder_t b;
    bles_adv_ibeacon_params_t p = { 0 };

    memset( &b, 0, sizeof( b ) );
    RUN( "ib uninit builder", false, bles_adv_ibeacon_build( &b, &p ) );
}

static void test_ibeacon_no_room( void )
{
    uint8_t buf[29];
    bles_adv_builder_t b;
    bles_adv_ibeacon_params_t p = { 0 };

    bles_adv_builder_init( &b, buf, sizeof( buf ) );
    RUN( "ib no room ret", false, bles_adv_ibeacon_build( &b, &p ) );
    RUN( "ib no room len", 0, bles_adv_get_len( &b ) );
}

int main( void )
{
    printf( "\n=== builder ===\n" );
    test_builder_init_ok();
    test_builder_init_param();

    printf( "\n=== append common ===\n" );
    test_append_flags();
    test_append_name_complete();
    test_append_name_short();
    test_append_name_null();
    test_append_name_empty();
    test_append_uuid16();
    test_append_tx_power();

    printf( "\n=== boundary ===\n" );
    test_append_no_room();
    test_append_255();
    test_append_254();

    printf( "\n=== uuid filter ===\n" );
    test_filter_uuid16();
    test_filter_uuid_incomplete();
    test_filter_uuid_null();
    test_filter_uuid_bad_len();

    printf( "\n=== uuid128 ===\n" );
    test_filter_uuid128_complete();
    test_filter_uuid128_incomplete();
    test_filter_uuid128_miss();
    test_filter_uuid128_multi();

    printf( "\n=== filter core ===\n" );
    test_filter_find_exist();
    test_filter_malformed_trunc();
    test_filter_malformed_zero_len();

    printf( "\n=== name filter ===\n" );
    test_filter_name_exact();
    test_filter_name_prefix();
    test_filter_name_contains();
    test_filter_name_short();
    test_filter_name_null();
    test_filter_name_empty();

    printf( "\n=== manufacturer filter ===\n" );
    test_filter_manu_exact();
    test_filter_manu_too_long();
    test_filter_manu_null();
    test_filter_manu_len0();
    test_filter_manu_string();

    printf( "\n=== ibeacon ===\n" );
    test_ibeacon_build();
    test_ibeacon_null();
    test_ibeacon_non_empty();
    test_ibeacon_uninit();
    test_ibeacon_no_room();

    printf( "\n========================================\n" );
    printf( "passed: %d  failed: %d\n", g_pass, g_fail );
    printf( "RESULT: %s\n", g_fail == 0 ? "PASS" : "FAIL" );
    printf( "========================================\n" );

    return g_fail == 0 ? 0 : 1;
}
