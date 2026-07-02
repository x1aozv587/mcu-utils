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
#include "mu_crc32.h"

static int g_passed = 0;
static int g_failed = 0;

#define RUN( func, expect, width, actual ) \
    do { \
        printf( "  %-38s  expect=0x%0*X  actual=0x%0*X", \
                #func, (int)(width), (unsigned)(expect), (int)(width), (unsigned)(actual) ); \
        if( (actual) == (expect) ) { printf( "  PASS\n" ); g_passed++; } \
        else { printf( "  FAIL\n" ); g_failed++; } \
    } while(0)

static const uint8_t g_vec[] = "123456789";
static const uint32_t g_len = 9;
#define D  g_vec, g_len
#define D0 NULL, 0

static void t_crc8(void)  { RUN(t_crc8,  0xF4, 2, mu_crc8_calc(D)); }
static void t_crc8_e(void) { RUN(t_crc8_e, 0x00, 2, mu_crc8_calc(D0)); }
static void t_crc8_1(void) { uint8_t d=1; RUN(t_crc8_1, 0x07, 2, mu_crc8_calc(&d,1)); }
static void t_crc8_c(void) { uint8_t a=mu_crc8_calc(D),b=mu_crc8_calc(g_vec,4),c=mu_crc8_continue(b,g_vec+4,g_len-4); RUN(t_crc8_c,a,2,c); }

static void t_mb(void)  { RUN(t_mb,  0x4B37, 4, mu_crc16_modbus(D)); }
static void t_mb_e(void) { RUN(t_mb_e, 0xFFFF, 4, mu_crc16_modbus(D0)); }
static void t_mb_1(void) { uint8_t d=1; RUN(t_mb_1, 0x807E, 4, mu_crc16_modbus(&d,1)); }
static void t_mb_f(void) { uint8_t f[]={1,3,0,0,0,1}; RUN(t_mb_f, 0x0A84, 4, mu_crc16_modbus(f,sizeof(f))); }

static void t_xm(void)  { RUN(t_xm,  0x31C3, 4, mu_crc16_xmodem(D)); }
static void t_xm_e(void) { RUN(t_xm_e, 0x0000, 4, mu_crc16_xmodem(D0)); }
static void t_xm_f(void) { uint8_t f[]={1,3,0,0,0,1}; RUN(t_xm_f, 0xBB53, 4, mu_crc16_xmodem(f,sizeof(f))); }

static void t_cc(void)  { RUN(t_cc,  0x2189, 4, mu_crc16_ccitt(D)); }
static void t_cc_e(void) { RUN(t_cc_e, 0x0000, 4, mu_crc16_ccitt(D0)); }
static void t_cc_c(void) { uint16_t a=mu_crc16_ccitt(D),b=mu_crc16_ccitt(g_vec,4),c=mu_crc16_ccitt_continue(b,g_vec+4,g_len-4); RUN(t_cc_c,a,4,c); }

static void t_cf(void)  { RUN(t_cf,  0x29B1, 4, mu_crc16_ccitt_false(D)); }
static void t_cf_e(void) { RUN(t_cf_e, 0xFFFF, 4, mu_crc16_ccitt_false(D0)); }
static void t_cf_c(void) { uint16_t a=mu_crc16_ccitt_false(D),b=mu_crc16_ccitt_false(g_vec,4),c=mu_crc16_ccitt_false_continue(b,g_vec+4,g_len-4); RUN(t_cf_c,a,4,c); }

static void t_c32(void)  { RUN(t_c32,  0xCBF43926, 8, mu_crc32_calc(D)); }
static void t_c32_e(void) { RUN(t_c32_e, 0x00000000, 8, mu_crc32_calc(D0)); }
static void t_c32_c(void) { uint32_t a=mu_crc32_calc(D),b=mu_crc32_calc(g_vec,4),c=mu_crc32_continue(b,g_vec+4,g_len-4); RUN(t_c32_c,a,8,c); }

static void t_calc8(void)  { mu_crc_params_t p={8,0x07,0,0,false,false};  RUN(t_calc8,  0xF4,   2, mu_crc_calc(&p,D)); }
static void t_calc16(void) { mu_crc_params_t p={16,0x1021,0,0,false,false}; RUN(t_calc16, 0x31C3, 4, mu_crc_calc(&p,D)); }

static void t_tbl_mb(void) { mu_crc_params_t p={16,0x8005,0xFFFF,0,true,true};   RUN(t_tbl_mb, 0x4B37, 4, mu_crc_calc(&p,D)); }
static void t_tbl_xm(void) { mu_crc_params_t p={16,0x1021,0,0,false,false};      RUN(t_tbl_xm, 0x31C3, 4, mu_crc_calc(&p,D)); }
static void t_tbl_n(void)  { mu_crc_params_t p={16,0x1021,0,0,false,false};      RUN(t_tbl_n,  0x0,     8, mu_crc_calc_tbl(&p,D,NULL)); }

static void t_cnt_mb(void) { uint16_t a=mu_crc16_modbus(D),b=mu_crc16_modbus(g_vec,4),c=mu_crc16_modbus_continue(b,g_vec+4,g_len-4); RUN(t_cnt_mb,a,4,c); }
static void t_cnt_xm(void) { uint16_t a=mu_crc16_xmodem(D),b=mu_crc16_xmodem(g_vec,4),c=mu_crc16_xmodem_continue(b,g_vec+4,g_len-4); RUN(t_cnt_xm,a,4,c); }

static void t_null_p(void) { RUN(t_null_p, 0, 8, mu_crc_calc(NULL,D)); }
static void t_null_d(void) { mu_crc_params_t p={8,0x07,0,0,false,false}; RUN(t_null_d, 0, 8, mu_crc_calc(&p,D0)); }
static void t_zero_w(void) { mu_crc_params_t p={0,0x07,0,0,false,false}; RUN(t_zero_w, 0, 8, mu_crc_calc(&p,D)); }

int main( int argc, char *argv[] )
{
    if( argc >= 2 && strcmp( argv[1], "list" ) == 0 )
    {
        printf( "Usage: %s\n", argv[0] );
        return 0;
    }

    printf( "\n=== CRC-8 ===\n" );               t_crc8();t_crc8_e();t_crc8_1();t_crc8_c();
    printf( "\n=== CRC-16 MODBUS ===\n" );        t_mb();t_mb_e();t_mb_1();t_mb_f();
    printf( "\n=== CRC-16 XMODEM ===\n" );        t_xm();t_xm_e();t_xm_f();
    printf( "\n=== CRC-16 CCITT ===\n" );         t_cc();t_cc_e();t_cc_c();
    printf( "\n=== CRC-16 CCITT-FALSE ===\n" );   t_cf();t_cf_e();t_cf_c();
    printf( "\n=== CRC-32 ===\n" );               t_c32();t_c32_e();t_c32_c();
    printf( "\n=== mu_crc_calc ===\n" );          t_calc8();t_calc16();
    printf( "\n=== table vs bitwise ===\n" );     t_tbl_mb();t_tbl_xm();t_tbl_n();
    printf( "\n=== continue ===\n" );             t_cnt_mb();t_cnt_xm();
    printf( "\n=== parameter check ===\n" );      t_null_p();t_null_d();t_zero_w();

    printf( "\n========================================\n" );
    printf( "  passed: %d  failed: %d\n", g_passed, g_failed );
    printf( "  RESULT: %s\n", g_failed > 0 ? "FAIL" : "PASS (all)" );
    printf( "========================================\n" );
    return g_failed > 0 ? 1 : 0;
}
