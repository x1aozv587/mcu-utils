#include "mu_ringbuf.h"

#include <string.h>

#define RINGBUF_VAILD_BUFF_SIZE     2

/* ==================== 静态函数声明 ==================== */

static bool mu_ringbuf_is_valid( const mu_ringbuf_t *p_rb );
static void mu_ringbuf_lock( mu_ringbuf_t *p_rb );
static void mu_ringbuf_unlock( mu_ringbuf_t *p_rb );
static uint32_t get_count( const mu_ringbuf_t *p_rb );

/* ==================== 静态函数 ==================== */

static bool mu_ringbuf_is_valid( const mu_ringbuf_t *p_rb )
{
    if( p_rb == NULL )
    {
        return false;
    }

    if( p_rb->buffer == NULL )
    {
        return false;
    }

    if( p_rb->size < RINGBUF_VAILD_BUFF_SIZE )
    {
        return false;
    }

    if( p_rb->head >= p_rb->size || p_rb->tail >= p_rb->size )
    {
        return false;
    }

    return true;
}

static void mu_ringbuf_lock( mu_ringbuf_t *p_rb )
{
    if( p_rb->enter_mutex != NULL )
    {
        p_rb->enter_mutex();
    }
}

static void mu_ringbuf_unlock( mu_ringbuf_t *p_rb )
{
    if( p_rb->exit_mutex != NULL )
    {
        p_rb->exit_mutex();
    }
}

static uint32_t get_count( const mu_ringbuf_t *p_rb )
{
    if( p_rb->head >= p_rb->tail )
    {
        return p_rb->head - p_rb->tail;
    }

    return p_rb->size - p_rb->tail + p_rb->head;
}

/* ==================== 对外接口 ==================== */

uint32_t mu_ringbuf_write( mu_ringbuf_t *p_rb, const uint8_t *p_data, uint32_t len )
{
    uint32_t count = 0;
    uint32_t free = 0;
    uint32_t write_len = 0;
    uint32_t part1 = 0;

    if( mu_ringbuf_is_valid( p_rb ) == false || p_data == NULL || len == 0 )
    {
        return 0;
    }

    mu_ringbuf_lock( p_rb );

    count = get_count( p_rb );
    free  = p_rb->size - count - 1;
    write_len = ( len > free ) ? free : len;

    if( write_len > 0 )
    {
        part1 = p_rb->size - p_rb->head;

        if( write_len <= part1 )
        {
            memcpy( p_rb->buffer + p_rb->head, p_data, write_len );
        }
        else
        {
            memcpy( p_rb->buffer + p_rb->head, p_data, part1 );
            memcpy( p_rb->buffer, p_data + part1, write_len - part1 );
        }

        p_rb->head = ( p_rb->head + write_len ) % p_rb->size;
    }

    mu_ringbuf_unlock( p_rb );

    return write_len;
}

uint32_t mu_ringbuf_read( mu_ringbuf_t *p_rb, uint8_t *p_data, uint32_t len )
{
    uint32_t count = 0;
    uint32_t read_len = 0;
    uint32_t part1 = 0;

    if( mu_ringbuf_is_valid( p_rb ) == false || p_data == NULL || len == 0 )
    {
        return 0;
    }

    mu_ringbuf_lock( p_rb );

    count = get_count( p_rb );
    read_len = ( len > count ) ? count : len;

    if( read_len > 0 )
    {
        part1 = p_rb->size - p_rb->tail;

        if( read_len <= part1 )
        {
            memcpy( p_data, p_rb->buffer + p_rb->tail, read_len );
        }
        else
        {
            memcpy( p_data, p_rb->buffer + p_rb->tail, part1 );
            memcpy( p_data + part1, p_rb->buffer, read_len - part1 );
        }

        p_rb->tail = ( p_rb->tail + read_len ) % p_rb->size;
    }

    mu_ringbuf_unlock( p_rb );

    return read_len;
}

void mu_ringbuf_reset( mu_ringbuf_t *p_rb )
{
    if( mu_ringbuf_is_valid( p_rb ) == false )
    {
        return;
    }

    mu_ringbuf_lock( p_rb );
    p_rb->head = 0;
    p_rb->tail = 0;
    mu_ringbuf_unlock( p_rb );
}

uint32_t mu_ringbuf_peek( mu_ringbuf_t *p_rb,
                          uint32_t offset,
                          uint8_t *p_data,
                          uint32_t len )
{
    uint32_t count = 0;
    uint32_t peek_len = 0;
    uint32_t pos = 0;
    uint32_t part1 = 0;

    if( mu_ringbuf_is_valid( p_rb ) == false || p_data == NULL || len == 0 )
    {
        return 0;
    }

    mu_ringbuf_lock( p_rb );

    count = get_count( p_rb );

    if( offset >= count )
    {
        mu_ringbuf_unlock( p_rb );

        return 0;
    }

    peek_len = count - offset;
    peek_len = ( len > peek_len ) ? peek_len : len;

    pos = ( p_rb->tail + offset ) % p_rb->size;
    part1 = p_rb->size - pos;

    if( peek_len <= part1 )
    {
        memcpy( p_data, p_rb->buffer + pos, peek_len );
    }
    else
    {
        memcpy( p_data, p_rb->buffer + pos, part1 );
        memcpy( p_data + part1, p_rb->buffer, peek_len - part1 );
    }

    mu_ringbuf_unlock( p_rb );

    return peek_len;
}

uint32_t mu_ringbuf_drop( mu_ringbuf_t *p_rb, uint32_t len )
{
    uint32_t count = 0;
    uint32_t drop_len = 0;

    if( mu_ringbuf_is_valid( p_rb ) == false || len == 0 )
    {
        return 0;
    }

    mu_ringbuf_lock( p_rb );

    count = get_count( p_rb );
    drop_len = ( len > count ) ? count : len;

    if( drop_len > 0 )
    {
        p_rb->tail = ( p_rb->tail + drop_len ) % p_rb->size;
    }

    mu_ringbuf_unlock( p_rb );

    return drop_len;
}

uint32_t mu_ringbuf_write_byte( mu_ringbuf_t *p_rb, uint8_t data )
{
    return mu_ringbuf_write( p_rb, &data, 1 );
}

uint32_t mu_ringbuf_read_byte( mu_ringbuf_t *p_rb, uint8_t *p_data )
{
    return mu_ringbuf_read( p_rb, p_data, 1 );
}

/* ==================== 初始化 ==================== */

mu_status_t mu_ringbuf_init( mu_ringbuf_t *p_rb,
                             uint8_t *p_buffer,
                             uint32_t size,
                             mu_ringbuf_mutex_fn_t enter_mutex,
                             mu_ringbuf_mutex_fn_t exit_mutex )
{
    if( p_rb == NULL || p_buffer == NULL || size < RINGBUF_VAILD_BUFF_SIZE )
    {
        return MU_ERR_PARAM;
    }

    p_rb->buffer      = p_buffer;
    p_rb->size        = size;
    p_rb->head        = 0;
    p_rb->tail        = 0;
    p_rb->enter_mutex = enter_mutex;
    p_rb->exit_mutex  = exit_mutex;

    return MU_OK;
}

/* ==================== Get 函数 ==================== */

uint32_t mu_ringbuf_get_free( mu_ringbuf_t *p_rb )
{
    uint32_t free = 0;

    if( mu_ringbuf_is_valid( p_rb ) == false )
    {
        return 0;
    }

    mu_ringbuf_lock( p_rb );
    free = p_rb->size - get_count( p_rb ) - 1;
    mu_ringbuf_unlock( p_rb );

    return free;
}

uint32_t mu_ringbuf_get_count( mu_ringbuf_t *p_rb )
{
    uint32_t count = 0;

    if( mu_ringbuf_is_valid( p_rb ) == false )
    {
        return 0;
    }

    mu_ringbuf_lock( p_rb );
    count = get_count( p_rb );
    mu_ringbuf_unlock( p_rb );

    return count;
}

uint32_t mu_ringbuf_get_capacity( mu_ringbuf_t *p_rb )
{
    if( mu_ringbuf_is_valid( p_rb ) == false )
    {
        return 0;
    }

    return p_rb->size - 1;
}

bool mu_ringbuf_is_empty( mu_ringbuf_t *p_rb )
{
    bool result = true;

    if( mu_ringbuf_is_valid( p_rb ) == false )
    {
        return true;
    }

    mu_ringbuf_lock( p_rb );
    result = ( p_rb->head == p_rb->tail );
    mu_ringbuf_unlock( p_rb );

    return result;
}

bool mu_ringbuf_is_full( mu_ringbuf_t *p_rb )
{
    bool result = true;

    if( mu_ringbuf_is_valid( p_rb ) == false )
    {
        return true;
    }

    mu_ringbuf_lock( p_rb );
    result = ( get_count( p_rb ) >= p_rb->size - 1 );
    mu_ringbuf_unlock( p_rb );

    return result;
}