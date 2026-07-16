#include "lists.h"

void lists_init( lists_t *list )
{
    list->head.pre  = &list->head;
    list->head.next = &list->head;
}

void lists_add_tail( lists_node_t *p_head, lists_node_t *p_new )
{
    /**< @note 
     *  对于 `head` 来说 
     *  tail -> next 和 head -> pre 是同一个位置
     */
    p_new->next       = p_head;
    p_new->pre        = p_head->pre;
    p_head->pre->next = p_new;
    p_head->pre       = p_new;
}

void lists_add_head( lists_node_t *p_head, lists_node_t *p_new )
{
    p_new->next = p_head->next;
    p_new->pre  = p_head;

    p_head->next->pre = p_new;
    p_head->next      = p_new;
}

void lists_remove( lists_node_t *p_node )
{
    p_node->pre->next = p_node->next;
    p_node->next->pre = p_node->pre;
    
    /**< p_node->next = p_node->pre = p_node */
    p_node->next = NULL;
    p_node->pre = NULL;
}
