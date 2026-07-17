# lists — 通用双向循环链表

## 0 修改记录

| 日期 | 版本 | 内容 |
|------|------|------|
| 2026-07-16 | v1.1 | `lists_remove` 增加空节点保护；补充完整测试用例（42 项）；新增 pc_test 构建目录 |
| 2026-07-10 | v1.0 | 初始版本，Linux 内核风格侵入式链表 |

## 1 简介

`lists` 是面向 MCU 的侵入式双向循环链表库，参考 Linux kernel `list_head` 设计。

特点：
- 节点嵌入用户结构体，零额外内存分配
- 双向循环链表，支持头/尾插入、删除、正反向遍历
- `lists_foreach_safe` 支持遍历中安全删除当前节点
- `lists_entry` 宏从节点指针恢复容器结构体指针
- 全部 `static inline` 辅助函数零调用开销
- `lists_remove` 自动判空，可安全对未链接节点调用

## 2 文件清单

```
list/
  lists.h               # 接口声明、结构体定义、宏、内联辅助函数
  lists.c               # 实现（init / add_head / add_tail / remove）
  test_lists.c          # 测试（42 项，含压力测试）
  pc_test/              # PC 端构建目录
    Makefile            # 编译脚本（支持 make / make test / make clean）
    tmp_build/          # 编译输出目录
  README.md             # 本文件
```

## 3 数据结构

```c
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
```

**设计说明**：
- `lists_node_t` 嵌入用户结构体中，作为链表"钩子"
- `lists_t.head` 是哨兵节点，其 `pre` 指向尾节点，`next` 指向首节点
- 空链表时 `head.pre == &head` 且 `head.next == &head`

## 4 API 参考

### 4.1 链表操作

| 函数 | 说明 |
|------|------|
| `lists_init(p_list)` | 初始化链表，哨兵节点前后指向自身 |
| `lists_add_head(p_head, p_new)` | 将 `p_new` 插入到 `p_head` 之后（头部） |
| `lists_add_tail(p_head, p_new)` | 将 `p_new` 插入到 `p_head` 之前（尾部） |
| `lists_remove(p_node)` | 从链表中移除节点，已移除的节点重复调用安全 |

### 4.2 遍历宏

| 宏 | 说明 |
|------|------|
| `lists_foreach(p_pos, p_head)` | 正向遍历链表，`p_pos` 为 `lists_node_t *` 游标 |
| `lists_foreach_safe(p_pos, p_tmp, p_head)` | 安全遍历，允许在循环体内删除 `p_pos` |

### 4.3 容器转换

| 宏 | 说明 |
|------|------|
| `lists_entry(p_node, type, member)` | 从节点指针获取包含它的结构体指针 |

### 4.4 状态查询

| 函数 | 说明 |
|------|------|
| `lists_is_empty(p_head)` | 判断链表是否为空 |
| `lists_first(p_head)` | 获取第一个节点，空链表返回哨兵节点 |
| `lists_last(p_head)` | 获取最后一个节点，空链表返回哨兵节点 |
| `lists_is_linked(p_node)` | 判断节点是否在链表中（`next != NULL`） |
| `lists_node_init(p_node)` | 初始化节点为未链接状态（`next = pre = NULL`） |

## 5 使用例程

### 5.1 基本增删遍历

```c
#include "lists.h"

typedef struct
{
    int           id;
    lists_node_t  node;
} my_item_t;

void example( void )
{
    lists_t list;
    my_item_t a = { .id = 1 };
    my_item_t b = { .id = 2 };
    my_item_t c = { .id = 3 };
    lists_node_t *p;

    lists_init( &list );

    lists_add_tail( &list.head, &a.node );
    lists_add_tail( &list.head, &b.node );
    lists_add_tail( &list.head, &c.node );

    /**< 遍历 */
    lists_foreach( p, &list.head )
    {
        my_item_t *p_item = lists_entry( p, my_item_t, node );
        printf( "id = %d\n", p_item->id );
    }

    /**< 删除中间节点 */
    lists_remove( &b.node );
}
```

### 5.2 遍历中安全删除

```c
lists_node_t *p;
lists_node_t *tmp;

lists_foreach_safe( p, tmp, &list.head )
{
    my_item_t *p_item = lists_entry( p, my_item_t, node );

    if( p_item->id == 2 )
    {
        lists_remove( p );
    }
}
```

### 5.3 头插入构建栈序

```c
my_item_t items[3];

lists_init( &list );
for( int i = 0; i < 3; i++ )
{
    lists_add_head( &list.head, &items[i].node );
}
// 遍历顺序: items[2] -> items[1] -> items[0]
```

## 6 注意事项

1. **节点生命周期**：链表不负责节点的内存管理，节点由调用方分配（栈/静态/动态均可）
2. **哨兵节点判断**：空链表时 `lists_first` / `lists_last` 返回 `&head` 自身，使用前需判断
3. **lists_entry 安全性**：仅在遍历回调内使用，传入的 `p_node` 必须确实嵌入在对应类型中
4. **遍历中删除**：必须使用 `lists_foreach_safe`，普通 `lists_foreach` 中删除会导致迭代器失效
5. **节点复用**：从链表移除的节点（`next == NULL`）可以重新加入另一个链表，无需额外初始化
