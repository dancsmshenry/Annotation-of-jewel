
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size,
    ngx_uint_t align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);


ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;

    // ����һ���СΪsize���ڴ�ռ䣬��һ����ڴ����ڴ���ڴ�ص����ݽṹ�Լ������õ����ڴ�ռ�
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(ngx_pool_t); //����ʹ�õ��ڴ����ڴ�غ��濪ʼ�Ŀռ�
    p->d.end = (u_char *) p + size; // �ڴ�صĽ�����ַ
    p->d.next = NULL; // һ��ʼû���ڴ�ص�
    p->d.failed = 0; // ʧ�ܴ���Ĭ��Ϊ0

    size = size - sizeof(ngx_pool_t); // �����ڴ�صĴ�С��ʵ��size - �ڴ�صĴ�С
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p; // ָ��ǰ���ڴ��
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}


void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    // ÿ��cleanup_s�ṹ���ж���ָ������ݼ�������Ļص����������ûص�������������
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (l = pool->large; l; l = l->next) {
        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif

    // ����pool -> large�����������Ĵ������ڴ�飩
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    // ���ڴ�ص�data���������ͷţ��ͷŵ�ǰ��pool����nextѰ����һ���ڴ�ز��ͷţ�
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}


void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    // ����pool -> large ����pool -> large Ϊ�����Ĵ������ڴ�飩
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    // ����ÿ���ڴ�ص�lastָ�룬�ﵽ�����ڴ�ص�Ŀ��
    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL;
}


void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) { // �����С�ڵ�ǰ�ڴ��ʣ����ڴ棬��ֱ�ӷ���
        return ngx_palloc_small(pool, size, 1); // �ڴ����
    }
#endif

    return ngx_palloc_large(pool, size); // ���򣬾�ͨ��palloc_large����
}


void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) { // �����С�ڵ�ǰ�ڴ��ʣ����ڴ棬��ֱ�ӷ���
        return ngx_palloc_small(pool, size, 0); // �ڴ治����
    }
#endif

    return ngx_palloc_large(pool, size); // ���򣬾�ͨ��palloc_large����
}


static ngx_inline void *
ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align) // ���ڴ���з���small�ռ�
{
    u_char      *m;
    ngx_pool_t  *p;

    p = pool->current;

    do {
        m = p->d.last;

        if (align) { //���align��1�Ļ�����Ҫִ�ж������������ʧ�����ڴ棬�������Ч��
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) { // �����ǰ���ڴ���ܹ������㹻�Ŀռ䣬�ͷ��䣬Ȼ�󷵻ص�ַ
            p->d.last = m + size;

            return m;
        }

        p = p->d.next; // ���򣬾Ϳ���һ���ڴ���Ƿ���Է���ռ�

    } while (p);

    return ngx_palloc_block(pool, size); // ˵����ǰ���е��ڴ�ض������Է����ˣ���������һ��pool�ڵ�
}


static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size) // ����һ���µĻ���أ��������ػ������ԭ����pool��next��
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log); // �����µĿ�
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new->d.last = m + size;

    /**
     * ����ӽڵ�pool�ĸ�������4���Ļ����Ͱ�pool->currentָ�����µ��Ǹ��ӽڵ��pool
     * ԭ�򣺷�ֹpool�ϵ��ӽڵ���࣬����ÿ��ngx_pallocѭ���ڴ������ngx_pool_data_t->next��
     * ������һ����ʵ���ڵ���current��λ��
    */
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = new; // ���µ��ڴ�ؽӵ���ǰpool�ĺ���

    return m;
}


static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size) // �ڴ�ز�����Ҫ��large�Ϸ���ռ�
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

    p = ngx_alloc(size, pool->log); // ������һ��size��С���ڴ�
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    /**
     * ȥpool->large�����ϲ�ѯ�Ƿ���NULL��
     * ֻ�����������²�ѯ4��
     * ��Ҫ�жϴ����ݿ��Ƿ��б��ͷŵģ����û����ֻ������
     **/ 
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    /**
     * ���largeû�пռ�����������ڴ棨ÿ��large->alloc������nullptr��������large�Ŀ���������4��
     * �������ڴ��������һ�������ݿ�ṹ���������ngx_large_s��
     * Ȼ������ӵ�ngx_pool_large_t��ͷ��
    */
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    // Ȼ�����������ݿ�ŵ�ngx_pool_large_t�����ͷ���
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) { // ֻ�ͷ����ݣ�alloc�������ͷ����ݽṹ
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ngx_free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    // ���size�д�С���Ͱ�dataָ��һ��size��С�Ŀռ���
    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    // ���´�����cleanup_t��ӵ������ͷ��
    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}


void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}


void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
