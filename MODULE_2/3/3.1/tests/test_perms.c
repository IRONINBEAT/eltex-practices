/* Для объявления chmod при сборке с -std=c11 на glibc */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "perms.h"

/* Создаётся и удаляется внутри тестов stat */
#define TEST_FILE "test_perms_tmp.txt"

static int tests_run;
static int tests_failed;

#define ASSERT(cond) do {                                             \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);      \
    }                                                                 \
} while (0)

/* Как ASSERT, но прерывает тест: без выполненной подготовки
 * продолжать проверку бессмысленно. */
#define REQUIRE(cond) do {                                            \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d) — тест прерван\n",                   \
               #cond, __FILE__, __LINE__);                            \
        return;                                                       \
    }                                                                 \
} while (0)

static void test_parse_octal(void)
{
    perm_t m = 0;

    ASSERT(perm_parse_octal("755", &m) == 0 && m == 0755);
    ASSERT(perm_parse_octal("0644", &m) == 0 && m == 0644);
    ASSERT(perm_parse_octal("4755", &m) == 0 && m == 04755);
    ASSERT(perm_parse_octal("7777", &m) == 0 && m == 07777);
    ASSERT(perm_parse_octal("0", &m) == 0 && m == 0);
    ASSERT(perm_parse_octal("000", &m) == 0 && m == 0);

    m = 0123;
    ASSERT(perm_parse_octal("", &m) == -1 && m == 0123);
    ASSERT(perm_parse_octal("8", &m) == -1);
    ASSERT(perm_parse_octal("79", &m) == -1);
    ASSERT(perm_parse_octal("abc", &m) == -1);
    ASSERT(perm_parse_octal("12a", &m) == -1);
    ASSERT(perm_parse_octal("77777", &m) == -1);
    ASSERT(perm_parse_octal("-1", &m) == -1);
    ASSERT(m == 0123); /* при ошибках значение не тронуто */
}

static void test_parse_symbolic(void)
{
    perm_t m = 0;

    ASSERT(perm_parse_symbolic("rwxr-xr-x", &m) == 0 && m == 0755);
    ASSERT(perm_parse_symbolic("rw-r--r--", &m) == 0 && m == 0644);
    ASSERT(perm_parse_symbolic("---------", &m) == 0 && m == 0);
    ASSERT(perm_parse_symbolic("rwxrwxrwx", &m) == 0 && m == 0777);

    /* спецбиты: строчная буква включает и x, заглавная — нет */
    ASSERT(perm_parse_symbolic("rwsr-xr-x", &m) == 0 && m == 04755);
    ASSERT(perm_parse_symbolic("rwSr--r--", &m) == 0 && m == 04644);
    ASSERT(perm_parse_symbolic("rwxr-sr-x", &m) == 0 && m == 02755);
    ASSERT(perm_parse_symbolic("rwxrwxrwt", &m) == 0 && m == 01777);
    ASSERT(perm_parse_symbolic("rwxrwxrwT", &m) == 0 && m == 01776);
    ASSERT(perm_parse_symbolic("rwsr-sr-t", &m) == 0 && m == 07755);

    m = 0123;
    ASSERT(perm_parse_symbolic("rwx", &m) == -1);          /* короче 9 */
    ASSERT(perm_parse_symbolic("rwxr-xr-xx", &m) == -1);   /* длиннее */
    ASSERT(perm_parse_symbolic("wrxr-xr-x", &m) == -1);    /* не на месте */
    ASSERT(perm_parse_symbolic("rwxr-xr-y", &m) == -1);
    ASSERT(perm_parse_symbolic("rwtr-xr-x", &m) == -1);    /* t не у o */
    ASSERT(perm_parse_symbolic("rwxr-xr-s", &m) == -1);    /* s не у u/g */
    ASSERT(m == 0123);
}

static void test_parse_auto(void)
{
    perm_t m = 0;

    ASSERT(perm_parse("644", &m) == 0 && m == 0644);
    ASSERT(perm_parse("rw-r--r--", &m) == 0 && m == 0644);
    ASSERT(perm_parse("", &m) == -1);
    ASSERT(perm_parse("9wx", &m) == -1); /* цифра — значит восьмеричная */
}

static void test_format(void)
{
    char sym[10], oct[8], bin[16];

    perm_to_symbolic(0755, sym);
    ASSERT(strcmp(sym, "rwxr-xr-x") == 0);
    perm_to_symbolic(0644, sym);
    ASSERT(strcmp(sym, "rw-r--r--") == 0);
    perm_to_symbolic(0, sym);
    ASSERT(strcmp(sym, "---------") == 0);
    perm_to_symbolic(04755, sym);
    ASSERT(strcmp(sym, "rwsr-xr-x") == 0);
    perm_to_symbolic(04655, sym);
    ASSERT(strcmp(sym, "rwSr-xr-x") == 0);
    perm_to_symbolic(01777, sym);
    ASSERT(strcmp(sym, "rwxrwxrwt") == 0);
    perm_to_symbolic(01776, sym);
    ASSERT(strcmp(sym, "rwxrwxrwT") == 0);

    perm_to_octal(0755, oct);
    ASSERT(strcmp(oct, "0755") == 0);
    perm_to_octal(04755, oct);
    ASSERT(strcmp(oct, "4755") == 0);
    perm_to_octal(0, oct);
    ASSERT(strcmp(oct, "0000") == 0);

    perm_to_binary(0755, bin);
    ASSERT(strcmp(bin, "000 111 101 101") == 0);
    perm_to_binary(04755, bin);
    ASSERT(strcmp(bin, "100 111 101 101") == 0);
    perm_to_binary(0, bin);
    ASSERT(strcmp(bin, "000 000 000 000") == 0);
    perm_to_binary(07777, bin);
    ASSERT(strcmp(bin, "111 111 111 111") == 0);
}

/* Каждая запись: стартовая маска + команда -> ожидаемая маска */
static void test_apply_basic(void)
{
    perm_t m;

    m = 0644;
    ASSERT(perm_apply(&m, "u+x") == 0 && m == 0744);
    m = 0644;
    ASSERT(perm_apply(&m, "go-r") == 0 && m == 0600);
    m = 0644;
    ASSERT(perm_apply(&m, "a+x") == 0 && m == 0755);
    m = 0644;
    ASSERT(perm_apply(&m, "+x") == 0 && m == 0755); /* пусто = a */
    m = 0777;
    ASSERT(perm_apply(&m, "-w") == 0 && m == 0555);
    m = 0000;
    ASSERT(perm_apply(&m, "ug+rw") == 0 && m == 0660);
    m = 0777;
    ASSERT(perm_apply(&m, "o-rwx") == 0 && m == 0770);

    /* повторное применение ничего не меняет */
    m = 0744;
    ASSERT(perm_apply(&m, "u+x") == 0 && m == 0744);
}

static void test_apply_assign(void)
{
    perm_t m;

    m = 0777;
    ASSERT(perm_apply(&m, "u=rx") == 0 && m == 0577);
    m = 0000;
    ASSERT(perm_apply(&m, "a=rx") == 0 && m == 0555);
    m = 0777;
    ASSERT(perm_apply(&m, "=r") == 0 && m == 0444);
    m = 0640;
    ASSERT(perm_apply(&m, "go=") == 0 && m == 0600); /* пустое = очистка */

    /* '=' сбрасывает спецбит только своей группы */
    m = 04755;
    ASSERT(perm_apply(&m, "u=rwx") == 0 && m == 0755);
    m = 06755;
    ASSERT(perm_apply(&m, "g=rx") == 0 && m == 04755);
    m = 01777;
    ASSERT(perm_apply(&m, "o=rx") == 0 && m == 0775);
    /* ...и не трогает чужие спецбиты */
    m = 04755;
    ASSERT(perm_apply(&m, "g=rx") == 0 && m == 04755);
}

static void test_apply_special_bits(void)
{
    perm_t m;

    m = 0755;
    ASSERT(perm_apply(&m, "u+s") == 0 && m == 04755);
    m = 0755;
    ASSERT(perm_apply(&m, "g+s") == 0 && m == 02755);
    m = 0755;
    ASSERT(perm_apply(&m, "+s") == 0 && m == 06755); /* a: suid и sgid */
    m = 0777;
    ASSERT(perm_apply(&m, "o+t") == 0 && m == 01777);
    m = 0777;
    ASSERT(perm_apply(&m, "+t") == 0 && m == 01777);
    m = 06755;
    ASSERT(perm_apply(&m, "a-s") == 0 && m == 0755);
    m = 01777;
    ASSERT(perm_apply(&m, "-t") == 0 && m == 0777);

    /* s у "остальных" не существует — команда законна, но бита нет */
    m = 0755;
    ASSERT(perm_apply(&m, "o+s") == 0 && m == 0755);
}

static void test_apply_lists_and_errors(void)
{
    perm_t m;

    m = 0644;
    ASSERT(perm_apply(&m, "u+x,go-r") == 0 && m == 0700);
    m = 0000;
    ASSERT(perm_apply(&m, "u=rwx,g=rx,o=x") == 0 && m == 0751);

    /* ошибка в любой части списка — маска не изменяется вовсе */
    m = 0644;
    ASSERT(perm_apply(&m, "u+x,q-w") == -1 && m == 0644);
    ASSERT(perm_apply(&m, "u+y") == -1 && m == 0644);
    ASSERT(perm_apply(&m, "u~x") == -1 && m == 0644);
    ASSERT(perm_apply(&m, "") == -1 && m == 0644);
    ASSERT(perm_apply(&m, "u+x,") == -1 && m == 0644);
    ASSERT(perm_apply(&m, ",u+x") == -1 && m == 0644);
    ASSERT(perm_apply(&m, "ux") == -1 && m == 0644); /* нет операции */
}

static void test_from_file(void)
{
    perm_t m = 0;
    char type = '?';
    FILE *f = fopen(TEST_FILE, "w");

    REQUIRE(f != NULL);
    fputs("test\n", f);
    fclose(f);

    /* выставляем известные права и сверяем со stat */
    REQUIRE(chmod(TEST_FILE, 0640) == 0);
    ASSERT(perm_from_file(TEST_FILE, &m, &type) == 0);
    ASSERT(m == 0640);
    ASSERT(type == '-');

    REQUIRE(chmod(TEST_FILE, 0755) == 0);
    ASSERT(perm_from_file(TEST_FILE, &m, &type) == 0 && m == 0755);

    remove(TEST_FILE);

    /* каталог */
    ASSERT(perm_from_file(".", &m, &type) == 0);
    ASSERT(type == 'd');

    /* несуществующий файл: ошибка, выход не тронут */
    m = 0123;
    type = 'q';
    ASSERT(perm_from_file("нет_такого_файла", &m, &type) == -1);
    ASSERT(m == 0123 && type == 'q');
}

/* Разбор и форматирование согласованы: parse(format(m)) == m */
static void test_roundtrip(void)
{
    char sym[10], oct[8];
    perm_t back = 0;

    static const perm_t cases[] = {
        0, 0644, 0755, 0777, 04755, 02755, 01777, 04644, 07755, 07777
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        perm_to_symbolic(cases[i], sym);
        ASSERT(perm_parse_symbolic(sym, &back) == 0 && back == cases[i]);

        perm_to_octal(cases[i], oct);
        ASSERT(perm_parse_octal(oct, &back) == 0 && back == cases[i]);
    }
}

int main(void)
{
    test_parse_octal();
    test_parse_symbolic();
    test_parse_auto();
    test_format();
    test_apply_basic();
    test_apply_assign();
    test_apply_special_bits();
    test_apply_lists_and_errors();
    test_from_file();
    test_roundtrip();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
