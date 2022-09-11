
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
 * �������Ķ���
 * �봫ͳ��ͳ�Ķ��岻ͬ����������ÿ�������Դ洢һ��������ĳ��Ԫ��
 * ֻ�иý��洢�˳���������������Ԫ�أ��Ż������µĽ������洢Ԫ��
*/
struct ngx_list_part_s {
    void             *elts; // ����ڴ����ʼλ��
    ngx_uint_t        nelts; // �Ѿ�ʹ�õ�Ԫ�صĸ���
    ngx_list_part_t  *next; // ָ����һ����������ָ��
};


// �����ṹ�Ķ���
typedef struct {
    ngx_list_part_t  *last; // ָ�����һ���������
    ngx_list_part_t   part; // ��һ���������
    size_t            size; // ����Ĭ��ÿ��Ԫ�صĴ�С
    ngx_uint_t        nalloc; // ÿ��������֧�ֶ��ٸ�Ԫ��
    ngx_pool_t       *pool; // �ڴ��
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

// ��ʼ������
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