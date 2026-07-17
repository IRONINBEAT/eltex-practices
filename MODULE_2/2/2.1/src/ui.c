#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage.h"
#include "ui.h"

static void autosave(const PhoneBook *pb)
{
    if (pb_save(pb, PB_DEFAULT_FILE) != PB_OK)
        printf("Внимание: не удалось сохранить книгу в %s\n",
               PB_DEFAULT_FILE);
}

/* Возвращает 0 при EOF */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }

    char *nl = strchr(buf, '\n');
    if (nl != NULL) {
        *nl = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ; /* отбросить остаток слишком длинной строки */
    }
    return 1;
}

/* mandatory=1 — повторять запрос, пока не введут непустое значение */
static void prompt_field(const char *title, char *buf, size_t size,
                         int mandatory)
{
    for (;;) {
        printf("%s%s: ", title, mandatory ? " (обязательно)" : "");
        read_line(buf, size);
        if (!mandatory || buf[0] != '\0')
            return;
        printf("Поле обязательно для заполнения!\n");
    }
}

static void prompt_list(const char *title, char list[][LINK_LEN],
                        size_t item_size, size_t max, size_t *count)
{
    char buf[LINK_LEN];

    *count = 0;
    printf("%s (до %zu, пустая строка — закончить):\n", title, max);
    while (*count < max) {
        printf("  %zu) ", *count + 1);
        read_line(buf, sizeof(buf));
        if (buf[0] == '\0')
            break;
        /* у телефонов item_size меньше LINK_LEN — обрезаем по item_size */
        strncpy(&list[0][0] + (*count) * item_size, buf, item_size - 1);
        (&list[0][0] + (*count) * item_size)[item_size - 1] = '\0';
        (*count)++;
    }
}

static void input_contact(Contact *c)
{
    contact_init(c);

    prompt_field("Фамилия", c->surname, NAME_LEN, 1);
    prompt_field("Имя", c->name, NAME_LEN, 1);
    prompt_field("Отчество", c->patronymic, NAME_LEN, 0);
    prompt_field("Место работы", c->workplace, TEXT_LEN, 0);
    prompt_field("Должность", c->position, TEXT_LEN, 0);

    prompt_list("Телефоны", (char (*)[LINK_LEN])c->phones,
                PHONE_LEN, MAX_PHONES, &c->phone_count);
    prompt_list("E-mail", c->emails, LINK_LEN, MAX_EMAILS,
                &c->email_count);
    prompt_list("Соцсети", c->socials, LINK_LEN, MAX_SOCIALS,
                &c->social_count);
    prompt_list("Мессенджеры", c->messengers, LINK_LEN, MAX_MESSENGERS,
                &c->messenger_count);
}

/* Печатает "label: v1 v2 ..." из непустых значений. */
static void print_fields(const char *label, ...)
{
    va_list ap;
    const char *s;
    int printed = 0;

    va_start(ap, label);
    while ((s = va_arg(ap, const char *)) != NULL) {
        if (s[0] == '\0')
            continue;
        if (printed) {
            printf(" %s", s);
        } else {
            printf("%s: %s", label, s);
            printed = 1;
        }
    }
    va_end(ap);

    if (printed)
        printf("\n");
}

static void print_multivalue(const char *title, const char *base,
                             size_t item_size, size_t count)
{
    if (count == 0)
        return;
    printf("%s:\n", title);
    for (size_t i = 0; i < count; i++)
        printf("    %s\n", base + i * item_size);
}

static void print_contact(const Contact *c, size_t number)
{
    printf("--- Контакт #%zu ---\n", number);
    print_fields("ФИО", c->surname, c->name, c->patronymic, NULL);
    print_fields("Место работы", c->workplace, NULL);
    print_fields("Должность", c->position, NULL);

    print_multivalue("Телефоны", &c->phones[0][0],
                     PHONE_LEN, c->phone_count);
    print_multivalue("E-mail", &c->emails[0][0],
                     LINK_LEN, c->email_count);
    print_multivalue("Соцсети", &c->socials[0][0],
                     LINK_LEN, c->social_count);
    print_multivalue("Мессенджеры", &c->messengers[0][0],
                     LINK_LEN, c->messenger_count);
}

static void action_list(const PhoneBook *pb)
{
    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    for (size_t i = 0; i < pb->count; i++)
        print_contact(&pb->items[i], i + 1);
    printf("Всего контактов: %zu\n", pb->count);
}

static void action_add(PhoneBook *pb)
{
    Contact c;

    if (pb->count >= MAX_CONTACTS) {
        printf("Книга заполнена (максимум %d контактов).\n", MAX_CONTACTS);
        return;
    }
    input_contact(&c);
    if (pb_add(pb, &c) == PB_OK) {
        printf("Контакт добавлен.\n");
        autosave(pb);
    } else {
        printf("Ошибка: контакт не добавлен.\n");
    }
}

/* Возвращает 1 и индекс с нуля в *index, 0 — если номер неверный */
static int ask_index(const PhoneBook *pb, size_t *index)
{
    char buf[16];
    long n;

    printf("Введите номер контакта (1-%zu): ", pb->count);
    read_line(buf, sizeof(buf));
    n = strtol(buf, NULL, 10);
    if (n < 1 || (size_t)n > pb->count) {
        printf("Нет контакта с таким номером.\n");
        return 0;
    }
    *index = (size_t)n - 1;
    return 1;
}

/* Пустой ввод оставляет старое значение. Для обязательных полей это
 * гарантирует, что их нельзя очистить при редактировании. */
static void edit_field(const char *title, char *buf, size_t size)
{
    char tmp[TEXT_LEN];

    printf("%s [%s] (Enter — оставить): ",
           title, buf[0] ? buf : "пусто");
    read_line(tmp, sizeof(tmp));
    if (tmp[0] != '\0') {
        strncpy(buf, tmp, size - 1);
        buf[size - 1] = '\0';
    }
}

/* Смещение считается через item_size: у телефонов строка короче,
 * чем LINK_LEN в типе параметра. */
static char *list_item(char list[][LINK_LEN], size_t item_size, size_t i)
{
    return &list[0][0] + i * item_size;
}

static void print_list(const char *title, char list[][LINK_LEN],
                       size_t item_size, size_t count)
{
    printf("%s — текущие значения:\n", title);
    if (count == 0)
        printf("  (пусто)\n");
    for (size_t i = 0; i < count; i++)
        printf("  %zu) %s\n", i + 1, list_item(list, item_size, i));
}

static int ask_list_index(size_t count, size_t *idx)
{
    char buf[16];
    long n;

    printf("Номер значения (1-%zu): ", count);
    read_line(buf, sizeof(buf));
    n = strtol(buf, NULL, 10);
    if (n < 1 || (size_t)n > count) {
        printf("Нет значения с таким номером.\n");
        return 0;
    }
    *idx = (size_t)n - 1;
    return 1;
}

static void list_add(const char *title, char list[][LINK_LEN],
                     size_t item_size, size_t max, size_t *count)
{
    char buf[LINK_LEN];

    if (*count >= max) {
        printf("Достигнут предел (%zu значений).\n", max);
        return;
    }
    printf("Новое значение (%s): ", title);
    read_line(buf, sizeof(buf));
    if (buf[0] == '\0') {
        printf("Пустое значение не добавлено.\n");
        return;
    }
    strncpy(list_item(list, item_size, *count), buf, item_size - 1);
    list_item(list, item_size, *count)[item_size - 1] = '\0';
    (*count)++;
}

/* Порядок оставшихся значений сохраняется */
static void list_delete(char list[][LINK_LEN], size_t item_size,
                        size_t *count, size_t idx)
{
    memmove(list_item(list, item_size, idx),
            list_item(list, item_size, idx + 1),
            (*count - idx - 1) * item_size);
    (*count)--;
}

static void edit_list(const char *title, char list[][LINK_LEN],
                      size_t item_size, size_t max, size_t *count)
{
    char choice[8];
    size_t idx;

    for (;;) {
        printf("\n");
        print_list(title, list, item_size, *count);
        printf("a. Добавить  e. Изменить  d. Удалить  0. Назад\n"
               "Выберите действие: ");

        if (!read_line(choice, sizeof(choice)))
            return;

        switch (choice[0]) {
        case 'a':
        case 'A':
            list_add(title, list, item_size, max, count);
            break;
        case 'e':
        case 'E':
            if (*count == 0) {
                printf("Список пуст — нечего изменять.\n");
                break;
            }
            if (ask_list_index(*count, &idx))
                edit_field("Новое значение",
                           list_item(list, item_size, idx), item_size);
            break;
        case 'd':
        case 'D':
            if (*count == 0) {
                printf("Список пуст — нечего удалять.\n");
                break;
            }
            if (ask_list_index(*count, &idx)) {
                list_delete(list, item_size, count, idx);
                printf("Значение удалено.\n");
            }
            break;
        case '0':
            return;
        default:
            printf("Неизвестное действие.\n");
        }
    }
}

static void action_edit(PhoneBook *pb)
{
    size_t index;
    char choice[8];

    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    if (!ask_index(pb, &index))
        return;

    /* Правки идут в копию, в книгу попадают только по "Сохранить" */
    Contact c = *pb_get(pb, index);

    for (;;) {
        printf("\n--- Редактирование контакта #%zu ---\n"
               "1. Фамилия      [%s]\n"
               "2. Имя          [%s]\n"
               "3. Отчество     [%s]\n"
               "4. Место работы [%s]\n"
               "5. Должность    [%s]\n"
               "6. Телефоны     (%zu шт.)\n"
               "7. E-mail       (%zu шт.)\n"
               "8. Соцсети      (%zu шт.)\n"
               "9. Мессенджеры  (%zu шт.)\n"
               "s. Сохранить и выйти\n"
               "0. Отмена (не сохранять)\n"
               "Выберите поле: ",
               index + 1, c.surname, c.name, c.patronymic,
               c.workplace, c.position, c.phone_count,
               c.email_count, c.social_count, c.messenger_count);

        if (!read_line(choice, sizeof(choice)))
            return;

        switch (choice[0]) {
        case '1': edit_field("Фамилия", c.surname, NAME_LEN);      break;
        case '2': edit_field("Имя", c.name, NAME_LEN);             break;
        case '3': edit_field("Отчество", c.patronymic, NAME_LEN);  break;
        case '4': edit_field("Место работы", c.workplace, TEXT_LEN); break;
        case '5': edit_field("Должность", c.position, TEXT_LEN);   break;
        case '6': edit_list("Телефоны", (char (*)[LINK_LEN])c.phones,
                            PHONE_LEN, MAX_PHONES, &c.phone_count); break;
        case '7': edit_list("E-mail", c.emails, LINK_LEN,
                            MAX_EMAILS, &c.email_count);            break;
        case '8': edit_list("Соцсети", c.socials, LINK_LEN,
                            MAX_SOCIALS, &c.social_count);          break;
        case '9': edit_list("Мессенджеры", c.messengers, LINK_LEN,
                            MAX_MESSENGERS, &c.messenger_count);    break;
        case 's':
        case 'S':
            if (pb_update(pb, index, &c) == PB_OK) {
                printf("Контакт обновлён.\n");
                autosave(pb);
            } else {
                printf("Ошибка: контакт не обновлён.\n");
            }
            return;
        case '0':
            printf("Изменения отменены.\n");
            return;
        default:
            printf("Неизвестный пункт меню.\n");
        }
    }
}

static void action_remove(PhoneBook *pb)
{
    size_t index;
    char buf[8];

    if (pb->count == 0) {
        printf("Телефонная книга пуста.\n");
        return;
    }
    if (!ask_index(pb, &index))
        return;

    print_contact(pb_get(pb, index), index + 1);
    printf("Удалить этот контакт? (y/N): ");
    read_line(buf, sizeof(buf));
    if (buf[0] != 'y' && buf[0] != 'Y') {
        printf("Удаление отменено.\n");
        return;
    }

    if (pb_remove(pb, index) == PB_OK) {
        printf("Контакт удалён.\n");
        autosave(pb);
    } else {
        printf("Ошибка: контакт не удалён.\n");
    }
}

static void action_find(const PhoneBook *pb)
{
    char query[NAME_LEN];
    size_t found[MAX_CONTACTS];
    size_t n;

    printf("Поиск по фамилии/имени: ");
    read_line(query, sizeof(query));
    if (query[0] == '\0')
        return;

    n = pb_find(pb, query, found, MAX_CONTACTS);
    if (n == 0) {
        printf("Ничего не найдено.\n");
        return;
    }
    for (size_t i = 0; i < n; i++)
        print_contact(&pb->items[found[i]], found[i] + 1);
    printf("Найдено контактов: %zu\n", n);
}

void ui_run(PhoneBook *pb)
{
    char choice[8];

    for (;;) {
        printf("\n===== ТЕЛЕФОННАЯ КНИГА =====\n"
               "1. Показать все контакты\n"
               "2. Добавить контакт\n"
               "3. Редактировать контакт\n"
               "4. Удалить контакт\n"
               "5. Поиск\n"
               "0. Выход\n"
               "Выберите пункт: ");

        if (!read_line(choice, sizeof(choice)))
            return;

        switch (choice[0]) {
        case '1': action_list(pb);   break;
        case '2': action_add(pb);    break;
        case '3': action_edit(pb);   break;
        case '4': action_remove(pb); break;
        case '5': action_find(pb);   break;
        case '0': return;
        default:  printf("Неизвестный пункт меню.\n");
        }
    }
}
