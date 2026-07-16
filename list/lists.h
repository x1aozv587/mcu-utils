/**
 * @file    lists.h
 * @author  lrz
 * @date    2026-07-10
 * @brief   通用链表（侵入式双向循环链表）
 *
 * @note
 * 参考 Linux kernel list_head 设计。
 * 节点嵌入在用户结构体中，通过 lists_entry 反推结构体指针。
 */

#ifndef __LISTS_H__
#define __LISTS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern C {
#endif

/* ==================== 节点与头 ==================== */

/**< 链表节点 */
typedef struct lists_node
{
    struct lists_node *pre;
    struct lists_node *next;
} lists_node_t;

/**< 链表头 */
typedef struct
{
    lists_node_t head;
} lists_t;

/* ==================== 操作 ==================== */

void lists_init( lists_t *p_list );
void lists_add_head( lists_node_t *p_head, lists_node_t *p_new );
void lists_add_tail( lists_node_t *p_head, lists_node_t *p_new );
void lists_remove( lists_node_t *p_node );

/* ==================== 遍历宏 ==================== */

/**
 * @brief 正向遍历
 * @param p_pos  当前节点指针（lists_node_t *）
 * @param p_head 头节点指针（lists_node_t *）
 */
#define lists_foreach( p_pos, p_head )     for( p_pos = (p_head)->next; p_pos != (p_head); p_pos = p_pos->next )

/**
 * @brief 安全正向遍历（允许在循环内删除 p_pos）
 * @param p_pos  当前节点指针（lists_node_t *）
 * @param p_tmp  临时节点指针（lists_node_t *）
 * @param p_head 头节点指针（lists_node_t *）
 */
#define lists_foreach_safe( p_pos, p_tmp, p_head ) \
    for( p_pos = (p_head)->next, \
         p_tmp = p_pos->next; \
         p_pos != (p_head); \
         p_pos = p_tmp, \
         p_tmp = p_pos->next )
         
/**
 * @brief 从节点指针获取父结构体指针
 * @param p_node 节点指针
 * @param type   父结构体类型
 * @param member 节点在父结构体中的成员名
 */
#define lists_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#ifdef __cplusplus
}
#endif

#endif /* __LISTS_H__ */
