#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "log/logs.h"

#define LOG  printf

static int g_pass = 0;
static int g_fail = 0;

#define RUN( name, expect, actual ) \
    do { \
        int _e = (int)(expect); int _a = (int)(actual); \
        printf( "%-42s expect=%d actual=%d %s\n", name, _e, _a, _e==_a?"PASS":"FAIL" ); \
        if( _e == _a ) g_pass++; else g_fail++; \
    } while(0)

#define RUN_STR( name, expect, actual ) \
    do { \
        const char *_e = (expect); const char *_a = (actual); \
        int _ok = ( strcmp( _e, _a ) == 0 ) ? 1 : 0; \
        printf( "%-42s expect=\"%s\" actual=\"%s\" %s\n", name, _e, _a, _ok?"PASS":"FAIL" ); \
        if( _ok ) g_pass++; else g_fail++; \
    } while(0)

/* ==================== 测试基础设施 ==================== */

static char     g_cap_buf[1024];
static uint32_t g_cap_len;
static uint32_t g_cap_flag;
static uint32_t g_write_cnt;

static uint32_t g_lock_cnt;
static uint32_t g_unlock_cnt;

static uint32_t my_write( uint32_t flag, uint8_t *p_data, uint32_t len )
{
    g_write_cnt++;
    g_cap_flag = flag;

    if( len > sizeof( g_cap_buf ) - 1 )
    {
        len = sizeof( g_cap_buf ) - 1;
    }

    memcpy( g_cap_buf, p_data, len );
    g_cap_buf[len] = '\0';
    g_cap_len = len;

    return len;
}

static void my_lock( void )
{
    g_lock_cnt++;
}

static void my_unlock( void )
{
    g_unlock_cnt++;
}

static void test_reset( void )
{
    logs_opt_t opt;

    memset( g_cap_buf, 0, sizeof( g_cap_buf ) );
    g_cap_len   = 0;
    g_cap_flag  = 0;
    g_write_cnt = 0;
    g_lock_cnt  = 0;
    g_unlock_cnt = 0;

    opt.write  = my_write;
    opt.lock   = my_lock;
    opt.unlock = my_unlock;

    logs_init( &opt );
    logs_clear_bitmap( LOGS_ALL_MASK );
    logs_clear_bitmap( LOGS_EN_MASK );
}

/* ==================== 初始化测试 ==================== */

static void test_init( void )
{
    logs_opt_t opt;

    opt.write  = my_write;
    opt.lock   = NULL;
    opt.unlock = NULL;

    RUN( "init null opt", MU_ERR_NULL_POINT, logs_init( NULL ) );

    opt.write = NULL;
    RUN( "init null write", MU_ERR_NULL_POINT, logs_init( &opt ) );

    opt.write = my_write;
    RUN( "init ok", MU_OK, logs_init( &opt ) );
}

/* ==================== 基本输出测试 ==================== */

static void test_output_levels( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    /**< ERROR */
    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "TAG", "err msg" );
    RUN( "output error flag", LOGS_FLAG_ERROR, (int)g_cap_flag );
    RUN_STR( "output error text", "[TAG] err msg", g_cap_buf );

    /**< WARN */
    g_write_cnt = 0;
    logs_warn( LOGS_ALL_MASK, "TAG", "warn msg" );
    RUN( "output warn flag", LOGS_FLAG_WARN, (int)g_cap_flag );
    RUN_STR( "output warn text", "[TAG] warn msg", g_cap_buf );

    /**< INFO */
    g_write_cnt = 0;
    logs_info( LOGS_ALL_MASK, "TAG", "info msg" );
    RUN( "output info flag", LOGS_FLAG_INFO, (int)g_cap_flag );
    RUN_STR( "output info text", "[TAG] info msg", g_cap_buf );

    /**< DEBUG */
    g_write_cnt = 0;
    logs_debug( LOGS_ALL_MASK, "TAG", "dbg msg" );
    RUN( "output debug flag", LOGS_FLAG_DEBUG, (int)g_cap_flag );
    RUN_STR( "output debug text", "[TAG] dbg msg", g_cap_buf );
}

/* ==================== 等级过滤测试 ==================== */

static void test_filter_error( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_ERROR );

    logs_error( LOGS_ALL_MASK, "T", "e" );
    RUN( "filter err on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_warn( LOGS_ALL_MASK, "T", "w" );
    RUN( "filter warn off", 0, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_info( LOGS_ALL_MASK, "T", "i" );
    RUN( "filter info off", 0, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_debug( LOGS_ALL_MASK, "T", "d" );
    RUN( "filter dbg off", 0, (int)g_write_cnt );
}

static void test_filter_warn( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_WARN );

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "e" );
    RUN( "filter w->err on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_warn( LOGS_ALL_MASK, "T", "w" );
    RUN( "filter w->warn on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_info( LOGS_ALL_MASK, "T", "i" );
    RUN( "filter w->info off", 0, (int)g_write_cnt );
}

static void test_filter_debug( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "e" );
    RUN( "filter d->err on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_debug( LOGS_ALL_MASK, "T", "d" );
    RUN( "filter d->dbg on", 1, (int)g_write_cnt );
}

/* ==================== 模块掩码隔离测试 ==================== */

#define MOD_A  ((logs_mask_t)0x02)
#define MOD_B  ((logs_mask_t)0x04)

static void test_mask_isolation( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( MOD_A, LOGS_LEVEL_DEBUG );
    /**< MOD_B 未使能 */

    g_write_cnt = 0;
    logs_error( MOD_A, "A", "msg" );
    RUN( "mask mod_a on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_error( MOD_B, "B", "msg" );
    RUN( "mask mod_b off", 0, (int)g_write_cnt );
}

static void test_mask_independent( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( MOD_A, LOGS_LEVEL_ERROR );
    logs_set_bitmap( MOD_B, LOGS_LEVEL_DEBUG );

    /**< MOD_A 只能输出 ERROR */
    g_write_cnt = 0;
    logs_error( MOD_A, "A", "e" );
    RUN( "mask indep a err", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_debug( MOD_A, "A", "d" );
    RUN( "mask indep a dbg", 0, (int)g_write_cnt );

    /**< MOD_B 可以输出 DEBUG */
    g_write_cnt = 0;
    logs_debug( MOD_B, "B", "d" );
    RUN( "mask indep b dbg", 1, (int)g_write_cnt );
}

#undef MOD_A
#undef MOD_B

/* ==================== 全局使能测试 ==================== */

static void test_global_enable( void )
{
    test_reset();

    /**< 未设置全局使能位 */
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );
    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "msg" );
    RUN( "global off", 0, (int)g_write_cnt );

    /**< 设置全局使能位 */
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "msg" );
    RUN( "global on", 1, (int)g_write_cnt );

    /**< 清除全局使能位 */
    logs_clear_bitmap( LOGS_EN_MASK );
    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "msg" );
    RUN( "global off again", 0, (int)g_write_cnt );
}

/* ==================== bitmap 操作测试 ==================== */

static void test_bitmap_set_debug( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "e" );
    RUN( "bm debug err on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_warn( LOGS_ALL_MASK, "T", "w" );
    RUN( "bm debug warn on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_info( LOGS_ALL_MASK, "T", "i" );
    RUN( "bm debug info on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_debug( LOGS_ALL_MASK, "T", "d" );
    RUN( "bm debug dbg on", 1, (int)g_write_cnt );
}

static void test_bitmap_upgrade( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );

    /**< 先设 ERROR 再升级到 INFO */
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_ERROR );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_INFO );

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "e" );
    RUN( "bm up error on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_warn( LOGS_ALL_MASK, "T", "w" );
    RUN( "bm up warn on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_info( LOGS_ALL_MASK, "T", "i" );
    RUN( "bm up info on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_debug( LOGS_ALL_MASK, "T", "d" );
    RUN( "bm up debug off", 0, (int)g_write_cnt );
}

static void test_bitmap_downgrade( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );

    /**< 先设 DEBUG 再降级到 WARN */
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_WARN );

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "e" );
    RUN( "bm down error on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_warn( LOGS_ALL_MASK, "T", "w" );
    RUN( "bm down warn on", 1, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_info( LOGS_ALL_MASK, "T", "i" );
    RUN( "bm down info off", 0, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_debug( LOGS_ALL_MASK, "T", "d" );
    RUN( "bm down debug off", 0, (int)g_write_cnt );
}

static void test_bitmap_clear( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    logs_clear_bitmap( LOGS_ALL_MASK );

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "e" );
    RUN( "bm clear err off", 0, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_warn( LOGS_ALL_MASK, "T", "w" );
    RUN( "bm clear warn off", 0, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_info( LOGS_ALL_MASK, "T", "i" );
    RUN( "bm clear info off", 0, (int)g_write_cnt );

    g_write_cnt = 0;
    logs_debug( LOGS_ALL_MASK, "T", "d" );
    RUN( "bm clear dbg off", 0, (int)g_write_cnt );
}

/* ==================== 锁配对测试 ==================== */

static void test_lock_pairing( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    g_lock_cnt  = 0;
    g_unlock_cnt = 0;

    logs_error( LOGS_ALL_MASK, "T", "m1" );
    logs_warn( LOGS_ALL_MASK, "T", "m2" );
    logs_info( LOGS_ALL_MASK, "T", "m3" );
    logs_debug( LOGS_ALL_MASK, "T", "m4" );

    RUN( "lock count", 4, (int)g_lock_cnt );
    RUN( "unlock count", 4, (int)g_unlock_cnt );
}

static void test_lock_skip_on_filter( void )
{
    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_ERROR );

    g_lock_cnt   = 0;
    g_unlock_cnt = 0;

    /**< 等级未开，应该跳过 lock/unlock */
    logs_warn( LOGS_ALL_MASK, "T", "msg" );
    logs_info( LOGS_ALL_MASK, "T", "msg" );
    logs_debug( LOGS_ALL_MASK, "T", "msg" );

    RUN( "lock skip count", 0, (int)g_lock_cnt );
    RUN( "unlock skip count", 0, (int)g_unlock_cnt );
}

/* ==================== 虚拟多任务测试 ==================== */

#define TASK_A  ((logs_mask_t)0x02)
#define TASK_B  ((logs_mask_t)0x04)
#define TASK_C  ((logs_mask_t)0x08)

static void test_multi_task( void )
{
    int i;

    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( TASK_A, LOGS_LEVEL_ERROR );
    logs_set_bitmap( TASK_B, LOGS_LEVEL_WARN );
    logs_set_bitmap( TASK_C, LOGS_LEVEL_DEBUG );

    g_lock_cnt   = 0;
    g_unlock_cnt = 0;
    g_write_cnt  = 0;

    for( i = 0; i < 1000; i++ )
    {
        logs_error( TASK_A, "A", "msg_%d", i );
        logs_warn( TASK_B, "B", "msg_%d", i );
        logs_info( TASK_C, "C", "msg_%d", i );
        logs_debug( TASK_C, "C", "dbg_%d", i );
    }

    /**< TASK_A=ERROR only → 1 write per round, TASK_B=WARN → 2, TASK_C=DEBUG → 4 */
    /**< Total: 1+2+4=7 writes per round × 1000 = 7000 */
    RUN( "mt write count", 4000, (int)g_write_cnt );
    RUN( "mt lock pair", true, g_lock_cnt == g_unlock_cnt );
    RUN( "mt lock positive", true, g_lock_cnt > 0 );
}

#undef TASK_A
#undef TASK_B
#undef TASK_C

/* ==================== MAX SIZE 溢出测试 ==================== */

static void test_overflow_tag( void )
{
    char long_tag[520];

    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    memset( long_tag, 'T', sizeof( long_tag ) - 1 );
    long_tag[sizeof( long_tag ) - 1] = '\0';

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, long_tag, "body" );
    RUN( "overflow tag count", 0, (int)g_write_cnt );
    RUN( "overflow tag len max", true, g_cap_len <= LOGS_FRAME_MAX_SIZE );
}

static void test_overflow_body( void )
{
    char long_body[600];

    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    memset( long_body, 'X', sizeof( long_body ) - 1 );
    long_body[sizeof( long_body ) - 1] = '\0';

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "%s", long_body );
    RUN( "overflow body count", 1, (int)g_write_cnt );
    RUN( "overflow body len max", true, g_cap_len <= LOGS_FRAME_MAX_SIZE );
}

static void test_overflow_boundary( void )
{
    char exact_body[512 - 10];  /**< "[T] " = 4, body = 508, total ≈ 512 */

    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    memset( exact_body, 'Y', sizeof( exact_body ) - 1 );
    exact_body[sizeof( exact_body ) - 1] = '\0';

    g_write_cnt = 0;
    logs_error( LOGS_ALL_MASK, "T", "%s", exact_body );
    RUN( "overflow boundary ok", 1, (int)g_write_cnt );
    RUN( "overflow boundary len", true, g_cap_len <= LOGS_FRAME_MAX_SIZE );
}

/* ==================== 高速压力测试 ==================== */

static void test_stress_single( void )
{
    int i;

    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    g_lock_cnt   = 0;
    g_unlock_cnt = 0;

    for( i = 0; i < 10000; i++ )
    {
        logs_error( LOGS_ALL_MASK, "S", "iter_%d", i );
    }

    RUN( "stress single lock pair", true, g_lock_cnt == g_unlock_cnt );
    RUN( "stress single count", 10000, (int)g_write_cnt );
}

static void test_stress_rotate( void )
{
    int i;

    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    g_lock_cnt   = 0;
    g_unlock_cnt = 0;

    for( i = 0; i < 5000; i++ )
    {
        logs_error( LOGS_ALL_MASK, "R", "e_%d", i );
        logs_warn( LOGS_ALL_MASK, "R", "w_%d", i );
        logs_info( LOGS_ALL_MASK, "R", "i_%d", i );
        logs_debug( LOGS_ALL_MASK, "R", "d_%d", i );
    }

    RUN( "stress rotate lock pair", true, g_lock_cnt == g_unlock_cnt );
    RUN( "stress rotate count", 20000, (int)g_write_cnt );
}

static void test_stress_format( void )
{
    int i;

    test_reset();
    logs_set_bitmap( LOGS_EN_MASK, LOGS_LEVEL_ENABLED );
    logs_set_bitmap( LOGS_ALL_MASK, LOGS_LEVEL_DEBUG );

    g_lock_cnt   = 0;
    g_unlock_cnt = 0;

    for( i = 0; i < 5000; i++ )
    {
        logs_error( LOGS_ALL_MASK, "F", "d=%d x=0x%x s=%s",
                    i, i * 3, "hello" );
    }

    RUN( "stress fmt lock pair", true, g_lock_cnt == g_unlock_cnt );
    RUN( "stress fmt count", 5000, (int)g_write_cnt );
    RUN( "stress fmt len ok", true, g_cap_len > 0 );
}

/* ==================== 入口 ==================== */

int main( void )
{
    printf( "\n=== init ===\n" );
    test_init();

    printf( "\n=== output levels ===\n" );
    test_output_levels();

    printf( "\n=== filter error ===\n" );
    test_filter_error();

    printf( "\n=== filter warn ===\n" );
    test_filter_warn();

    printf( "\n=== filter debug ===\n" );
    test_filter_debug();

    printf( "\n=== mask isolation ===\n" );
    test_mask_isolation();
    test_mask_independent();

    printf( "\n=== global enable ===\n" );
    test_global_enable();

    printf( "\n=== bitmap set debug ===\n" );
    test_bitmap_set_debug();

    printf( "\n=== bitmap upgrade ===\n" );
    test_bitmap_upgrade();

    printf( "\n=== bitmap downgrade ===\n" );
    test_bitmap_downgrade();

    printf( "\n=== bitmap clear ===\n" );
    test_bitmap_clear();

    printf( "\n=== lock pairing ===\n" );
    test_lock_pairing();

    printf( "\n=== lock skip on filter ===\n" );
    test_lock_skip_on_filter();

    printf( "\n=== multi task ===\n" );
    test_multi_task();

    printf( "\n=== overflow tag ===\n" );
    test_overflow_tag();

    printf( "\n=== overflow body ===\n" );
    test_overflow_body();

    printf( "\n=== overflow boundary ===\n" );
    test_overflow_boundary();

    printf( "\n=== stress single ===\n" );
    test_stress_single();

    printf( "\n=== stress rotate ===\n" );
    test_stress_rotate();

    printf( "\n=== stress format ===\n" );
    test_stress_format();

    printf( "\n========================================\n" );
    printf( "passed: %d  failed: %d\n", g_pass, g_fail );
    printf( "RESULT: %s\n", g_fail == 0 ? "PASS" : "FAIL" );
    printf( "========================================\n" );

    return g_fail == 0 ? 0 : 1;
}
