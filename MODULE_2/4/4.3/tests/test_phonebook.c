#include <stdio.h>
#include <string.h>

#include "phonebook.h"
#include "storage.h"
#include "utf8.h"

#define TEST_FILE "test_phonebook_tmp.txt"

static int tests_run;
static int tests_failed;

#define ASSERT(cond) do {                                             \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);      \
    }                                                                 \
} while (0)

#define REQUIRE(cond) do {                                            \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d) — тест прерван\n",                   \
               #cond, __FILE__, __LINE__);                            \
        return;                                                       \
    }                                                                 \
} while (0)

static Contact make_contact(const char *surname, const char *name)
{
    Contact c;

    contact_init(&c);
    strncpy(c.surname, surname, NAME_LEN - 1);
    strncpy(c.name, name, NAME_LEN - 1);
    return c;
}

/* Проверяет, что pb_get(0..count-1) идёт по алфавиту (in-order),
 * то есть дерево остаётся корректным BST. */
static int order_ok(const PhoneBook *pb)
{
    size_t n = pb_count(pb);

    for (size_t i = 1; i < n; i++) {
        if (contact_cmp(pb_get(pb, i - 1), pb_get(pb, i)) > 0)
            return 0;
    }
    return 1;
}

static void test_sorted_access(void)
{
    PhoneBook pb;
    Contact s = make_contact("Сидоров", "Семён");
    Contact i = make_contact("Иванов", "Иван");
    Contact p = make_contact("Петров", "Пётр");

    pb_init(&pb);
    pb_add(&pb, &s);
    pb_add(&pb, &i);
    pb_add(&pb, &p);

    REQUIRE(pb_count(&pb) == 3);
    ASSERT(order_ok(&pb));
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Иванов") == 0);
    ASSERT(strcmp(pb_get(&pb, 1)->surname, "Петров") == 0);
    ASSERT(strcmp(pb_get(&pb, 2)->surname, "Сидоров") == 0);
    ASSERT(pb_get(&pb, 3) == NULL);

    pb_free(&pb);
}

static void test_add_invalid(void)
{
    PhoneBook pb;
    Contact bad;

    pb_init(&pb);
    contact_init(&bad);
    ASSERT(pb_add(&pb, &bad) == PB_INVALID);
    ASSERT(pb_count(&pb) == 0);
    pb_free(&pb);
}

static void test_duplicates(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    pb_init(&pb);
    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb_count(&pb) == 3);
    ASSERT(order_ok(&pb));
    pb_free(&pb);
}

/* Удаление во всех трёх конфигурациях: лист, один потомок, два потомка */
static void test_remove_cases(void)
{
    PhoneBook pb;
    /* строим дерево с известной формой: Г корень, Б слева, Е справа,
     * А/В под Б, Д/Ж под Е */
    const char *names[] = {"Г", "Б", "Е", "А", "В", "Д", "Ж"};

    pb_init(&pb);
    for (int k = 0; k < 7; k++) {
        Contact c = make_contact(names[k], "и");
        pb_add(&pb, &c);
    }
    REQUIRE(pb_count(&pb) == 7);
    ASSERT(order_ok(&pb));

    /* удаляем лист (А, индекс 0) */
    ASSERT(pb_remove(&pb, 0) == PB_OK);
    ASSERT(pb_count(&pb) == 6);
    ASSERT(order_ok(&pb));
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Б") == 0);

    /* удаляем узел с двумя потомками (Е). Его in-order индекс: сейчас
     * Б,В,Г,Д,Е,Ж -> Е это индекс 4 */
    ASSERT(pb_remove(&pb, 4) == PB_OK);
    ASSERT(pb_count(&pb) == 5);
    ASSERT(order_ok(&pb));
    /* Е исчез, порядок Б,В,Г,Д,Ж */
    ASSERT(strcmp(pb_get(&pb, 4)->surname, "Ж") == 0);

    /* удаляем корень до опустошения */
    while (pb_count(&pb) > 0) {
        ASSERT(pb_remove(&pb, 0) == PB_OK);
        ASSERT(order_ok(&pb));
    }
    ASSERT(pb_remove(&pb, 0) == PB_NOT_FOUND);
    pb_free(&pb);
}

static void test_update_moves(void)
{
    PhoneBook pb;
    Contact a = make_contact("Абрамов", "Олег");
    Contact b = make_contact("Иванов", "Иван");
    Contact c = make_contact("Петров", "Пётр");

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);
    pb_add(&pb, &c);

    /* Абрамов -> Яковлев: должен уехать в конец */
    Contact upd = make_contact("Яковлев", "Олег");
    ASSERT(pb_update(&pb, 0, &upd) == PB_OK);
    REQUIRE(pb_count(&pb) == 3);
    ASSERT(order_ok(&pb));
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Иванов") == 0);
    ASSERT(strcmp(pb_get(&pb, 2)->surname, "Яковлев") == 0);

    ASSERT(pb_update(&pb, 9, &upd) == PB_NOT_FOUND);
    Contact bad;
    contact_init(&bad);
    ASSERT(pb_update(&pb, 0, &bad) == PB_INVALID);
    ASSERT(order_ok(&pb));
    pb_free(&pb);
}

/* Ключевой тест: перекошенное дерево балансируется, порядок сохраняется */
static void test_balance(void)
{
    PhoneBook pb;
    char sur[NAME_LEN];

    pb_init(&pb);
    /* добавляем 100 контактов по возрастанию — чистый BST выродился бы
     * в «палку» высотой 100; но периодическая балансировка не даёт */
    for (int k = 0; k < 100; k++) {
        snprintf(sur, sizeof(sur), "Фамилия%03d", k);
        Contact c = make_contact(sur, "Имя");
        pb_add(&pb, &c);
    }
    REQUIRE(pb_count(&pb) == 100);
    ASSERT(order_ok(&pb));

    /* высота не должна быть близка к 100: после балансировки она
     * порядка log2(100) ~ 7; допускаем небольшой запас на вставки
     * после последней автобалансировки */
    ASSERT(pb_height(&pb) <= 20);

    /* явная балансировка доводит до идеальной высоты */
    pb_balance(&pb);
    ASSERT(order_ok(&pb));
    ASSERT(pb_height(&pb) <= 8); /* ceil(log2(101)) = 7 */

    /* порядок и данные не потерялись */
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Фамилия000") == 0);
    ASSERT(strcmp(pb_get(&pb, 99)->surname, "Фамилия099") == 0);

    pb_free(&pb);
}

static void test_balance_small(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    pb_init(&pb);
    /* балансировка пустого и одноузлового дерева безопасна */
    pb_balance(&pb);
    ASSERT(pb_height(&pb) == 0);
    pb_add(&pb, &c);
    pb_balance(&pb);
    ASSERT(pb_height(&pb) == 1);
    pb_free(&pb);
}

static void test_find(void)
{
    PhoneBook pb;
    Contact a = make_contact("Яковлев", "Иван");
    Contact b = make_contact("Иванченко", "Пётр");
    Contact c = make_contact("Петров", "Кирилл");
    size_t out[8];
    size_t n;

    strcpy(c.patronymic, "Иванович");

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);
    pb_add(&pb, &c);
    /* алфавит: Иванченко(0), Петров(1), Яковлев(2) */

    n = pb_find(&pb, "иван", out, 8);
    ASSERT(n == 3);
    ASSERT(out[0] == 0 && out[1] == 1 && out[2] == 2); /* по алфавиту */

    n = pb_find(&pb, "иванович", out, 8);
    ASSERT(n == 1);
    ASSERT(strcmp(pb_get(&pb, out[0])->surname, "Петров") == 0);

    n = pb_find(&pb, "ЯКОВЛЕВ", out, 8);
    ASSERT(n == 1);

    n = pb_find(&pb, "нет", out, 8);
    ASSERT(n == 0);

    n = pb_find(&pb, "иван", out, 2); /* ограничение размера */
    ASSERT(n == 2);

    pb_free(&pb);
}

static void test_snapshot(void)
{
    PhoneBook pb;
    const Contact *snap[8];
    size_t n;

    pb_init(&pb);
    Contact z = make_contact("Яковлев", "Ю");
    Contact a = make_contact("Абрамов", "О");
    pb_add(&pb, &z);
    pb_add(&pb, &a);

    n = pb_snapshot(&pb, snap, 8);
    REQUIRE(n == 2);
    ASSERT(strcmp(snap[0]->surname, "Абрамов") == 0);
    ASSERT(strcmp(snap[1]->surname, "Яковлев") == 0);

    /* усечение по размеру буфера */
    n = pb_snapshot(&pb, snap, 1);
    ASSERT(n == 1);
    ASSERT(strcmp(snap[0]->surname, "Абрамов") == 0);

    pb_free(&pb);
}

static void test_free_reuse(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    pb_init(&pb);
    for (int k = 0; k < 20; k++)
        pb_add(&pb, &c);
    pb_free(&pb);
    ASSERT(pb_count(&pb) == 0);
    ASSERT(pb_height(&pb) == 0);

    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb_count(&pb) == 1);
    pb_free(&pb);
    pb_free(&pb); /* повторно безопасно */
}

static void test_save_load(void)
{
    PhoneBook pb, loaded;
    Contact a = make_contact("Иванов", "Иван");
    Contact b = make_contact("Smith", "John");

    strcpy(a.patronymic, "Иванович");
    strcpy(a.phones[0], "+7-999");
    a.phone_count = 1;

    pb_init(&pb);
    pb_init(&loaded);
    pb_add(&pb, &a);
    pb_add(&pb, &b);

    ASSERT(pb_save(&pb, TEST_FILE) == PB_OK);
    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);

    REQUIRE(pb_count(&loaded) == 2);
    ASSERT(order_ok(&loaded));
    /* латиница раньше кириллицы */
    ASSERT(strcmp(pb_get(&loaded, 0)->surname, "Smith") == 0);
    ASSERT(strcmp(pb_get(&loaded, 1)->patronymic, "Иванович") == 0);
    ASSERT(strcmp(pb_get(&loaded, 1)->phones[0], "+7-999") == 0);

    pb_free(&pb);
    pb_free(&loaded);
    remove(TEST_FILE);
}

static void test_load_sorted_file_balances(void)
{
    PhoneBook loaded;
    FILE *f = fopen(TEST_FILE, "w");
    char line[64];

    REQUIRE(f != NULL);
    /* 50 контактов строго по алфавиту — без балансировки после
     * загрузки дерево было бы «палкой» */
    for (int k = 0; k < 50; k++) {
        snprintf(line, sizeof(line),
                 "[contact]\nsurname=Ф%03d\nname=И\n\n", k);
        fputs(line, f);
    }
    fclose(f);

    pb_init(&loaded);
    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);
    REQUIRE(pb_count(&loaded) == 50);
    ASSERT(order_ok(&loaded));
    ASSERT(pb_height(&loaded) <= 8); /* сбалансировано при загрузке */

    pb_free(&loaded);
    remove(TEST_FILE);
}

int main(void)
{
    test_sorted_access();
    test_add_invalid();
    test_duplicates();
    test_remove_cases();
    test_update_moves();
    test_balance();
    test_balance_small();
    test_find();
    test_snapshot();
    test_free_reuse();
    test_save_load();
    test_load_sorted_file_balances();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
