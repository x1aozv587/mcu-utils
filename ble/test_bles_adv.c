#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bles_adv.h"
#include "bles_adv_ibeacon.h"

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
    bles_adv_ibeacon_params_t p = { 0 };

    RUN( "ib null builder", false, bles_adv_ibeacon_build( NULL, &p ) );
    RUN( "ib null params", false, bles_adv_ibeacon_build( NULL, NULL ) );
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
