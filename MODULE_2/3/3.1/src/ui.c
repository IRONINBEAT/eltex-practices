#include <stdio.h>
#include <string.h>

#include "perms.h"
#include "ui.h"

static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

static void show(perm_t m)
{
    char sym[10], oct[8], bin[16];

    perm_to_symbolic(m, sym);
    perm_to_octal(m, oct);
    perm_to_binary(m, bin);

    printf("  буквенное: %s\n", sym);
    printf("  цифровое:  %s\n", oct);
    printf("  битовое:   %s\n", bin);
    printf("             sst uuu ggg ooo\n");
}

/* Возвращает 1, если права успешно введены */
static int action_input(perm_t *cur)
{
    char buf[64];

    printf("Права (например rw-r--r-- или 644): ");
    if (!read_line(buf, sizeof(buf)))
        return 0;

    if (perm_parse(buf, cur) != 0) {
        printf("Не удалось разобрать \"%s\".\n"
               "Ожидается 9 символов rwx или 1-4 восьмеричные цифры.\n",
               buf);
        return 0;
    }
    show(*cur);
    return 1;
}

/* Возвращает 1, если права успешно прочитаны из файла */
static int action_stat(perm_t *cur)
{
    char path[512];
    char sym[10];
    char type;

    printf("Имя файла: ");
    if (!read_line(path, sizeof(path)) || path[0] == '\0')
        return 0;

    if (perm_from_file(path, cur, &type) != 0) {
        printf("Не удалось прочитать \"%s\": файла нет или нет доступа.\n",
               path);
        return 0;
    }

    perm_to_symbolic(*cur, sym);
    printf("Как в ls -l: %c%s\n", type, sym);
    show(*cur);
    printf("Для сравнения выполните: ls -l \"%s\"\n", path);
    return 1;
}

static void action_modify(perm_t *cur, int has)
{
    char buf[128];

    if (!has) {
        printf("Сначала задайте права: пункт 1 или 2.\n");
        return;
    }
    printf("Команды (например u+x или go-w,a+r): ");
    if (!read_line(buf, sizeof(buf)))
        return;

    if (perm_apply(cur, buf) != 0) {
        printf("Ошибка в \"%s\" — права не изменены.\n"
               "Формат: [ugoa][+-=][rwxst], список через запятую.\n",
               buf);
        return;
    }
    show(*cur);
}

void ui_run(void)
{
    char choice[8];
    perm_t cur = 0;
    int has = 0;

    for (;;) {
        printf("\n===== ПРАВА ДОСТУПА К ФАЙЛУ =====\n"
               "1. Ввести права (буквенно или цифрами)\n"
               "2. Прочитать права файла (stat)\n"
               "3. Изменить права (команды как у chmod)\n"
               "4. Показать текущие права\n"
               "0. Выход\n"
               "Выберите пункт: ");

        if (!read_line(choice, sizeof(choice)))
            return;

        switch (choice[0]) {
        case '1':
            if (action_input(&cur))
                has = 1;
            break;
        case '2':
            if (action_stat(&cur))
                has = 1;
            break;
        case '3':
            action_modify(&cur, has);
            break;
        case '4':
            if (has)
                show(cur);
            else
                printf("Права ещё не заданы: пункт 1 или 2.\n");
            break;
        case '0':
            return;
        default:
            printf("Неизвестный пункт меню.\n");
        }
    }
}
