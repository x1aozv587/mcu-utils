#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "mu_crc.h"
#include "mu_crc8.h"
#include "mu_crc16_modbus.h"
#include "mu_crc16_xmodem.h"
#include "mu_crc16_ccitt.h"
#include "mu_crc16_ccitt_false.h"

static int g_passed = 0;
static int g_failed = 0;

#define RUN_TEST( func, expect, width, actual ) \
    do { \
        printf( "  %-38s  expect=0x%0*X  actual=0x%0*X", \
                #func, \
                ( int )( width ), ( unsigned int )( expect ), \
                ( int )( width ), ( unsigned int )( actual ) ); \
        if( ( actual ) == ( expect ) ) \
        { \
            printf( "  PASS\n" ); \
            g_passed++; \
        } \
        else \
        { \
            printf( "  FAIL\n" ); \
            g_failed++; \
        } \
    } while( 0 )

/* ==================== 测试向量 ==================== */

static const uint8_t g_ascii[] = "123456789";
static const uint32_t g_ascii_len = 9;

/* ==================== CRC-8 ==================== */

static void test_crc8_vector( void )    { RUN_TEST( test_crc8_vector,    0xF4,   2, mu_crc8_calc( g_ascii, g_ascii_len ) ); }
static void test_crc8_empty( void )     { RUN_TEST( test_crc8_empty,     0x00,   2, mu_crc8_calc( NULL, 0 ) ); }
static void test_crc8_single( void )    { uint8_t d=0x01; RUN_TEST( test_crc8_single, 0x07, 2, mu_crc8_calc(&d,1) ); }

static void test_crc8_continue( void )
{
    uint8_t one = mu_crc8_calc( g_ascii, g_ascii_len );
    uint8_t s1  = mu_crc8_calc( g_ascii, 4 );
    uint8_t s2  = mu_crc8_continue( s1, g_ascii + 4, g_ascii_len - 4 );

    RUN_TEST( test_crc8_continue, one, 2, s2 );
}

/* ==================== CRC-16 MODBUS ==================== */

static void test_modbus_vector( void )    { RUN_TEST( test_modbus_vector,    0x4B37, 4, mu_crc16_modbus( g_ascii, g_ascii_len ) ); }
static void test_modbus_empty( void )     { RUN_TEST( test_modbus_empty,     0xFFFF, 4, mu_crc16_modbus( NULL, 0 ) ); }
static void test_modbus_single( void )    { uint8_t d=0x01; RUN_TEST( test_modbus_single, 0x807E, 4, mu_crc16_modbus(&d,1) ); }
static void test_modbus_frame( void )     { uint8_t f[]={0x01,0x03,0x00,0x00,0x00,0x01}; RUN_TEST( test_modbus_frame, 0x0A84, 4, mu_crc16_modbus(f,sizeof(f)) ); }

/* ==================== CRC-16 XMODEM ==================== */

static void test_xmodem_vector( void )    { RUN_TEST( test_xmodem_vector,    0x31C3, 4, mu_crc16_xmodem( g_ascii, g_ascii_len ) ); }
static void test_xmodem_empty( void )     { RUN_TEST( test_xmodem_empty,     0x0000, 4, mu_crc16_xmodem( NULL, 0 ) ); }
static void test_xmodem_frame( void )     { uint8_t f[]={0x01,0x03,0x00,0x00,0x00,0x01}; RUN_TEST( test_xmodem_frame, 0xBB53, 4, mu_crc16_xmodem(f,sizeof(f)) ); }

/* ==================== CRC-16 CCITT ==================== */

static void test_ccitt_vector( void )     { RUN_TEST( test_ccitt_vector,     0x2189, 4, mu_crc16_ccitt( g_ascii, g_ascii_len ) ); }
static void test_ccitt_empty( void )      { RUN_TEST( test_ccitt_empty,      0x0000, 4, mu_crc16_ccitt( NULL, 0 ) ); }

static void test_ccitt_continue( void )
{
    uint16_t one = mu_crc16_ccitt( g_ascii, g_ascii_len );
    uint16_t s1  = mu_crc16_ccitt( g_ascii, 4 );
    uint16_t s2  = mu_crc16_ccitt_continue( s1, g_ascii + 4, g_ascii_len - 4 );

    RUN_TEST( test_ccitt_continue, one, 4, s2 );
}

/* ==================== CRC-16 CCITT-FALSE ==================== */

static void test_ccitt_false_vector( void ) { RUN_TEST( test_ccitt_false_vector, 0x29B1, 4, mu_crc16_ccitt_false( g_ascii, g_ascii_len ) ); }
static void test_ccitt_false_empty( void )  { RUN_TEST( test_ccitt_false_empty,  0xFFFF, 4, mu_crc16_ccitt_false( NULL, 0 ) ); }

static void test_ccitt_false_continue( void )
{
    uint16_t one = mu_crc16_ccitt_false( g_ascii, g_ascii_len );
    uint16_t s1  = mu_crc16_ccitt_false( g_ascii, 4 );
    uint16_t s2  = mu_crc16_ccitt_false_continue( s1, g_ascii + 4, g_ascii_len - 4 );

    RUN_TEST( test_ccitt_false_continue, one, 4, s2 );
}

/* ==================== mu_crc_calc ==================== */

static void test_calc_crc8( void )
{
    mu_crc_params_t p = { 8, 0x07, 0x00, 0x00, false, false };
    RUN_TEST( test_calc_crc8, 0xF4, 2, mu_crc_calc( &p, g_ascii, g_ascii_len ) );
}

static void test_calc_ccitt( void )
{
    mu_crc_params_t p = { 16, 0x1021, 0x0000, 0x0000, false, false };
    RUN_TEST( test_calc_ccitt, 0x31C3, 4, mu_crc_calc( &p, g_ascii, g_ascii_len ) );
}

/* ==================== 表 vs 位 ==================== */

static void test_tbl_vs_bit_modbus( void )
{
    mu_crc_params_t p = { 16, 0x8005, 0xFFFF, 0x0000, true, true };
    RUN_TEST( test_tbl_vs_bit_modbus, 0x4B37, 4, mu_crc_calc( &p, g_ascii, g_ascii_len ) );
}

static void test_tbl_vs_bit_xmodem( void )
{
    mu_crc_params_t p = { 16, 0x1021, 0x0000, 0x0000, false, false };
    RUN_TEST( test_tbl_vs_bit_xmodem, 0x31C3, 4, mu_crc_calc( &p, g_ascii, g_ascii_len ) );
}

static void test_tbl_null( void )
{
    mu_crc_params_t p = { 16, 0x1021, 0x0000, 0x0000, false, false };
    RUN_TEST( test_tbl_null, 0x00000000, 8, mu_crc_calc_tbl( &p, g_ascii, g_ascii_len, NULL ) );
}

/* ==================== continue ==================== */

static void test_continue_modbus( void )
{
    uint16_t one = mu_crc16_modbus( g_ascii, g_ascii_len );
    uint16_t s1  = mu_crc16_modbus( g_ascii, 4 );
    uint16_t s2  = mu_crc16_modbus_continue( s1, g_ascii + 4, g_ascii_len - 4 );

    RUN_TEST( test_continue_modbus, one, 4, s2 );
}

static void test_continue_xmodem( void )
{
    uint16_t one = mu_crc16_xmodem( g_ascii, g_ascii_len );
    uint16_t s1  = mu_crc16_xmodem( g_ascii, 4 );
    uint16_t s2  = mu_crc16_xmodem_continue( s1, g_ascii + 4, g_ascii_len - 4 );

    RUN_TEST( test_continue_xmodem, one, 4, s2 );
}

/* ==================== 参数校验 ==================== */

static void test_null_param( void )
{
    RUN_TEST( test_null_param, 0x00000000, 8, mu_crc_calc( NULL, g_ascii, g_ascii_len ) );
}

static void test_null_data( void )
{
    mu_crc_params_t p = { 8, 0x07, 0x00, 0x00, false, false };
    RUN_TEST( test_null_data, 0x00000000, 8, mu_crc_calc( &p, NULL, 0 ) );
}

static void test_zero_width( void )
{
    mu_crc_params_t p = { 0, 0x07, 0x00, 0x00, false, false };
    RUN_TEST( test_zero_width, 0x00000000, 8, mu_crc_calc( &p, g_ascii, g_ascii_len ) );
}

/* ==================== main ==================== */

int main( int argc, char *argv[] )
{
    if( argc >= 2 && strcmp( argv[1], "list" ) == 0 )
    {
        printf( "Usage: %s\n", argv[0] );

        return 0;
    }

    printf( "\n" );
    printf( "=== CRC-8 ===\n" );
    test_crc8_vector();
    test_crc8_empty();
    test_crc8_single();
    test_crc8_continue();

    printf( "\n" );
    printf( "=== CRC-16 MODBUS ===\n" );
    test_modbus_vector();
    test_modbus_empty();
    test_modbus_single();
    test_modbus_frame();

    printf( "\n" );
    printf( "=== CRC-16 XMODEM ===\n" );
    test_xmodem_vector();
    test_xmodem_empty();
    test_xmodem_frame();

    printf( "\n" );
    printf( "=== CRC-16 CCITT ===\n" );
    test_ccitt_vector();
    test_ccitt_empty();
    test_ccitt_continue();

    printf( "\n" );
    printf( "=== CRC-16 CCITT-FALSE ===\n" );
    test_ccitt_false_vector();
    test_ccitt_false_empty();
    test_ccitt_false_continue();

    printf( "\n" );
    printf( "=== mu_crc_calc ===\n" );
    test_calc_crc8();
    test_calc_ccitt();

    printf( "\n" );
    printf( "=== table vs bitwise ===\n" );
    test_tbl_vs_bit_modbus();
    test_tbl_vs_bit_xmodem();
    test_tbl_null();

    printf( "\n" );
    printf( "=== continue ===\n" );
    test_continue_modbus();
    test_continue_xmodem();

    printf( "\n" );
    printf( "=== parameter check ===\n" );
    test_null_param();
    test_null_data();
    test_zero_width();

    printf( "\n" );
    printf( "========================================\n" );
    printf( "  passed: %d  failed: %d\n", g_passed, g_failed );

    if( g_failed > 0 )
    {
        printf( "  RESULT: FAIL\n" );
    }
    else
    {
        printf( "  RESULT: PASS (all)\n" );
    }
    printf( "========================================\n" );

    return g_failed > 0 ? 1 : 0;
}
