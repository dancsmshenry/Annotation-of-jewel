
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

// ��������ռ�Ļص�������ÿ�������˿ռ��Ժ󣬶������һ�����ֽṹ�壬ָ������Ŀռ估������Ļص�������
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler; // ��������ռ�Ļص�����
    void                 *data; // ָ��洢�����ݵĵ�ַ�������ŵ�����Ӧ����ngx_pool_cleanup_file_t��������ͨ���Ա�fd����ȷ���Ƿ�Ҫ���õ�ǰ�Ļص�����
    ngx_pool_cleanup_t   *next; // ��һ��ngx_pool_cleanup_t
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;

// ָ������ݿ�Ľṹ
struct ngx_pool_large_s {
    ngx_pool_large_t     *next; // ָ����һ���洢��ַ��ͨ�������ַ����֪����ǰ�鳤��
    void                 *alloc; // ���ݿ�ָ���ַ
};


// �ڴ�صľ������
typedef struct {
    u_char               *last; // �ڴ����δʹ���ڴ�Ŀ�ʼ�ڵ��ַ
    u_char               *end; // �ڴ�صĽ�����ַ
    ngx_pool_t           *next; // ָ����һ���ڴ��
    ngx_uint_t            failed; // ʧ�ܴ���
} ngx_pool_data_t;


// �ڴ�ص����ݽṹ��Ҫ������㹻����Ϊ���е����ݽṹ�����������湹���ģ�
struct ngx_pool_s {
    ngx_pool_data_t       d; // �ڴ�ص���������
    size_t                max; // ��ǰ�ɷ��������ڴ棨��ʣ�µ��ڴ��С��
    ngx_pool_t           *current; // ָ��ǰ���ڴ��ָ���ַ��������init����Ȼ��ָ���Լ���һ��ָ��...�����������濴���ˣ�����ڵ㻹��ָ���ӽڵ��pool�ģ���ngx_palloc_block������
    ngx_chain_t          *chain; // ����������
    ngx_pool_large_t     *large; // �洢�����ݵ�����
    ngx_pool_cleanup_t   *cleanup; // ���Զ���ص�����������ڴ�������ڴ�
    ngx_log_t            *log; //  ��־
};


typedef struct {
    ngx_fd_t              fd; // fd�¼�id
    u_char               *name;
    ngx_log_t            *log; // ��־
} ngx_pool_cleanup_file_t;


// �����ڴ��
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);

// �����ڴ��
void ngx_destroy_pool(ngx_pool_t *pool);

// �����ڴ��
void ngx_reset_pool(ngx_pool_t *pool);

// ����һ��size��С���ڴ�ռ䣨���ڴ���룩
void *ngx_palloc(ngx_pool_t *pool, size_t size);

// ����һ��size��С���ڴ�ռ䣨���ڴ���룩
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);

// ��ӵ���ngx_palloc��ͬʱ������õ����ڴ�����
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);

// ֱ������һ��large�ڴ��ӵ�large�����ͷ�����������ж���ģ�
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);

// �����ͷŴ��ڴ�������е�ÿһ�����ڴ��
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


// �´���һ��cleanup_t�Ŀ�ӵ�cleanup�����ͷ��
ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);

// ���cleanup��������fd�¼��Ļص�����
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);

// �ر�fd�¼���ʵ���ϣ����ﴫ�����һ��ngx_pool_cleanup_file_t���ͣ�
void ngx_pool_cleanup_file(void *data);

// �ر�fd�¼���ɾ��file�������Ҳ��һ��ngx_pool_cleanup_file_t���ͣ�
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
