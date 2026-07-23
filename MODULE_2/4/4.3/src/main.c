#include <stdio.h>

#include "phonebook.h"
#include "storage.h"
#include "ui.h"

int main(void)
{
    PhoneBook pb;

    pb_init(&pb);
    if (pb_load(&pb, PB_DEFAULT_FILE) == PB_OK)
        printf("Загружено контактов из %s: %zu\n",
               PB_DEFAULT_FILE, pb_count(&pb));
    else
        printf("Файл %s не найден — начинаем с пустой книги.\n",
               PB_DEFAULT_FILE);

    ui_run(&pb);

    /* Дублирует автосохранение из ui.c */
    if (pb_save(&pb, PB_DEFAULT_FILE) != PB_OK)
        printf("Ошибка: не удалось сохранить книгу в %s\n",
               PB_DEFAULT_FILE);

    pb_free(&pb);
    return 0;
}
