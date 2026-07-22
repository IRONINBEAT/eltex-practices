#include <stdlib.h>

#include "phonebook.h"
#include "utf8.h"

void pb_init(PhoneBook *pb)
{
    pb->head = NULL;
    pb->tail = NULL;
    pb->count = 0;
}

void pb_free(PhoneBook *pb)
{
    PbNode *n = pb->head;

    while (n != NULL) {
        PbNode *next = n->next;
        free(n);
        n = next;
    }
    pb_init(pb);
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

/* Узел по индексу (NULL при выходе за границу) */
static PbNode *node_at(const PhoneBook *pb, size_t index)
{
    const PbNode *n;

    if (index >= pb->count)
        return NULL;
    n = pb->head;
    while (index-- > 0)
        n = n->next;
    return (PbNode *)n;
}

/* Первый узел, контакт которого не меньше c (NULL — все меньше) */
static PbNode *find_insert_pos(const PhoneBook *pb, const Contact *c)
{
    const PbNode *n = pb->head;

    while (n != NULL && contact_cmp(&n->data, c) < 0)
        n = n->next;
    return (PbNode *)n;
}

/* Вставляет node перед pos; pos == NULL означает вставку в конец */
static void link_before(PhoneBook *pb, PbNode *pos, PbNode *node)
{
    if (pos == NULL) {
        node->prev = pb->tail;
        node->next = NULL;
        if (pb->tail != NULL)
            pb->tail->next = node;
        else
            pb->head = node;
        pb->tail = node;
    } else {
        node->next = pos;
        node->prev = pos->prev;
        if (pos->prev != NULL)
            pos->prev->next = node;
        else
            pb->head = node;
        pos->prev = node;
    }
    pb->count++;
}

/* Исключает node из списка, не освобождая память */
static void unlink_node(PhoneBook *pb, PbNode *node)
{
    if (node->prev != NULL)
        node->prev->next = node->next;
    else
        pb->head = node->next;

    if (node->next != NULL)
        node->next->prev = node->prev;
    else
        pb->tail = node->prev;

    node->next = NULL;
    node->prev = NULL;
    pb->count--;
}

PbResult pb_add(PhoneBook *pb, const Contact *c)
{
    PbNode *node;

    if (!contact_is_valid(c))
        return PB_INVALID;

    node = malloc(sizeof(PbNode));
    if (node == NULL)
        return PB_NO_MEMORY;

    node->data = *c;
    link_before(pb, find_insert_pos(pb, c), node);
    return PB_OK;
}

PbResult pb_update(PhoneBook *pb, size_t index, const Contact *c)
{
    PbNode *n = node_at(pb, index);

    if (n == NULL)
        return PB_NOT_FOUND;
    if (!contact_is_valid(c))
        return PB_INVALID;

    /* ФИО могло измениться — узел переставляется на новую позицию;
     * память не перевыделяется, узел переиспользуется */
    unlink_node(pb, n);
    n->data = *c;
    link_before(pb, find_insert_pos(pb, c), n);
    return PB_OK;
}

PbResult pb_remove(PhoneBook *pb, size_t index)
{
    PbNode *n = node_at(pb, index);

    if (n == NULL)
        return PB_NOT_FOUND;

    unlink_node(pb, n);
    free(n);
    return PB_OK;
}

const Contact *pb_get(const PhoneBook *pb, size_t index)
{
    const PbNode *n = node_at(pb, index);

    return n != NULL ? &n->data : NULL;
}

size_t pb_count(const PhoneBook *pb)
{
    return pb->count;
}

const PbNode *pb_head(const PhoneBook *pb)
{
    return pb->head;
}

const PbNode *pb_tail(const PhoneBook *pb)
{
    return pb->tail;
}

size_t pb_find(const PhoneBook *pb, const char *query,
               size_t *out, size_t out_size)
{
    size_t idx = 0;
    size_t found = 0;

    for (const PbNode *n = pb->head;
         n != NULL && found < out_size;
         n = n->next, idx++) {
        if (utf8_casestr(n->data.surname, query) ||
            utf8_casestr(n->data.name, query) ||
            utf8_casestr(n->data.patronymic, query)) {
            out[found++] = idx;
        }
    }
    return found;
}
