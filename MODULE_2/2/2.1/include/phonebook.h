#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include "contact.h"

#define MAX_CONTACTS 100

typedef enum {
    PB_OK        = 0,
    PB_FULL      = -1, /* книга заполнена */
    PB_INVALID   = -2, /* не заполнены обязательные поля */
    PB_NOT_FOUND = -3, /* неверный индекс контакта */
    PB_IO_ERROR  = -4  /* не удалось открыть/записать файл */
} PbResult;

typedef struct {
    Contact items[MAX_CONTACTS];
    size_t  count;
} PhoneBook;

void pb_init(PhoneBook *pb);

/* Добавляет копию контакта в конец книги */
PbResult pb_add(PhoneBook *pb, const Contact *c);

PbResult pb_update(PhoneBook *pb, size_t index, const Contact *c);

/* Порядок остальных контактов сохраняется */
PbResult pb_remove(PhoneBook *pb, size_t index);

/* NULL, если индекс за пределами count */
const Contact *pb_get(const PhoneBook *pb, size_t index);

/* Ищет подстроку query в фамилии и имени, индексы найденных пишет
 * в out (не более out_size). Возвращает количество найденных. */
size_t pb_find(const PhoneBook *pb, const char *query,
               size_t *out, size_t out_size);

#endif /* PHONEBOOK_H */
