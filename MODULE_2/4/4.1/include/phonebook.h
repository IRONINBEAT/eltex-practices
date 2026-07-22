#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stddef.h>

#include "contact.h"

/*
 * Индекс контакта — его позиция в отсортированном списке (с нуля).
 * Любое изменение книги может сместить индексы соседних контактов.
 */

typedef enum {
    PB_OK        = 0,
    PB_NO_MEMORY = -1, /* malloc не выделил память под узел */
    PB_INVALID   = -2, /* не заполнены обязательные поля */
    PB_NOT_FOUND = -3, /* неверный индекс контакта */
    PB_IO_ERROR  = -4  /* не удалось открыть/записать файл */
} PbResult;

typedef struct PbNode {
    Contact data;
    struct PbNode *next;
    struct PbNode *prev;
} PbNode;

typedef struct {
    PbNode *head;
    PbNode *tail;
    size_t  count;
} PhoneBook;

void pb_init(PhoneBook *pb);

/* Освобождает все узлы; книга снова пуста и пригодна к работе */
void pb_free(PhoneBook *pb);

/* Порядок сортировки: фамилия, имя, отчество (без учёта регистра). */
int contact_cmp(const Contact *a, const Contact *b);

/* Вставляет копию контакта в позицию согласно сортировке.
 * Одинаковые ФИО допускаются. */
PbResult pb_add(PhoneBook *pb, const Contact *c);

/* Заменяет данные контакта; из-за сортировки контакт может
 * переместиться на другую позицию списка */
PbResult pb_update(PhoneBook *pb, size_t index, const Contact *c);

PbResult pb_remove(PhoneBook *pb, size_t index);

/* NULL, если индекс за пределами списка. */
const Contact *pb_get(const PhoneBook *pb, size_t index);

size_t pb_count(const PhoneBook *pb);

/* Первый и последний узлы для обхода (NULL — книга пуста). */
const PbNode *pb_head(const PhoneBook *pb);
const PbNode *pb_tail(const PhoneBook *pb);

/* Поиск подстроки query в ФИО (фамилия, имя, отчество; без учёта
 * регистра). Возвращает количество найденных. */
size_t pb_find(const PhoneBook *pb, const char *query,
               size_t *out, size_t out_size);

#endif /* PHONEBOOK_H */
