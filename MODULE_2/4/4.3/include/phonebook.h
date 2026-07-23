#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stddef.h>

#include "contact.h"

/*
 * Телефонная книга на бинарном дереве поиска (BST).
 * Ключ упорядочивания — ФИО (фамилия, имя, отчество; без учёта
 * регистра). Левое поддерево < узел <= правое поддерево.
 */

#define PB_BALANCE_INTERVAL 8

typedef enum {
    PB_OK        = 0,
    PB_NO_MEMORY = -1,
    PB_INVALID   = -2,
    PB_NOT_FOUND = -3,
    PB_IO_ERROR  = -4
} PbResult;

typedef struct TreeNode {
    Contact data;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

typedef struct {
    TreeNode *root;
    size_t    count;
    size_t    ops_since_balance; /* счётчик для периодической балансировки */
} PhoneBook;

void pb_init(PhoneBook *pb);
void pb_free(PhoneBook *pb);
size_t pb_count(const PhoneBook *pb);

/* Порядок: фамилия, имя, отчество (без учёта регистра).  */
int contact_cmp(const Contact *a, const Contact *b);

/* Вставка копии контакта; одинаковые ФИО допускаются */
PbResult pb_add(PhoneBook *pb, const Contact *c);

/* Замена контакта; из-за смены ФИО он может занять другую позицию */
PbResult pb_update(PhoneBook *pb, size_t index, const Contact *c);

PbResult pb_remove(PhoneBook *pb, size_t index);

/* Контакт по in-order индексу (NULL — вне диапазона) */
const Contact *pb_get(const PhoneBook *pb, size_t index);

/* Заполняет out указателями на контакты в алфавитном порядке
 * (не более out_size). Возвращает записанное количество. */
size_t pb_snapshot(const PhoneBook *pb, const Contact **out,
                   size_t out_size);

/* Поиск подстроки query в ФИО (фамилия, имя, отчество; без регистра).
 * Индексы найденных пишутся в out в алфавитном порядке. */
size_t pb_find(const PhoneBook *pb, const char *query,
               size_t *out, size_t out_size);

/* Немедленная балансировка (перестройка дерева). Вызывается
 * автоматически периодически, также доступна и вручную. */
void pb_balance(PhoneBook *pb);

/* Высота дерева: 0 — пусто, 1 — только корень */
size_t pb_height(const PhoneBook *pb);

/* Корень дерева для визуализации (NULL — пусто). Только чтение. */
const TreeNode *pb_root(const PhoneBook *pb);

#endif /* PHONEBOOK_H */
