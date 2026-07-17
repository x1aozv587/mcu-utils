#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "lists.h"

#define LOG  printf

static int g_pass = 0;
static int g_fail = 0;

#define RUN( name, expect, actual ) \
    do { \
        int _e = (int)(expect); int _a = (int)(actual); \
        printf( "%-38s expect=%d actual=%d %s\n", name, _e, _a, _e==_a?"PASS":"FAIL" ); \
        if( _e == _a ) g_pass++; else g_fail++; \
    } while(0)

typedef struct
{
    int           value;
    char          name[8];
    lists_node_t  node;
} test_node_t;

static void print_list( lists_node_t *p_head )
{
    lists_node_t *p;

    LOG( "  list: " );
    lists_foreach( p, p_head )
    {
        test_node_t *p_item = lists_entry( p, test_node_t, node );
        LOG( "[%s:%d] -> ", p_item->name, p_item->value );
    }
    LOG( "head\n" );
}

/**< 链表不变式检查：验证双向链表的完整性 */
static bool check_invariant( lists_node_t *p_head, int expected_count )
{
    lists_node_t *p;
    int count = 0;
    int count_rev = 0;

    /**< 正向遍历计数 */
    lists_foreach( p, p_head )
    {
        count++;
        if( count > expected_count + 10 )
        {
            return false;
        }
    }

    /**< 反向遍历计数 */
    for( p = p_head->pre; p != p_head; p = p->pre )
    {
        count_rev++;
        if( count_rev > expected_count + 10 )
        {
            return false;
        }
    }

    if( count != expected_count )
    {
        return false;
    }

    if( count_rev != expected_count )
    {
        return false;
    }

    return true;
}

/* ==================== 基本测试 ==================== */

static void test_init( void )
{
    lists_t list;

    lists_init( &list );
    RUN( "init empty", true, lists_is_empty( &list.head ) );
    RUN( "init first", true, lists_first( &list.head ) == &list.head );
    RUN( "init last", true, lists_last( &list.head ) == &list.head );
}

static void test_add_tail( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    RUN( "add_tail not empty", false, lists_is_empty( &list.head ) );

    LOG( "after add_tail A B C:\n" );
    print_list( &list.head );
}

static void test_add_tail_order( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };
    int expect[] = { 1, 2, 3 };
    lists_node_t *p;
    int idx = 0;
    bool ok = true;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    lists_foreach( p, &list.head )
    {
        test_node_t *p_item = lists_entry( p, test_node_t, node );
        if( p_item->value != expect[idx] )
        {
            ok = false;
        }
        idx++;
    }

    RUN( "add_tail_order", 3, idx );
    RUN( "add_tail_order correct", true, ok );
}

static void test_add_head( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };

    lists_init( &list );

    lists_add_head( &list.head, &b.node );
    lists_add_head( &list.head, &a.node );

    test_node_t *p_first = lists_entry( lists_first( &list.head ), test_node_t, node );
    RUN( "add_head first", 1, p_first->value );

    LOG( "after add_head A then B:\n" );
    print_list( &list.head );
}

static void test_add_head_order( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };
    int expect[] = { 1, 2, 3 };
    lists_node_t *p;
    int idx = 0;
    bool ok = true;

    lists_init( &list );

    lists_add_head( &list.head, &c.node );
    lists_add_head( &list.head, &b.node );
    lists_add_head( &list.head, &a.node );

    lists_foreach( p, &list.head )
    {
        test_node_t *p_item = lists_entry( p, test_node_t, node );
        if( p_item->value != expect[idx] )
        {
            ok = false;
        }
        idx++;
    }

    RUN( "add_head_order", 3, idx );
    RUN( "add_head_order correct", true, ok );
}

static void test_first_last( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };
    test_node_t *p_first;
    test_node_t *p_last;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    p_first = lists_entry( lists_first( &list.head ), test_node_t, node );
    p_last  = lists_entry( lists_last( &list.head ), test_node_t, node );

    RUN( "first value", 1, p_first->value );
    RUN( "last value", 3, p_last->value );
}

/* ==================== 删除测试 ==================== */

static void test_remove_mid( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    lists_remove( &b.node );

    RUN( "remove mid linked", false, lists_is_linked( &b.node ) );
    RUN( "remove mid count", true, lists_first( &list.head ) == &a.node );

    LOG( "after remove B:\n" );
    print_list( &list.head );
}

static void test_remove_first( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };
    test_node_t *p_first;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    lists_remove( &a.node );

    p_first = lists_entry( lists_first( &list.head ), test_node_t, node );
    RUN( "remove first next", 2, p_first->value );
    RUN( "remove first empty", false, lists_is_empty( &list.head ) );
    RUN( "remove first linked", false, lists_is_linked( &a.node ) );

    LOG( "after remove first A:\n" );
    print_list( &list.head );
}

static void test_remove_last( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };
    test_node_t *p_last;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    lists_remove( &c.node );

    p_last = lists_entry( lists_last( &list.head ), test_node_t, node );
    RUN( "remove last pre", 2, p_last->value );
    RUN( "remove last empty", false, lists_is_empty( &list.head ) );
    RUN( "remove last linked", false, lists_is_linked( &c.node ) );

    LOG( "after remove last C:\n" );
    print_list( &list.head );
}

static void test_remove_sole( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_remove( &a.node );

    RUN( "remove sole empty", true, lists_is_empty( &list.head ) );
    RUN( "remove sole linked", false, lists_is_linked( &a.node ) );
    RUN( "remove sole first", true, lists_first( &list.head ) == &list.head );
    RUN( "remove sole last", true, lists_last( &list.head ) == &list.head );
}

static void test_remove_cleanup( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_remove( &a.node );

    RUN( "remove cleanup next", true, a.node.next == NULL );
    RUN( "remove cleanup pre", true, a.node.pre == NULL );
}

/* ==================== 遍历测试 ==================== */

static void test_foreach( void )
{
    lists_t list;
    test_node_t a = { .value = 10, .name = "X" };
    test_node_t b = { .value = 20, .name = "Y" };
    test_node_t c = { .value = 30, .name = "Z" };
    lists_node_t *p;
    int sum = 0;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    lists_foreach( p, &list.head )
    {
        test_node_t *p_item = lists_entry( p, test_node_t, node );
        sum += p_item->value;
    }

    RUN( "foreach sum", 60, sum );
}

static void test_foreach_empty( void )
{
    lists_t list;
    lists_node_t *p;
    int count = 0;

    lists_init( &list );

    lists_foreach( p, &list.head )
    {
        count++;
    }

    RUN( "foreach empty", 0, count );
}

static void test_foreach_safe_remove( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    test_node_t c = { .value = 3, .name = "C" };
    lists_node_t *p;
    lists_node_t *tmp;
    int count = 0;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    lists_foreach_safe( p, tmp, &list.head )
    {
        lists_remove( p );
        count++;
    }

    RUN( "safe remove count", 3, count );
    RUN( "safe remove empty", true, lists_is_empty( &list.head ) );
}

static void test_foreach_safe_empty( void )
{
    lists_t list;
    lists_node_t *p;
    lists_node_t *tmp;
    int count = 0;

    lists_init( &list );

    lists_foreach_safe( p, tmp, &list.head )
    {
        count++;
    }

    RUN( "foreach safe empty", 0, count );
}

/* ==================== 节点状态测试 ==================== */

static void test_node_init( void )
{
    test_node_t n = { .value = 0, .name = "N" };

    lists_node_init( &n.node );

    RUN( "node init linked", false, lists_is_linked( &n.node ) );
}

static void test_is_linked( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };

    RUN( "is_linked before", false, lists_is_linked( &a.node ) );

    lists_init( &list );
    lists_add_tail( &list.head, &a.node );

    RUN( "is_linked after", true, lists_is_linked( &a.node ) );

    lists_remove( &a.node );
    RUN( "is_linked removed", false, lists_is_linked( &a.node ) );
}

/* ==================== 大量节点测试 ==================== */

static void test_large_nodes( void )
{
    lists_t list;
    test_node_t *p_items;
    lists_node_t *p;
    int i;
    int count = 0;
    int sum = 0;
    int expect_sum = 0;
    bool ok = true;

    #define LARGE_COUNT  1000

    p_items = (test_node_t *)malloc( sizeof( test_node_t ) * LARGE_COUNT );

    lists_init( &list );

    for( i = 0; i < LARGE_COUNT; i++ )
    {
        p_items[i].value = i;
        p_items[i].name[0] = '\0';
        lists_add_tail( &list.head, &p_items[i].node );
    }

    lists_foreach( p, &list.head )
    {
        test_node_t *p_item = lists_entry( p, test_node_t, node );
        sum += p_item->value;

        if( p_item->value != count )
        {
            ok = false;
        }
        count++;
    }

    expect_sum = ( LARGE_COUNT - 1 ) * LARGE_COUNT / 2;

    RUN( "large count", LARGE_COUNT, count );
    RUN( "large order", true, ok );
    RUN( "large sum", expect_sum, sum );
    RUN( "large invariant", true, check_invariant( &list.head, LARGE_COUNT ) );

    free( p_items );

    #undef LARGE_COUNT
}

/* ==================== 重复删除测试 ==================== */

static void test_double_remove( void )
{
    lists_t list;
    test_node_t a = { .value = 1, .name = "A" };
    test_node_t b = { .value = 2, .name = "B" };
    int count = 0;
    lists_node_t *p;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );

    lists_remove( &a.node );
    lists_remove( &a.node );

    lists_foreach( p, &list.head )
    {
        count++;
    }

    RUN( "double remove count", 1, count );
    RUN( "double remove invariant", true, check_invariant( &list.head, 1 ) );
}

/* ==================== 随机压力测试 ==================== */

static void test_stress( void )
{
    lists_t list;
    test_node_t *p_items;
    #define NODE_COUNT   100
    #define OP_COUNT     10000
    int op;
    int i;
    int idx;
    int count = 0;
    int seed = 0x12345678;
    bool inv_ok = true;

    p_items = (test_node_t *)malloc( sizeof( test_node_t ) * NODE_COUNT );

    lists_init( &list );

    for( i = 0; i < NODE_COUNT; i++ )
    {
        p_items[i].value = i;
        p_items[i].name[0] = 0;
        lists_node_init( &p_items[i].node );
    }

    /**< 执行大量随机操作，节点可被反复增删 */
    for( i = 0; i < OP_COUNT; i++ )
    {
        seed = seed * 1103515245 + 12345;
        idx = ( seed >> 4 ) & 0x7FFFFFFF;
        idx = idx % NODE_COUNT;
        op = ( seed >> 16 ) & 0x3;

        switch( op )
        {
            case 0:
            {
                if( lists_is_linked( &p_items[idx].node ) == false )
                {
                    lists_add_tail( &list.head, &p_items[idx].node );
                    count++;
                }
            } break;

            case 1:
            {
                if( lists_is_linked( &p_items[idx].node ) == false )
                {
                    lists_add_head( &list.head, &p_items[idx].node );
                    count++;
                }
            } break;

            case 2:
            {
                if( lists_is_linked( &p_items[idx].node ) == true )
                {
                    lists_remove( &p_items[idx].node );
                    count--;
                }
            } break;

            case 3:
            {
                if( check_invariant( &list.head, count ) == false )
                {
                    inv_ok = false;
                }
            } break;

            default:
            {
            } break;
        }
    }

    RUN( "stress invariant final", true, check_invariant( &list.head, count ) );
    RUN( "stress invariant passed", true, inv_ok );

    free( p_items );

    #undef NODE_COUNT
    #undef OP_COUNT
}

int main( void )
{
    printf( "\n=== init ===\n" );
    test_init();

    printf( "\n=== add tail ===\n" );
    test_add_tail();

    printf( "\n=== add tail order ===\n" );
    test_add_tail_order();

    printf( "\n=== add head ===\n" );
    test_add_head();

    printf( "\n=== add head order ===\n" );
    test_add_head_order();

    printf( "\n=== first / last ===\n" );
    test_first_last();

    printf( "\n=== remove mid ===\n" );
    test_remove_mid();

    printf( "\n=== remove first ===\n" );
    test_remove_first();

    printf( "\n=== remove last ===\n" );
    test_remove_last();

    printf( "\n=== remove sole ===\n" );
    test_remove_sole();

    printf( "\n=== remove cleanup ===\n" );
    test_remove_cleanup();

    printf( "\n=== foreach ===\n" );
    test_foreach();

    printf( "\n=== foreach empty ===\n" );
    test_foreach_empty();

    printf( "\n=== foreach safe ===\n" );
    test_foreach_safe_remove();

    printf( "\n=== foreach safe empty ===\n" );
    test_foreach_safe_empty();

    printf( "\n=== node init ===\n" );
    test_node_init();

    printf( "\n=== is linked ===\n" );
    test_is_linked();

    printf( "\n=== large nodes ===\n" );
    test_large_nodes();

    printf( "\n=== double remove ===\n" );
    test_double_remove();

    printf( "\n=== stress test ===\n" );
    test_stress();

    printf( "\n========================================\n" );
    printf( "passed: %d  failed: %d\n", g_pass, g_fail );
    printf( "RESULT: %s\n", g_fail == 0 ? "PASS" : "FAIL" );
    printf( "========================================\n" );

    return g_fail == 0 ? 0 : 1;
}
