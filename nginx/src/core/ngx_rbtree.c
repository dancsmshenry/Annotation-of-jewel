
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * The red-black tree code is based on the algorithm described in
 * the "Introduction to Algorithms" by Cormen, Leiserson and Rivest.
 */


 // 左旋节点，root为根结点，node为被左旋节点，sentinel（感觉是用来判断是否为空的）
static ngx_inline void ngx_rbtree_left_rotate(ngx_rbtree_node_t **root,
    ngx_rbtree_node_t *sentinel, ngx_rbtree_node_t *node);
 // 右旋节点，root为根结点，node为被右旋节点，sentinel（感觉是用来判断是否为空的）
static ngx_inline void ngx_rbtree_right_rotate(ngx_rbtree_node_t **root,
    ngx_rbtree_node_t *sentinel, ngx_rbtree_node_t *node);


void
ngx_rbtree_insert(ngx_rbtree_t *tree, ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  **root, *temp, *sentinel;

    /* a binary tree insert */

    root = &tree->root;
    sentinel = tree->sentinel;

    if (*root == sentinel) {
        node->parent = NULL;
        node->left = sentinel;
        node->right = sentinel;
        ngx_rbt_black(node);
        *root = node;

        return;
    }

    tree->insert(*root, node, sentinel);

    /* re-balance tree */

    while (node != *root && ngx_rbt_is_red(node->parent)) {

        if (node->parent == node->parent->parent->left) {
            temp = node->parent->parent->right;

            if (ngx_rbt_is_red(temp)) {
                ngx_rbt_black(node->parent);
                ngx_rbt_black(temp);
                ngx_rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    ngx_rbtree_left_rotate(root, sentinel, node);
                }

                ngx_rbt_black(node->parent);
                ngx_rbt_red(node->parent->parent);
                ngx_rbtree_right_rotate(root, sentinel, node->parent->parent);
            }

        } else {
            temp = node->parent->parent->left;

            if (ngx_rbt_is_red(temp)) {
                ngx_rbt_black(node->parent);
                ngx_rbt_black(temp);
                ngx_rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    ngx_rbtree_right_rotate(root, sentinel, node);
                }

                ngx_rbt_black(node->parent);
                ngx_rbt_red(node->parent->parent);
                ngx_rbtree_left_rotate(root, sentinel, node->parent->parent);
            }
        }
    }

    ngx_rbt_black(*root);
}


void
ngx_rbtree_insert_value(ngx_rbtree_node_t *temp, ngx_rbtree_node_t *node,
    ngx_rbtree_node_t *sentinel)
{
    ngx_rbtree_node_t  **p;

    for ( ;; ) {

        p = (node->key < temp->key) ? &temp->left : &temp->right;

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}


void
ngx_rbtree_insert_timer_value(ngx_rbtree_node_t *temp, ngx_rbtree_node_t *node,
    ngx_rbtree_node_t *sentinel)
{
    ngx_rbtree_node_t  **p;

    for ( ;; ) {

        /*
         * Timer values
         * 1) are spread in small range, usually several minutes,
         * 2) and overflow each 49 days, if milliseconds are stored in 32 bits.
         * The comparison takes into account that overflow.
         */

        /*  node->key < temp->key */

        p = ((ngx_rbtree_key_int_t) (node->key - temp->key) < 0)
            ? &temp->left : &temp->right;

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}


void
ngx_rbtree_delete(ngx_rbtree_t *tree, ngx_rbtree_node_t *node)
{
    ngx_uint_t           red;
    ngx_rbtree_node_t  **root, *sentinel, *subst, *temp, *w;

    /* a binary tree delete */

    root = &tree->root;
    sentinel = tree->sentinel;

    if (node->left == sentinel) { // 被删除节点的左节点为空
        temp = node->right;
        subst = node;

    } else if (node->right == sentinel) { // 被删除节点的右节点为空（左节点不为空）
        temp = node->left;
        subst = node;

    } else {
        subst = ngx_rbtree_min(node->right, sentinel);

        if (subst->left != sentinel) {
            temp = subst->left;
        } else {
            temp = subst->right;
        }
    }

    if (subst == *root) { // 被删除节点为root节点
        *root = temp; // 那temp就作为根节点
        ngx_rbt_black(temp); // 调整根节点的颜色

        // 将被删除的节点的指针全部置空
        /* DEBUG stuff */
        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;
        node->key = 0;

        return;
    }

    red = ngx_rbt_is_red(subst); // 判断当前节点是否为红色

    if (subst == subst->parent->left) { // subst是父节点的左节点
        subst->parent->left = temp; // 把当前节点的左子树接到自己父亲的左子树上

    } else { // subst是父节点的右节点
        subst->parent->right = temp; // 把当前节点的右子树接到自己父亲的右子树上
    }

    if (subst == node) { // 说明左右节点总有一个是空的

        temp->parent = subst->parent; // 让temp->parent指向node的父节点（注意，才是节点总有一边为空，所以只要处理好temp节点就好了）

    } else { // 说明左右节点都不是空的

        if (subst->parent == node) {
            temp->parent = subst;

        } else {
            temp->parent = subst->parent;
        }

        subst->left = node->left;
        subst->right = node->right;
        subst->parent = node->parent;
        ngx_rbt_copy_color(subst, node);

        if (node == *root) {
            *root = subst;

        } else {
            if (node == node->parent->left) {
                node->parent->left = subst;
            } else {
                node->parent->right = subst;
            }
        }

        if (subst->left != sentinel) {
            subst->left->parent = subst;
        }

        if (subst->right != sentinel) {
            subst->right->parent = subst;
        }
    }

    // 将被删除的节点的指针全部置空
    /* DEBUG stuff */
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->key = 0;

    if (red) { // 如果被删除节点为红色，就不用处理了
        return;
    }

    // 被删除节点为黑色，此时位置全部调整好了，现在要调整节点的颜色
    /* a delete fixup */

    while (temp != *root && ngx_rbt_is_black(temp)) {

        if (temp == temp->parent->left) { // 是父节点的左子树
            w = temp->parent->right;

            if (ngx_rbt_is_red(w)) {
                ngx_rbt_black(w);
                ngx_rbt_red(temp->parent);
                ngx_rbtree_left_rotate(root, sentinel, temp->parent);
                w = temp->parent->right;
            }

            if (ngx_rbt_is_black(w->left) && ngx_rbt_is_black(w->right)) {
                ngx_rbt_red(w);
                temp = temp->parent;

            } else {
                if (ngx_rbt_is_black(w->right)) {
                    ngx_rbt_black(w->left);
                    ngx_rbt_red(w);
                    ngx_rbtree_right_rotate(root, sentinel, w);
                    w = temp->parent->right;
                }

                ngx_rbt_copy_color(w, temp->parent);
                ngx_rbt_black(temp->parent);
                ngx_rbt_black(w->right);
                ngx_rbtree_left_rotate(root, sentinel, temp->parent);
                temp = *root;
            }

        } else { // 是父节点的右子树
            w = temp->parent->left;

            if (ngx_rbt_is_red(w)) { // 如果新的节点是红色
                ngx_rbt_black(w); // 新的节点设为黑色
                ngx_rbt_red(temp->parent); // 父亲节点设为红色 
                ngx_rbtree_right_rotate(root, sentinel, temp->parent); // 右旋父节点
                w = temp->parent->left; // 
            }

            if (ngx_rbt_is_black(w->left) && ngx_rbt_is_black(w->right)) {
                ngx_rbt_red(w);
                temp = temp->parent;

            } else {
                if (ngx_rbt_is_black(w->left)) {
                    ngx_rbt_black(w->right);
                    ngx_rbt_red(w);
                    ngx_rbtree_left_rotate(root, sentinel, w);
                    w = temp->parent->left;
                }

                ngx_rbt_copy_color(w, temp->parent);
                ngx_rbt_black(temp->parent);
                ngx_rbt_black(w->left);
                ngx_rbtree_right_rotate(root, sentinel, temp->parent);
                temp = *root;
            }
        }
    }

    ngx_rbt_black(temp);
}


static ngx_inline void
ngx_rbtree_left_rotate(ngx_rbtree_node_t **root, ngx_rbtree_node_t *sentinel,
    ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  *temp;

    temp = node->right; // 左旋后到node位置的节点（左旋，即逆时针旋转）
    node->right = temp->left; // node会作为temp的左节点，所以temp的左子树会接到node的右子树上

    if (temp->left != sentinel) { // 如果temp的左子树不为空，则让左子树的父亲指针指向node
        temp->left->parent = node;
    }

    temp->parent = node->parent; // 因为temp要变到node的位置，所以要调整temp->parent

    if (node == *root) { // 移动的节点是root，那就改变root的指向，下面同理
        *root = temp;

    } else if (node == node->parent->left) { // 移动节点是父节点的左子树
        node->parent->left = temp;

    } else { // 移动节点是父节点的右子树
        node->parent->right = temp;
    }

    // 调整temp和node节点的指针
    temp->left = node;
    node->parent = temp;
}


static ngx_inline void
ngx_rbtree_right_rotate(ngx_rbtree_node_t **root, ngx_rbtree_node_t *sentinel,
    ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  *temp;

    temp = node->left; // 右旋后到node位置的节点（右旋，即顺时针旋转）
    node->left = temp->right; // node会作为temp的右子树，所以会把temp的右子树接到node的左子树上

    if (temp->right != sentinel) { // 如果temp的右子树不为空，则调整其父指针，令其指向node
        temp->right->parent = node;
    }

    temp->parent = node->parent; // 调整temp->parent，使其指向node->parent

    if (node == *root) { // 移动的节点是root，那就改变root的指向，下面同理
        *root = temp;

    } else if (node == node->parent->right) { // 移动的节点是父节点的右子树
        node->parent->right = temp;

    } else { // 移动的节点是父节点的左子树
        node->parent->left = temp;
    }

    // 调整temp和node节点的指针
    temp->right = node;
    node->parent = temp;
}


// 寻找当前node的下一个节点
ngx_rbtree_node_t *
ngx_rbtree_next(ngx_rbtree_t *tree, ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  *root, *sentinel, *parent;

    sentinel = tree->sentinel;

    if (node->right != sentinel) { // 如果右子树不为空，则直接调用ngx_rbtree_min找右子树的最小值
        return ngx_rbtree_min(node->right, sentinel);
    }

    root = tree->root; // 记录一下根节点

    // 说明此时右子树是空的
    for ( ;; ) {
        parent = node->parent;

        if (node == root) { // 这里可以推论一下，如果一直推导到了root，就说明当前是max了（这里的推论其实是有点绕的）
            return NULL;
        }

        if (node == parent->left) { // 如果当前节点是父节点的左节点的话，那父节点就是下一个节点
            return parent;
        }

        node = parent; // 如果当前节点是父节点的右节点的话，那就要继续往上找
    }
}
