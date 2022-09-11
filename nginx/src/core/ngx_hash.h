
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


// hashͰ�Ľṹ
typedef struct {
    void             *value; // ָ��value��ָ�룬Ͱ�����Ԫ���Ǵ����￪ʼ��
    u_short           len; // key�ĳ��ȣ�������ÿ��Ԫ�صĳ���
    u_char            name[1]; // ָ��key�ĵ�һ����ַ��key���ȱ䳤
} ngx_hash_elt_t;


// hash�������ṹ
typedef struct {
    ngx_hash_elt_t  **buckets; // ָ��һ����ngx_hash_elt_tͰ����ɵ����飬ÿ��Ͱ��������ngx_hash_elt_t
    ngx_uint_t        size; // ��ϣ��Ͱ�ĸ�����������ÿ��Ͱ�����Ԫ�ظ���
} ngx_hash_t;


typedef struct {
    ngx_hash_t        hash;
    void             *value;
} ngx_hash_wildcard_t;


typedef struct {
    ngx_str_t         key;
    ngx_uint_t        key_hash;
    void             *value;
} ngx_hash_key_t;


typedef ngx_uint_t (*ngx_hash_key_pt) (u_char *data, size_t len);


typedef struct {
    ngx_hash_t            hash;
    ngx_hash_wildcard_t  *wc_head;
    ngx_hash_wildcard_t  *wc_tail;
} ngx_hash_combined_t;


// hash������ṹ
typedef struct {
    ngx_hash_t       *hash; // ָ��hash����ṹ����hash����Ŵ洢���ݣ�
    ngx_hash_key_pt   key; // һ������ָ�룬����keyɢ�еķ���

    ngx_uint_t        max_size; // ������ж��ٸ�Ͱ
    ngx_uint_t        bucket_size; // Ͱ�Ĵ洢�ռ��С��ÿ��Ͱ�ܴ洢�����ֽڴ�С��Ԫ�أ�

    char             *name; // hash�������
    ngx_pool_t       *pool; // �ڴ��
    ngx_pool_t       *temp_pool; // ��ʱ�ڴ��
} ngx_hash_init_t;


#define NGX_HASH_SMALL            1
#define NGX_HASH_LARGE            2

#define NGX_HASH_LARGE_ASIZE      16384
#define NGX_HASH_LARGE_HSIZE      10007

#define NGX_HASH_WILDCARD_KEY     1
#define NGX_HASH_READONLY_KEY     2


typedef struct {
    ngx_uint_t        hsize;

    ngx_pool_t       *pool;
    ngx_pool_t       *temp_pool;

    ngx_array_t       keys;
    ngx_array_t      *keys_hash;

    ngx_array_t       dns_wc_head;
    ngx_array_t      *dns_wc_head_hash;

    ngx_array_t       dns_wc_tail;
    ngx_array_t      *dns_wc_tail_hash;
} ngx_hash_keys_arrays_t;


typedef struct {
    ngx_uint_t        hash;
    ngx_str_t         key;
    ngx_str_t         value;
    u_char           *lowcase_key;
} ngx_table_elt_t;


// �鿴��ֵkey���Ƿ�������name���޵Ļ�����null���еĻ��������ݿ�ʼ��ָ�룩
void *ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len);
void *ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key,
    u_char *name, size_t len);

// ��ʼ��һ��hash��
ngx_int_t ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);

#define ngx_hash(key, c)   ((ngx_uint_t) key * 31 + c)
ngx_uint_t ngx_hash_key(u_char *data, size_t len);
ngx_uint_t ngx_hash_key_lc(u_char *data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char *dst, u_char *src, size_t n);


ngx_int_t ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key,
    void *value, ngx_uint_t flags);


#endif /* _NGX_HASH_H_INCLUDED_ */
