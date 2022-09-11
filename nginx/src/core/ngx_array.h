
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_ARRAY_H_INCLUDED_
#define _NGX_ARRAY_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/**
 * 动态数组，能够根据需要自动扩容
*/
typedef struct {
    void        *elts;//指向数组的首地址（因为在扩容后，数组的首地址会发生变化）
    ngx_uint_t   nelts;//数组中已经使用的元素的个数
    size_t       size;//每个元素占用的内存大小
    ngx_uint_t   nalloc;//当前数组中能够容纳元素个数的总大小
    ngx_pool_t  *pool;//内存池
} ngx_array_t;


/**
 * 生成并返回一个动态数组
 * p为内存池，n为数组容量，size为每个元素的大小
*/
ngx_array_t *ngx_array_create(ngx_pool_t *p, ngx_uint_t n, size_t size);

/**
 * 销毁动态数组的数据（这里的销毁只是把内存池的指针偏移回初始的位置）
*/
void ngx_array_destroy(ngx_array_t *a);

// 添加一个元素，返回的是放置这个元素的空间地址
void *ngx_array_push(ngx_array_t *a);

// 添加n个元素，返回的是放置这个元素的空间地址
void *ngx_array_push_n(ngx_array_t *a, ngx_uint_t n);


/**
 * 初始化一个动态数组
 * array是被初始化的数组，pool为内存池，n为数组的容量
 * size为每个元素的大小
*/
static ngx_inline ngx_int_t
ngx_array_init(ngx_array_t *array, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    /*
     * set "array->nelts" before "array->elts", otherwise MSVC thinks
     * that "array->nelts" may be used without having been initialized
     */

    array->nelts = 0;
    array->size = size;
    array->nalloc = n;
    array->pool = pool;

    array->elts = ngx_palloc(pool, n * size);
    if (array->elts == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


#endif /* _NGX_ARRAY_H_INCLUDED_ */
