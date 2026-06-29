#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "mu_ringbuf.h"

#define TEST_GROUP_BASE     ( 1 << 0 )
#define TEST_GROUP_LOCK     ( 1 << 1 )
#define TEST_GROUP_P0       ( 1 << 2 )
#define TEST_GROUP_P1       ( 1 << 3 )
#define TEST_GROUP_P2       ( 1 << 4 )

static int g_passed = 0;
static int g_failed = 0;
static int g_skipped = 0;
static unsigned int g_groups = 0;

#define RUN_TEST( grp, func ) \
    do { \
        if( g_groups & ( grp ) ) \
        { \
            printf( "  %-40s", #func ); \
            fflush( stdout ); \
            func(); \
            printf( " OK\n" ); \
            g_passed++; \
        } \
        else \
        { \
            g_skipped++; \
        } \
    } while( 0 )

static int lock_count = 0;
static int unlock_count = 0;

static void test_lock( void )
{
    lock_count++;
}

static void test_unlock( void )
{
    unlock_count++;
}

static void expect_buf( const uint8_t *p_actual, const uint8_t *p_expect, uint32_t len )
{
    assert( memcmp( p_actual, p_expect, len ) == 0 );
}

/* ==================== P0 ==================== */

static void test_init_return_value( void )
{
    uint8_t buf[4];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( NULL, buf, 4, NULL, NULL ) == MU_ERR_PARAM );
    assert( mu_ringbuf_init( &rb, NULL, 4, NULL, NULL ) == MU_ERR_PARAM );
    assert( mu_ringbuf_init( &rb, buf, 0, NULL, NULL ) == MU_ERR_PARAM );
    assert( mu_ringbuf_init( &rb, buf, 1, NULL, NULL ) == MU_ERR_PARAM );

    assert( mu_ringbuf_init( &rb, buf, 2, NULL, NULL ) == MU_OK );
    assert( mu_ringbuf_get_capacity( &rb ) == 1 );

    assert( mu_ringbuf_init( &rb, buf, 4, NULL, NULL ) == MU_OK );
    assert( mu_ringbuf_get_capacity( &rb ) == 3 );
}

static void test_invalid_object_returns_safe( void )
{
    uint8_t buf[8];
    uint8_t data[4] = { 1, 2, 3, 4 };
    mu_ringbuf_t rb_uninit;

    memset( &rb_uninit, 0, sizeof( rb_uninit ) );

    assert( mu_ringbuf_write( &rb_uninit, data, 4 ) == 0 );
    assert( mu_ringbuf_read( &rb_uninit, data, 4 ) == 0 );
    assert( mu_ringbuf_get_count( &rb_uninit ) == 0 );
    assert( mu_ringbuf_get_free( &rb_uninit ) == 0 );
    assert( mu_ringbuf_get_capacity( &rb_uninit ) == 0 );
    assert( mu_ringbuf_is_empty( &rb_uninit ) == true );
    assert( mu_ringbuf_is_full( &rb_uninit ) == true );

    mu_ringbuf_reset( &rb_uninit );
    assert( mu_ringbuf_peek( &rb_uninit, 0, data, 1 ) == 0 );
    assert( mu_ringbuf_drop( &rb_uninit, 1 ) == 0 );
    assert( mu_ringbuf_write_byte( &rb_uninit, 0xAA ) == 0 );
    assert( mu_ringbuf_read_byte( &rb_uninit, data ) == 0 );

    assert( mu_ringbuf_write( NULL, data, 4 ) == 0 );
    assert( mu_ringbuf_read( NULL, data, 4 ) == 0 );
    assert( mu_ringbuf_get_count( NULL ) == 0 );
    assert( mu_ringbuf_get_free( NULL ) == 0 );
    assert( mu_ringbuf_get_capacity( NULL ) == 0 );
    assert( mu_ringbuf_is_empty( NULL ) == true );
    assert( mu_ringbuf_is_full( NULL ) == true );
    assert( mu_ringbuf_peek( NULL, 0, data, 1 ) == 0 );
    assert( mu_ringbuf_drop( NULL, 1 ) == 0 );
    assert( mu_ringbuf_write_byte( NULL, 0xAA ) == 0 );
    assert( mu_ringbuf_read_byte( NULL, data ) == 0 );

    mu_ringbuf_init( &rb_uninit, buf, sizeof( buf ), NULL, NULL );

    assert( mu_ringbuf_write( &rb_uninit, NULL, 4 ) == 0 );
    assert( mu_ringbuf_write( &rb_uninit, data, 0 ) == 0 );
    assert( mu_ringbuf_read( &rb_uninit, NULL, 4 ) == 0 );
    assert( mu_ringbuf_read( &rb_uninit, data, 0 ) == 0 );
    assert( mu_ringbuf_peek( &rb_uninit, 0, NULL, 1 ) == 0 );
    assert( mu_ringbuf_peek( &rb_uninit, 0, data, 0 ) == 0 );
    assert( mu_ringbuf_drop( &rb_uninit, 0 ) == 0 );
    assert( mu_ringbuf_read_byte( &rb_uninit, NULL ) == 0 );
}

/* ==================== BASE ==================== */

static void test_init_empty_free( void )
{
    uint8_t buf[8];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_is_empty( &rb ) == true );
    assert( mu_ringbuf_is_full( &rb ) == false );
    assert( mu_ringbuf_get_count( &rb ) == 0 );
    assert( mu_ringbuf_get_free( &rb ) == 7 );
    assert( mu_ringbuf_get_capacity( &rb ) == 7 );
}

static void test_write_read_basic( void )
{
    uint8_t buf[8];
    uint8_t out[8] = { 0 };
    uint8_t in[] = { 1, 2, 3, 4 };
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, in, sizeof( in ) ) == 4 );
    assert( mu_ringbuf_get_count( &rb ) == 4 );
    assert( mu_ringbuf_get_free( &rb ) == 3 );

    assert( mu_ringbuf_read( &rb, out, 2 ) == 2 );
    expect_buf( out, ( uint8_t[] ){ 1, 2 }, 2 );
    assert( mu_ringbuf_get_count( &rb ) == 2 );

    memset( out, 0, sizeof( out ) );
    assert( mu_ringbuf_read( &rb, out, sizeof( out ) ) == 2 );
    expect_buf( out, ( uint8_t[] ){ 3, 4 }, 2 );
    assert( mu_ringbuf_is_empty( &rb ) == true );
}

static void test_read_empty( void )
{
    uint8_t buf[8];
    uint8_t out[4];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_read( &rb, out, sizeof( out ) ) == 0 );
    assert( mu_ringbuf_read_byte( &rb, out ) == 0 );
}

static void test_full_truncate( void )
{
    uint8_t buf[4];
    uint8_t out[4] = { 0 };
    uint8_t in[] = { 10, 11, 12, 13, 14 };
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, in, sizeof( in ) ) == 3 );
    assert( mu_ringbuf_is_full( &rb ) == true );
    assert( mu_ringbuf_get_count( &rb ) == 3 );
    assert( mu_ringbuf_get_free( &rb ) == 0 );

    assert( mu_ringbuf_read( &rb, out, sizeof( out ) ) == 3 );
    expect_buf( out, ( uint8_t[] ){ 10, 11, 12 }, 3 );
}

static void test_wrap_around( void )
{
    uint8_t buf[5];
    uint8_t out[5] = { 0 };
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2, 3 }, 3 ) == 3 );
    assert( mu_ringbuf_read( &rb, out, 2 ) == 2 );
    expect_buf( out, ( uint8_t[] ){ 1, 2 }, 2 );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 4, 5, 6 }, 3 ) == 3 );
    assert( mu_ringbuf_get_count( &rb ) == 4 );

    memset( out, 0, sizeof( out ) );
    assert( mu_ringbuf_read( &rb, out, 4 ) == 4 );
    expect_buf( out, ( uint8_t[] ){ 3, 4, 5, 6 }, 4 );
    assert( mu_ringbuf_is_empty( &rb ) == true );
}

static void test_reset( void )
{
    uint8_t buf[8];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2, 3 }, 3 ) == 3 );

    mu_ringbuf_reset( &rb );

    assert( mu_ringbuf_is_empty( &rb ) == true );
    assert( mu_ringbuf_get_count( &rb ) == 0 );
    assert( mu_ringbuf_get_free( &rb ) == 7 );
}

static void test_write_large_buffer( void )
{
    uint8_t buf[128];
    uint8_t out[128] = { 0 };
    uint8_t in[128];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    for( int i = 0; i < 128; i++ )
    {
        in[i] = ( uint8_t )( i & 0xFF );
    }

    assert( mu_ringbuf_write( &rb, in, 127 ) == 127 );
    assert( mu_ringbuf_get_count( &rb ) == 127 );
    assert( mu_ringbuf_is_full( &rb ) == true );

    assert( mu_ringbuf_read( &rb, out, 127 ) == 127 );
    expect_buf( out, in, 127 );
    assert( mu_ringbuf_is_empty( &rb ) == true );
}

/* ==================== LOCK ==================== */

static void test_lock_unlock( void )
{
    uint8_t buf[8];
    uint8_t out[2];
    mu_ringbuf_t rb;

    lock_count = 0;
    unlock_count = 0;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), test_lock, test_unlock ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2 }, 2 ) == 2 );
    assert( mu_ringbuf_read( &rb, out, 1 ) == 1 );

    mu_ringbuf_reset( &rb );

    assert( lock_count == 3 );
    assert( unlock_count == 3 );
}

static void test_lock_unlock_on_query( void )
{
    uint8_t buf[8];
    mu_ringbuf_t rb;

    lock_count = 0;
    unlock_count = 0;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), test_lock, test_unlock ) == MU_OK );

    mu_ringbuf_is_empty( &rb );
    mu_ringbuf_is_full( &rb );
    mu_ringbuf_get_count( &rb );
    mu_ringbuf_get_free( &rb );

    assert( lock_count == 4 );
    assert( unlock_count == 4 );
}

/* ==================== P2 ==================== */

static void test_get_capacity( void )
{
    uint8_t buf[5];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );
    assert( mu_ringbuf_get_capacity( &rb ) == 4 );
}

static void test_peek_basic( void )
{
    uint8_t buf[8];
    uint8_t out[8] = { 0 };
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 10, 20, 30 }, 3 ) == 3 );

    assert( mu_ringbuf_peek( &rb, 0, out, 2 ) == 2 );
    expect_buf( out, ( uint8_t[] ){ 10, 20 }, 2 );

    assert( mu_ringbuf_get_count( &rb ) == 3 );

    assert( mu_ringbuf_peek( &rb, 1, out, 2 ) == 2 );
    expect_buf( out, ( uint8_t[] ){ 20, 30 }, 2 );

    assert( mu_ringbuf_get_count( &rb ) == 3 );
}

static void test_peek_offset_exceed( void )
{
    uint8_t buf[8];
    uint8_t out[4];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2 }, 2 ) == 2 );

    assert( mu_ringbuf_peek( &rb, 2, out, 1 ) == 0 );
    assert( mu_ringbuf_peek( &rb, 5, out, 1 ) == 0 );
}

static void test_peek_wrap( void )
{
    uint8_t buf[5];
    uint8_t out[4] = { 0 };
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2, 3 }, 3 ) == 3 );
    assert( mu_ringbuf_read( &rb, out, 2 ) == 2 );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 4, 5, 6 }, 3 ) == 3 );

    memset( out, 0, sizeof( out ) );
    assert( mu_ringbuf_peek( &rb, 0, out, 4 ) == 4 );
    expect_buf( out, ( uint8_t[] ){ 3, 4, 5, 6 }, 4 );
}

static void test_drop_basic( void )
{
    uint8_t buf[8];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2, 3, 4 }, 4 ) == 4 );

    assert( mu_ringbuf_drop( &rb, 2 ) == 2 );
    assert( mu_ringbuf_get_count( &rb ) == 2 );
}

static void test_drop_exceed( void )
{
    uint8_t buf[8];
    uint8_t out[4];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2 }, 2 ) == 2 );

    assert( mu_ringbuf_drop( &rb, 10 ) == 2 );
    assert( mu_ringbuf_is_empty( &rb ) == true );

    assert( mu_ringbuf_read( &rb, out, 4 ) == 0 );
}

static void test_drop_and_read( void )
{
    uint8_t buf[8];
    uint8_t out[4];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write( &rb, ( uint8_t[] ){ 1, 2, 3, 4, 5 }, 5 ) == 5 );

    assert( mu_ringbuf_drop( &rb, 3 ) == 3 );
    assert( mu_ringbuf_read( &rb, out, 2 ) == 2 );
    expect_buf( out, ( uint8_t[] ){ 4, 5 }, 2 );
    assert( mu_ringbuf_is_empty( &rb ) == true );
}

static void test_read_write_byte( void )
{
    uint8_t buf[4];
    uint8_t data = 0;
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write_byte( &rb, 0xA5 ) == 1 );
    assert( mu_ringbuf_write_byte( &rb, 0x5A ) == 1 );
    assert( mu_ringbuf_write_byte( &rb, 0xFF ) == 1 );
    assert( mu_ringbuf_get_count( &rb ) == 3 );

    assert( mu_ringbuf_read_byte( &rb, &data ) == 1 );
    assert( data == 0xA5 );

    assert( mu_ringbuf_read_byte( &rb, &data ) == 1 );
    assert( data == 0x5A );

    assert( mu_ringbuf_read_byte( &rb, &data ) == 1 );
    assert( data == 0xFF );

    assert( mu_ringbuf_read_byte( &rb, &data ) == 0 );
}

static void test_write_byte_full( void )
{
    uint8_t buf[4];
    mu_ringbuf_t rb;

    assert( mu_ringbuf_init( &rb, buf, sizeof( buf ), NULL, NULL ) == MU_OK );

    assert( mu_ringbuf_write_byte( &rb, 1 ) == 1 );
    assert( mu_ringbuf_write_byte( &rb, 2 ) == 1 );
    assert( mu_ringbuf_write_byte( &rb, 3 ) == 1 );
    assert( mu_ringbuf_is_full( &rb ) == true );
    assert( mu_ringbuf_write_byte( &rb, 4 ) == 0 );
}

static void run_tests( void )
{
    printf( "\n" );
    printf( "--- P0 ----------\n" );
    RUN_TEST( TEST_GROUP_P0, test_init_return_value );
    RUN_TEST( TEST_GROUP_P0, test_invalid_object_returns_safe );

    printf( "\n" );
    printf( "--- BASE ----------\n" );
    RUN_TEST( TEST_GROUP_BASE, test_init_empty_free );
    RUN_TEST( TEST_GROUP_BASE, test_write_read_basic );
    RUN_TEST( TEST_GROUP_BASE, test_read_empty );
    RUN_TEST( TEST_GROUP_BASE, test_full_truncate );
    RUN_TEST( TEST_GROUP_BASE, test_wrap_around );
    RUN_TEST( TEST_GROUP_BASE, test_reset );
    RUN_TEST( TEST_GROUP_BASE, test_write_large_buffer );

    printf( "\n" );
    printf( "--- LOCK ----------\n" );
    RUN_TEST( TEST_GROUP_LOCK, test_lock_unlock );
    RUN_TEST( TEST_GROUP_LOCK, test_lock_unlock_on_query );

    printf( "\n" );
    printf( "--- P2 ----------\n" );
    RUN_TEST( TEST_GROUP_P2, test_get_capacity );
    RUN_TEST( TEST_GROUP_P2, test_peek_basic );
    RUN_TEST( TEST_GROUP_P2, test_peek_offset_exceed );
    RUN_TEST( TEST_GROUP_P2, test_peek_wrap );
    RUN_TEST( TEST_GROUP_P2, test_drop_basic );
    RUN_TEST( TEST_GROUP_P2, test_drop_exceed );
    RUN_TEST( TEST_GROUP_P2, test_drop_and_read );
    RUN_TEST( TEST_GROUP_P2, test_read_write_byte );
    RUN_TEST( TEST_GROUP_P2, test_write_byte_full );
}

static void print_usage( const char *prog )
{
    printf( "Usage: %s [group ...]\n", prog );
    printf( "\n" );
    printf( "Groups:\n" );
    printf( "  all     run all tests (default)\n" );
    printf( "  p0      init/valid/safety checks\n" );
    printf( "  base    basic read/write/wrap/reset\n" );
    printf( "  lock    lock/unlock callback tests\n" );
    printf( "  p1      alias for lock\n" );
    printf( "  p2      peek/drop/byte/capacity\n" );
    printf( "  list    show this help\n" );
    printf( "\n" );
    printf( "Examples:\n" );
    printf( "  %s                     run all\n", prog );
    printf( "  %s p0                  run P0 only\n", prog );
    printf( "  %s p0 base             run P0 + base\n", prog );
    printf( "  %s p0 base lock p2     run all\n", prog );
}

int main( int argc, char *argv[] )
{
    if( argc <= 1 )
    {
        g_groups = TEST_GROUP_BASE | TEST_GROUP_LOCK | TEST_GROUP_P0 | TEST_GROUP_P2;
    }
    else
    {
        for( int i = 1; i < argc; i++ )
        {
            if( strcmp( argv[i], "all" ) == 0 )
            {
                g_groups |= ( TEST_GROUP_BASE | TEST_GROUP_LOCK |
                              TEST_GROUP_P0 | TEST_GROUP_P2 );
            }
            else if( strcmp( argv[i], "p0" ) == 0 )
            {
                g_groups |= TEST_GROUP_P0;
            }
            else if( strcmp( argv[i], "base" ) == 0 )
            {
                g_groups |= TEST_GROUP_BASE;
            }
            else if( strcmp( argv[i], "lock" ) == 0 || strcmp( argv[i], "p1" ) == 0 )
            {
                g_groups |= TEST_GROUP_LOCK;
            }
            else if( strcmp( argv[i], "p2" ) == 0 )
            {
                g_groups |= TEST_GROUP_P2;
            }
            else if( strcmp( argv[i], "list" ) == 0 )
            {
                print_usage( argv[0] );
                return 0;
            }
            else
            {
                printf( "unknown group: '%s'\n", argv[i] );
                print_usage( argv[0] );
                return 1;
            }
        }
    }

    run_tests();

    printf( "\n" );
    printf( "========================================\n" );
    printf( "  passed: %d  failed: %d  skipped: %d\n",
            g_passed, g_failed, g_skipped );

    if( g_failed > 0 )
    {
        printf( "  RESULT: FAIL\n" );
    }
    else if( g_skipped > 0 )
    {
        printf( "  RESULT: PASS (partial)\n" );
    }
    else
    {
        printf( "  RESULT: PASS (all)\n" );
    }
    printf( "========================================\n" );

    return g_failed > 0 ? 1 : 0;
}
