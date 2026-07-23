#include <stdlib.h>

#include "phonebook.h"
#include "utf8.h"

void pb_init(PhoneBook *pb)
{
    pb->root = NULL;
    pb->count = 0;
    pb->ops_since_balance = 0;
}

static void free_subtree(TreeNode *n)
{
    if (n == NULL)
        return;
    free_subtree(n->left);
    free_subtree(n->right);
    free(n);
}

void pb_free(PhoneBook *pb)
{
    free_subtree(pb->root);
    pb_init(pb);
}

size_t pb_count(const PhoneBook *pb)
{
    return pb->count;
}

int contact_cmp(const Contact *a, const Contact *b)
{
    int r = utf8_casecmp(a->surname, b->surname);
    if (r != 0)
        return r;
    r = utf8_casecmp(a->name, b->name);
    if (r != 0)
        return r;
    return utf8_casecmp(a->patronymic, b->patronymic);
}

/* ---------- обход in-order ---------- */


static void collect_nodes(TreeNode *n, TreeNode **arr, size_t *i)
{
    if (n == NULL)
        return;
    collect_nodes(n->left, arr, i);
    arr[(*i)++] = n;
    collect_nodes(n->right, arr, i);
}

/* Узел по in-order индексу через обход со счётчиком */
static TreeNode *find_by_index(TreeNode *n, size_t target, size_t *pos)
{
    if (n == NULL)
        return NULL;

    TreeNode *found = find_by_index(n->left, target, pos);
    if (found != NULL)
        return found;

    if (*pos == target)
        return n;
    (*pos)++;

    return find_by_index(n->right, target, pos);
}

static TreeNode *node_at(const PhoneBook *pb, size_t index)
{
    size_t pos = 0;

    if (index >= pb->count)
        return NULL;
    return find_by_index(pb->root, index, &pos);
}

/* ---------- балансировка ---------- */

/* Строит идеально сбалансированное дерево из отсортированного массива
 * узлов: середина становится корнем, половины — поддеревьями. */
static TreeNode *build_balanced(TreeNode **arr, long lo, long hi)
{
    if (lo > hi)
        return NULL;

    long mid = lo + (hi - lo) / 2;
    TreeNode *root = arr[mid];

    root->left = build_balanced(arr, lo, mid - 1);
    root->right = build_balanced(arr, mid + 1, hi);
    return root;
}

void pb_balance(PhoneBook *pb)
{
    TreeNode **arr;
    size_t i = 0;

    if (pb->count < 3)
        return;

    arr = malloc(pb->count * sizeof(TreeNode *));
    if (arr == NULL)
        return; 

    collect_nodes(pb->root, arr, &i);
    pb->root = build_balanced(arr, 0, (long)pb->count - 1);
    free(arr);
    pb->ops_since_balance = 0;
}

/* Периодический триггер: вызывается после каждой модификации */
static void maybe_balance(PhoneBook *pb)
{
    if (++pb->ops_since_balance >= PB_BALANCE_INTERVAL)
        pb_balance(pb);
}

/* ---------- вставка ---------- */

/* Вставляет узел в BST по ФИО; равные уходят в правое поддерево */
static TreeNode *insert_node(TreeNode *root, TreeNode *node)
{
    if (root == NULL)
        return node;
    if (contact_cmp(&node->data, &root->data) < 0)
        root->left = insert_node(root->left, node);
    else
        root->right = insert_node(root->right, node);
    return root;
}

static TreeNode *make_node(const Contact *c)
{
    TreeNode *node = malloc(sizeof(TreeNode));

    if (node == NULL)
        return NULL;
    node->data = *c;
    node->left = NULL;
    node->right = NULL;
    return node;
}

PbResult pb_add(PhoneBook *pb, const Contact *c)
{
    TreeNode *node;

    if (!contact_is_valid(c))
        return PB_INVALID;

    node = make_node(c);
    if (node == NULL)
        return PB_NO_MEMORY;

    pb->root = insert_node(pb->root, node);
    pb->count++;
    maybe_balance(pb);
    return PB_OK;
}

/* ---------- удаление ---------- */

/* Родитель target по идентичности указателя (NULL — target это корень).
 * Обычный обход, не по ключу: устойчив к дубликатам ФИО. */
static TreeNode *find_parent(TreeNode *root, const TreeNode *target)
{
    if (root == NULL || root == target)
        return NULL;
    if (root->left == target || root->right == target)
        return root;

    TreeNode *p = find_parent(root->left, target);
    if (p != NULL)
        return p;
    return find_parent(root->right, target);
}

/* Удаляет конкретный узел из дерева (три классических случая BST) */
static void delete_node(PhoneBook *pb, TreeNode *target)
{
    if (target->left != NULL && target->right != NULL) {
        /* два потомка: заменяем данные преемником (наименьший в правом
         * поддереве), затем удаляем узел-преемник — у него нет левого */
        TreeNode *succ_parent = target;
        TreeNode *succ = target->right;

        while (succ->left != NULL) {
            succ_parent = succ;
            succ = succ->left;
        }
        target->data = succ->data;

        if (succ_parent->left == succ)
            succ_parent->left = succ->right;
        else
            succ_parent->right = succ->right;
        free(succ);
    } else {
        /* ноль или один потомок */
        TreeNode *child = target->left ? target->left : target->right;
        TreeNode *parent = find_parent(pb->root, target);

        if (parent == NULL)
            pb->root = child;
        else if (parent->left == target)
            parent->left = child;
        else
            parent->right = child;
        free(target);
    }
    pb->count--;
}

PbResult pb_remove(PhoneBook *pb, size_t index)
{
    TreeNode *node = node_at(pb, index);

    if (node == NULL)
        return PB_NOT_FOUND;

    delete_node(pb, node);
    maybe_balance(pb);
    return PB_OK;
}

PbResult pb_update(PhoneBook *pb, size_t index, const Contact *c)
{
    TreeNode *old = node_at(pb, index);
    TreeNode *node;

    if (old == NULL)
        return PB_NOT_FOUND;
    if (!contact_is_valid(c))
        return PB_INVALID;

    /* Новый узел выделяем ДО удаления старого: при нехватке памяти
     * книга остаётся нетронутой. ФИО могло измениться, поэтому
     * контакт переставляется, а не правится на месте. */
    node = make_node(c);
    if (node == NULL)
        return PB_NO_MEMORY;

    delete_node(pb, old);
    pb->root = insert_node(pb->root, node);
    pb->count++;
    maybe_balance(pb);
    return PB_OK;
}

/* ---------- доступ ---------- */

const Contact *pb_get(const PhoneBook *pb, size_t index)
{
    TreeNode *n = node_at(pb, index);

    return n != NULL ? &n->data : NULL;
}

size_t pb_snapshot(const PhoneBook *pb, const Contact **out, size_t out_size)
{
    TreeNode **arr;
    size_t i = 0;
    size_t n;

    if (pb->count == 0)
        return 0;

    arr = malloc(pb->count * sizeof(TreeNode *));
    if (arr == NULL)
        return 0;

    collect_nodes(pb->root, arr, &i);
    n = pb->count < out_size ? pb->count : out_size;
    for (size_t k = 0; k < n; k++)
        out[k] = &arr[k]->data;
    free(arr);
    return n;
}

/* Обход со счётчиком индекса; собирает совпадения подстроки в ФИО */
static void find_rec(const TreeNode *n, const char *query, size_t *pos,
                     size_t *out, size_t out_size, size_t *found)
{
    if (n == NULL || *found >= out_size)
        return;

    find_rec(n->left, query, pos, out, out_size, found);

    if (*found < out_size &&
        (utf8_casestr(n->data.surname, query) ||
         utf8_casestr(n->data.name, query) ||
         utf8_casestr(n->data.patronymic, query))) {
        out[(*found)++] = *pos;
    }
    (*pos)++;

    find_rec(n->right, query, pos, out, out_size, found);
}

size_t pb_find(const PhoneBook *pb, const char *query,
               size_t *out, size_t out_size)
{
    size_t pos = 0;
    size_t found = 0;

    find_rec(pb->root, query, &pos, out, out_size, &found);
    return found;
}

static size_t height_rec(const TreeNode *n)
{
    if (n == NULL)
        return 0;

    size_t l = height_rec(n->left);
    size_t r = height_rec(n->right);
    return 1 + (l > r ? l : r);
}

size_t pb_height(const PhoneBook *pb)
{
    return height_rec(pb->root);
}

const TreeNode *pb_root(const PhoneBook *pb)
{
    return pb->root;
}
