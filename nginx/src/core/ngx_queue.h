
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef _NGX_QUEUE_H_INCLUDED_
#define _NGX_QUEUE_H_INCLUDED_


typedef struct ngx_queue_s  ngx_queue_t;

/**
 * 一个双向链表的结点
 * 既可以作为一个链表中的结点（如果作为链表，那就是一个环形链表，并且有一个哨兵结点）
 * 也可以嵌入到其他的数据结构中，通过offsetof来实现双向链表的泛型
*/
struct ngx_queue_s {
    ngx_queue_t  *prev;//指向前一个链表结点的指针
    ngx_queue_t  *next;//指向后一个链表结点的指针
};


// 初始化双向链表
#define ngx_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


// 判断链表是否为空
#define ngx_queue_empty(h)                                                    \
    (h == (h)->prev)


// 将结点x插入到结点h的后面
#define ngx_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define ngx_queue_insert_after   ngx_queue_insert_head


// 将结点x插入到链表尾部（h的前面，因为这个是一个环形链表）
#define ngx_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


// 返回链表头部
#define ngx_queue_head(h)                                                     \
    (h)->next


// 返回链表尾部
#define ngx_queue_last(h)                                                     \
    (h)->prev


// 返回哨兵结点（当前就是哨兵..）
#define ngx_queue_sentinel(h)                                                 \
    (h)


// 返回下一个结点
#define ngx_queue_next(q)                                                     \
    (q)->next


// 返回前一个结点
#define ngx_queue_prev(q)                                                     \
    (q)->prev


#if (NGX_DEBUG)

#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

// 删除x结点
#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


// 将链表h以q为分割点分为两部分，一部分是以h为哨兵开头的，另一部分是以n为哨兵开头的
#define ngx_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;


// 将链表h和链表n接在一起，得到的链表是以h为哨兵开头的
#define ngx_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;


/**
 * 获取双向链表所在的结构体的首地址
 * q是链表在结构体中的地址，type是结构体，link是链表
 * offsetof的作用是计算出link在type中的偏移量
*/
#define ngx_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


// 返回链表的中心元素（用快慢指针实现的）
ngx_queue_t *ngx_queue_middle(ngx_queue_t *queue);

// 对链表中的元素以cmp的方式从大到小进行排序（插入排序）
void ngx_queue_sort(ngx_queue_t *queue,
    ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *));


#endif /* _NGX_QUEUE_H_INCLUDED_ */
