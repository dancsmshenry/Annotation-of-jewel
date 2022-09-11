
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

// 用于清理空间的回调函数（每次申请了空间以后，都会接上一个这种结构体，指向申请的空间及其清理的回调函数）
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler; // 用于清理空间的回调函数
    void                 *data; // 指向存储的数据的地址（这里存放的数据应该是ngx_pool_cleanup_file_t），后续通过对比fd，来确认是否要调用当前的回调函数
    ngx_pool_cleanup_t   *next; // 下一个ngx_pool_cleanup_t
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;

// 指向大数据块的结构
struct ngx_pool_large_s {
    ngx_pool_large_t     *next; // 指向下一个存储地址，通过这个地址可以知道当前块长度
    void                 *alloc; // 数据块指针地址
};


// 内存池的具体参数
typedef struct {
    u_char               *last; // 内存池中未使用内存的开始节点地址
    u_char               *end; // 内存池的结束地址
    ngx_pool_t           *next; // 指向下一个内存池
    ngx_uint_t            failed; // 失败次数
} ngx_pool_data_t;


// 内存池的数据结构（要申请的足够大，因为所有的数据结构都是在这上面构建的）
struct ngx_pool_s {
    ngx_pool_data_t       d; // 内存池的数据区域
    size_t                max; // 当前可分配的最大内存（即剩下的内存大小）
    ngx_pool_t           *current; // 指向当前的内存池指针地址（看了下init，居然是指向自己的一个指针...）（不过后面看到了，这个节点还会指向子节点的pool的，在ngx_palloc_block函数）
    ngx_chain_t          *chain; // 缓冲区链表
    ngx_pool_large_t     *large; // 存储大数据的链表
    ngx_pool_cleanup_t   *cleanup; // 可自定义回调函数，清除内存块分配的内存
    ngx_log_t            *log; //  日志
};


typedef struct {
    ngx_fd_t              fd; // fd事件id
    u_char               *name;
    ngx_log_t            *log; // 日志
} ngx_pool_cleanup_file_t;


// 创建内存池
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);

// 销毁内存池
void ngx_destroy_pool(ngx_pool_t *pool);

// 重设内存池
void ngx_reset_pool(ngx_pool_t *pool);

// 分配一个size大小的内存空间（有内存对齐）
void *ngx_palloc(ngx_pool_t *pool, size_t size);

// 分配一个size大小的内存空间（无内存对齐）
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);

// 间接调用ngx_palloc，同时将申请得到的内存清零
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);

// 直接生成一个large内存块接到large链表的头部（好像是有对齐的）
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);

// 单独释放大内存块链表中的每一个大内存块
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


// 新创建一个cleanup_t的块接到cleanup链表的头部
ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);

// 检查cleanup链表，调用fd事件的回调函数
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);

// 关闭fd事件（实际上，这里传入的是一个ngx_pool_cleanup_file_t类型）
void ngx_pool_cleanup_file(void *data);

// 关闭fd事件，删除file（传入的也是一个ngx_pool_cleanup_file_t类型）
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
