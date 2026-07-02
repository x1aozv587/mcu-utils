#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "mu_crc.h"
#include "mu_crc16_modbus.h"
#include "mu_crc16_xmodem.h"

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

/* ==================== CRC-16 MODBUS ==================== */

static void test_modbus_vector( void )
{
    uint16_t result = mu_crc16_modbus( g_ascii, g_ascii_len );

    RUN_TEST( test_modbus_vector, 0x4B37, 4, result );
}

static void test_modbus_empty( void )
{
    uint16_t result = mu_crc16_modbus( NULL, 0 );

    RUN_TEST( test_modbus_empty, 0xFFFF, 4, result );
}

static void test_modbus_single( void )
{
    uint8_t data = 0x01;
    uint16_t result = mu_crc16_modbus( &data, 1 );

    RUN_TEST( test_modbus_single, 0x807E, 4, result );
}

static void test_modbus_hex_frame( void )
{
    uint8_t frame[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01 };
    uint16_t result = mu_crc16_modbus( frame, sizeof( frame ) );

    RUN_TEST( test_modbus_hex_frame, 0x0A84, 4, result );
}

/* ==================== CRC-16 XMODEM ==================== */

static void test_xmodem_vector( void )
{
    uint16_t result = mu_crc16_xmodem( g_ascii, g_ascii_len );

    RUN_TEST( test_xmodem_vector, 0x31C3, 4, result );
}

static void test_xmodem_empty( void )
{
    uint16_t result = mu_crc16_xmodem( NULL, 0 );

    RUN_TEST( test_xmodem_empty, 0x0000, 4, result );
}

static void test_xmodem_hex_frame( void )
{
    uint8_t frame[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01 };
    uint16_t result = mu_crc16_xmodem( frame, sizeof( frame ) );

    RUN_TEST( test_xmodem_hex_frame, 0xBB53, 4, result );
}

/* ==================== mu_crc_calc 通用引擎 ==================== */

static void test_calc_crc8( void )
{
    mu_crc_params_t param = { 8, 0x07, 0x00, 0x00, false, false };
    uint32_t result = mu_crc_calc( &param, g_ascii, g_ascii_len );

    RUN_TEST( test_calc_crc8, 0xF4, 2, result );
}

static void test_calc_ccitt( void )
{
    mu_crc_params_t param = { 16, 0x1021, 0x0000, 0x0000, false, false };
    uint32_t result = mu_crc_calc( &param, g_ascii, g_ascii_len );

    RUN_TEST( test_calc_ccitt, 0x31C3, 4, result );
}

/* ==================== 查表 vs 逐位一致性 ==================== */

static void test_tbl_vs_bitwise_modbus( void )
{
    mu_crc_params_t param = { 16, 0x8005, 0xFFFF, 0x0000, true, true };
    uint32_t result_bit = mu_crc_calc( &param, g_ascii, g_ascii_len );

    RUN_TEST( test_tbl_vs_bitwise_modbus, 0x4B37, 4, result_bit );
}

static void test_tbl_vs_bitwise_xmodem( void )
{
    mu_crc_params_t param = { 16, 0x1021, 0x0000, 0x0000, false, false };
    uint32_t result_bit = mu_crc_calc( &param, g_ascii, g_ascii_len );

    RUN_TEST( test_tbl_vs_bitwise_xmodem, 0x31C3, 4, result_bit );
}

static void test_tbl_null_table( void )
{
    mu_crc_params_t param = { 16, 0x1021, 0x0000, 0x0000, false, false };
    uint32_t result = mu_crc_calc_tbl( &param, g_ascii, g_ascii_len, NULL );

    RUN_TEST( test_tbl_null_table, 0x00000000, 8, result );
}

/* ==================== continue 续算 ==================== */

static void test_continue_modbus( void )
{
    uint16_t one_shot = 0;
    uint16_t step1 = 0;
    uint16_t step2 = 0;

    one_shot = mu_crc16_modbus( g_ascii, g_ascii_len );
    step1    = mu_crc16_modbus( g_ascii, 4 );
    step2    = mu_crc16_modbus_continue( step1, g_ascii + 4, g_ascii_len - 4 );

    RUN_TEST( test_continue_modbus, one_shot, 4, step2 );
}

static void test_continue_xmodem( void )
{
    uint16_t one_shot = 0;
    uint16_t step1 = 0;
    uint16_t step2 = 0;

    one_shot = mu_crc16_xmodem( g_ascii, g_ascii_len );
    step1    = mu_crc16_xmodem( g_ascii, 4 );
    step2    = mu_crc16_xmodem_continue( step1, g_ascii + 4, g_ascii_len - 4 );

    RUN_TEST( test_continue_xmodem, one_shot, 4, step2 );
}

/* ==================== 参数校验 ==================== */

static void test_null_param( void )
{
    uint32_t result = mu_crc_calc( NULL, g_ascii, g_ascii_len );

    RUN_TEST( test_null_param, 0x00000000, 8, result );
}

static void test_null_data( void )
{
    mu_crc_params_t param = { 8, 0x07, 0x00, 0x00, false, false };
    uint32_t result = mu_crc_calc( &param, NULL, 0 );

    RUN_TEST( test_null_data, 0x00000000, 8, result );
}

static void test_zero_width( void )
{
    mu_crc_params_t param = { 0, 0x07, 0x00, 0x00, false, false };
    uint32_t result = mu_crc_calc( &param, g_ascii, g_ascii_len );

    RUN_TEST( test_zero_width, 0x00000000, 8, result );
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
    printf( "=== CRC-16 MODBUS ===\n" );
    test_modbus_vector();
    test_modbus_empty();
    test_modbus_single();
    test_modbus_hex_frame();

    printf( "\n" );
    printf( "=== CRC-16 XMODEM ===\n" );
    test_xmodem_vector();
    test_xmodem_empty();
    test_xmodem_hex_frame();

    printf( "\n" );
    printf( "=== mu_crc_calc ===\n" );
    test_calc_crc8();
    test_calc_ccitt();

    printf( "\n" );
    printf( "=== table vs bitwise ===\n" );
    test_tbl_vs_bitwise_modbus();
    test_tbl_vs_bitwise_xmodem();
    test_tbl_null_table();

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
