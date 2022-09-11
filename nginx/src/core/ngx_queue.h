
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
 * һ��˫������Ľ��
 * �ȿ�����Ϊһ�������еĽ�㣨�����Ϊ�����Ǿ���һ����������������һ���ڱ���㣩
 * Ҳ����Ƕ�뵽���������ݽṹ�У�ͨ��offsetof��ʵ��˫������ķ���
*/
struct ngx_queue_s {
    ngx_queue_t  *prev;//ָ��ǰһ���������ָ��
    ngx_queue_t  *next;//ָ���һ���������ָ��
};


// ��ʼ��˫������
#define ngx_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


// �ж������Ƿ�Ϊ��
#define ngx_queue_empty(h)                                                    \
    (h == (h)->prev)


// �����x���뵽���h�ĺ���
#define ngx_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define ngx_queue_insert_after   ngx_queue_insert_head


// �����x���뵽����β����h��ǰ�棬��Ϊ�����һ����������
#define ngx_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


// ��������ͷ��
#define ngx_queue_head(h)                                                     \
    (h)->next


// ��������β��
#define ngx_queue_last(h)                                                     \
    (h)->prev


// �����ڱ���㣨��ǰ�����ڱ�..��
#define ngx_queue_sentinel(h)                                                 \
    (h)


// ������һ�����
#define ngx_queue_next(q)                                                     \
    (q)->next


// ����ǰһ�����
#define ngx_queue_prev(q)                                                     \
    (q)->prev


#if (NGX_DEBUG)

#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

// ɾ��x���
#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


// ������h��qΪ�ָ���Ϊ�����֣�һ��������hΪ�ڱ���ͷ�ģ���һ��������nΪ�ڱ���ͷ��
#define ngx_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;


// ������h������n����һ�𣬵õ�����������hΪ�ڱ���ͷ��
#define ngx_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;


/**
 * ��ȡ˫���������ڵĽṹ����׵�ַ
 * q�������ڽṹ���еĵ�ַ��type�ǽṹ�壬link������
 * offsetof�������Ǽ����link��type�е�ƫ����
*/
#define ngx_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


// �������������Ԫ�أ��ÿ���ָ��ʵ�ֵģ�
ngx_queue_t *ngx_queue_middle(ngx_queue_t *queue);

// �������е�Ԫ����cmp�ķ�ʽ�Ӵ�С�������򣨲�������
void ngx_queue_sort(ngx_queue_t *queue,
    ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *));


#endif /* _NGX_QUEUE_H_INCLUDED_ */
