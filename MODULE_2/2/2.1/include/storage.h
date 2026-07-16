#ifndef STORAGE_H
#define STORAGE_H

#include "phonebook.h"

/*
 * Формат файла книги:
 *
 * [contact]
 * surname=Иванов
 * name=Иван
 * phone=+7-999-111-22-33
 * phone=+7-383-000-00-00
 *
 * Каждый контакт начинается со строки [contact]. Пустые поля не
 * записываются, многозначные — по строке на значение.
 */

#define PB_DEFAULT_FILE "phonebook.txt"

/* Файл перезаписывается целиком */
PbResult pb_save(const PhoneBook *pb, const char *path);

/* Текущее содержимое pb затирается. Отсутствие файла — PB_IO_ERROR
 * (обычная ситуация при первом запуске). */
PbResult pb_load(PhoneBook *pb, const char *path);

#endif /* STORAGE_H */
