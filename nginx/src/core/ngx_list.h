
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t;

/**
 * 链表结点的定义
 * 与传统传统的定义不同，该链表的每个结点可以存储一定数量的某个元素
 * 只有该结点存储了超过其容量数量的元素，才会申请新的结点继续存储元素
*/
struct ngx_list_part_s {
    void             *elts; // 结点内存的起始位置
    ngx_uint_t        nelts; // 已经使用的元素的个数
    ngx_list_part_t  *next; // 指向下一个链表结点的指针
};


// 链表结构的定义
typedef struct {
    ngx_list_part_t  *last; // 指向最后一个链表结点
    ngx_list_part_t   part; // 第一个链表结点
    size_t            size; // 链表默认每个元素的大小
    ngx_uint_t        nalloc; // 每个结点可以支持多少个元素
    ngx_pool_t       *pool; // 内存池
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

// 初始化链表
static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
