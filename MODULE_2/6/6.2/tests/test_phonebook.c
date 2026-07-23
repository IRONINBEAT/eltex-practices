#include <stdio.h>
#include <string.h>

#include "phonebook.h"
#include "storage.h"
#include "utf8.h"

/* Создаётся и удаляется внутри тестов хранилища */
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

/* Как ASSERT, но прерывает тест: ставится перед разыменованием
 * указателя, иначе провалившийся тест упал бы вместо отчёта. */
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

/* Инвариант списка: связность в обе стороны, счётчик, сортировка.
 * Возвращает 1, если всё согласовано. */
static int list_ok(const PhoneBook *pb)
{
    size_t forward = 0;
    const PbNode *last = NULL;

    if (pb_head(pb) != NULL && pb_head(pb)->prev != NULL)
        return 0;
    if (pb_tail(pb) != NULL && pb_tail(pb)->next != NULL)
        return 0;

    for (const PbNode *n = pb_head(pb); n != NULL; n = n->next) {
        if (n->prev != last)
            return 0; /* обратная связь не согласована с прямой */
        if (last != NULL && contact_cmp(&last->data, &n->data) > 0)
            return 0; /* нарушен порядок сортировки */
        last = n;
        forward++;
    }
    if (last != pb_tail(pb))
        return 0;
    if (forward != pb_count(pb))
        return 0;

    /* обратный проход должен дать столько же узлов */
    size_t backward = 0;
    for (const PbNode *n = pb_tail(pb); n != NULL; n = n->prev)
        backward++;
    return backward == forward;
}

static void test_utf8_casecmp(void)
{
    ASSERT(utf8_casecmp("Иванов", "иванов") == 0);
    ASSERT(utf8_casecmp("ИВАНОВ", "иванов") == 0);
    ASSERT(utf8_casecmp("Иванов", "Петров") < 0);
    ASSERT(utf8_casecmp("Петров", "Иванов") > 0);
    ASSERT(utf8_casecmp("Иван", "Иванов") < 0);   /* префикс меньше */
    ASSERT(utf8_casecmp("Иванов", "Иван") > 0);
    ASSERT(utf8_casecmp("", "") == 0);
    ASSERT(utf8_casecmp("", "а") < 0);
    ASSERT(utf8_casecmp("Smith", "smith") == 0);
    ASSERT(utf8_casecmp("Абрамов", "Яковлев") < 0);
}

static void test_contact_cmp(void)
{
    Contact a = make_contact("Иванов", "Иван");
    Contact b = make_contact("Иванов", "Пётр");
    Contact c = make_contact("Петров", "Анна");

    ASSERT(contact_cmp(&a, &b) < 0);  /* фамилии равны — решает имя */
    ASSERT(contact_cmp(&b, &c) < 0);
    ASSERT(contact_cmp(&a, &a) == 0);

    /* при равных фамилии и имени решает отчество */
    Contact d = make_contact("Иванов", "Иван");
    strcpy(d.patronymic, "Борисович");
    Contact e = make_contact("Иванов", "Иван");
    strcpy(e.patronymic, "Аркадьевич");
    ASSERT(contact_cmp(&e, &d) < 0);
}

static void test_sorted_insert(void)
{
    PhoneBook pb;
    Contact s = make_contact("Сидоров", "Семён");
    Contact i = make_contact("Иванов", "Иван");
    Contact p = make_contact("Петров", "Пётр");

    pb_init(&pb);
    /* добавляем не по алфавиту */
    ASSERT(pb_add(&pb, &s) == PB_OK);
    ASSERT(pb_add(&pb, &i) == PB_OK);
    ASSERT(pb_add(&pb, &p) == PB_OK);

    REQUIRE(pb_count(&pb) == 3);
    ASSERT(list_ok(&pb));

    /* список выстроился по алфавиту сам */
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Иванов") == 0);
    ASSERT(strcmp(pb_get(&pb, 1)->surname, "Петров") == 0);
    ASSERT(strcmp(pb_get(&pb, 2)->surname, "Сидоров") == 0);

    /* вставка в начало и в конец */
    Contact a = make_contact("Абрамов", "Олег");
    Contact z = make_contact("Яковлев", "Юрий");
    ASSERT(pb_add(&pb, &z) == PB_OK);
    ASSERT(pb_add(&pb, &a) == PB_OK);
    ASSERT(list_ok(&pb));
    ASSERT(strcmp(pb_head(&pb)->data.surname, "Абрамов") == 0);
    ASSERT(strcmp(pb_tail(&pb)->data.surname, "Яковлев") == 0);

    pb_free(&pb);
}

static void test_case_insensitive_order(void)
{
    PhoneBook pb;
    Contact a = make_contact("иванов", "иван");
    Contact b = make_contact("Яковлев", "Юрий");

    pb_init(&pb);
    pb_add(&pb, &b);
    pb_add(&pb, &a);

    /* строчная "иванов" всё равно раньше "Яковлев" */
    REQUIRE(pb_count(&pb) == 2);
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "иванов") == 0);
    ASSERT(list_ok(&pb));
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
    ASSERT(list_ok(&pb));
    pb_free(&pb);
}

static void test_duplicates_allowed(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    pb_init(&pb);
    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb_count(&pb) == 2);
    ASSERT(list_ok(&pb));
    pb_free(&pb);
}

static void test_remove(void)
{
    PhoneBook pb;
    Contact a = make_contact("Абрамов", "Олег");
    Contact b = make_contact("Иванов", "Иван");
    Contact c = make_contact("Яковлев", "Юрий");

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);
    pb_add(&pb, &c);

    /* удаление из середины */
    ASSERT(pb_remove(&pb, 1) == PB_OK);
    REQUIRE(pb_count(&pb) == 2);
    ASSERT(list_ok(&pb));
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Абрамов") == 0);
    ASSERT(strcmp(pb_get(&pb, 1)->surname, "Яковлев") == 0);

    /* удаление головы */
    ASSERT(pb_remove(&pb, 0) == PB_OK);
    REQUIRE(pb_count(&pb) == 1);
    ASSERT(list_ok(&pb));
    ASSERT(strcmp(pb_head(&pb)->data.surname, "Яковлев") == 0);
    ASSERT(pb_head(&pb) == pb_tail(&pb)); /* единственный узел */

    /* удаление последнего узла */
    ASSERT(pb_remove(&pb, 0) == PB_OK);
    ASSERT(pb_count(&pb) == 0);
    ASSERT(pb_head(&pb) == NULL && pb_tail(&pb) == NULL);
    ASSERT(list_ok(&pb));

    /* из пустой книги */
    ASSERT(pb_remove(&pb, 0) == PB_NOT_FOUND);
    pb_free(&pb);
}

static void test_remove_tail(void)
{
    PhoneBook pb;
    Contact a = make_contact("Абрамов", "Олег");
    Contact b = make_contact("Иванов", "Иван");

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);

    ASSERT(pb_remove(&pb, 1) == PB_OK); /* хвост */
    REQUIRE(pb_count(&pb) == 1);
    ASSERT(list_ok(&pb));
    ASSERT(strcmp(pb_tail(&pb)->data.surname, "Абрамов") == 0);
    pb_free(&pb);
}

static void test_update_moves_node(void)
{
    PhoneBook pb;
    Contact a = make_contact("Абрамов", "Олег");
    Contact b = make_contact("Иванов", "Иван");
    Contact c = make_contact("Петров", "Пётр");

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);
    pb_add(&pb, &c);

    /* Абрамов (голова) становится Яковлевым — должен уехать в хвост */
    Contact upd = make_contact("Яковлев", "Олег");
    ASSERT(pb_update(&pb, 0, &upd) == PB_OK);
    REQUIRE(pb_count(&pb) == 3);
    ASSERT(list_ok(&pb));
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Иванов") == 0);
    ASSERT(strcmp(pb_get(&pb, 2)->surname, "Яковлев") == 0);

    /* неверный индекс и невалидные данные */
    ASSERT(pb_update(&pb, 5, &upd) == PB_NOT_FOUND);
    Contact bad;
    contact_init(&bad);
    ASSERT(pb_update(&pb, 0, &bad) == PB_INVALID);
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Иванов") == 0);
    ASSERT(list_ok(&pb));
    pb_free(&pb);
}

static void test_get_bounds(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    pb_init(&pb);
    ASSERT(pb_get(&pb, 0) == NULL);
    pb_add(&pb, &c);
    ASSERT(pb_get(&pb, 0) != NULL);
    ASSERT(pb_get(&pb, 1) == NULL);
    pb_free(&pb);
}

static void test_find(void)
{
    PhoneBook pb;
    Contact a = make_contact("Яковлев", "Иван");
    Contact b = make_contact("Иванченко", "Пётр");
    Contact c = make_contact("Петров", "Кирилл");

    strcpy(c.patronymic, "Иванович");

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);
    pb_add(&pb, &c);
    /* после сортировки: Иванченко(0), Петров(1), Яковлев(2) */

    size_t out[8];
    size_t n;

    /* "иван" находится в фамилии, имени и отчестве */
    n = pb_find(&pb, "иван", out, 8);
    ASSERT(n == 3);
    /* результаты в алфавитном порядке списка */
    ASSERT(out[0] == 0 && out[1] == 1 && out[2] == 2);

    /* поиск по отчеству */
    n = pb_find(&pb, "иванович", out, 8);
    ASSERT(n == 1);
    ASSERT(strcmp(pb_get(&pb, out[0])->surname, "Петров") == 0);

    /* регистр кириллицы не мешает */
    n = pb_find(&pb, "ЯКОВЛЕВ", out, 8);
    ASSERT(n == 1);

    n = pb_find(&pb, "нет такого", out, 8);
    ASSERT(n == 0);

    /* ограничение размера выходного массива */
    n = pb_find(&pb, "иван", out, 2);
    ASSERT(n == 2);

    pb_free(&pb);
}

static void test_free_and_reuse(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    pb_init(&pb);
    pb_add(&pb, &c);
    pb_add(&pb, &c);
    pb_free(&pb);
    ASSERT(pb_count(&pb) == 0);
    ASSERT(pb_head(&pb) == NULL && pb_tail(&pb) == NULL);

    /* книга пригодна к работе после освобождения */
    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb_count(&pb) == 1);
    ASSERT(list_ok(&pb));
    pb_free(&pb);

    /* повторный pb_free безопасен */
    pb_free(&pb);
}

static void test_save_load_roundtrip(void)
{
    PhoneBook pb, loaded;
    Contact a = make_contact("Иванов", "Иван");
    Contact b = make_contact("Smith", "John");

    strcpy(a.patronymic, "Иванович");
    strcpy(a.workplace, "Eltex");
    strcpy(a.phones[0], "+7-999-111-22-33");
    strcpy(a.phones[1], "+7-383-000-00-00");
    a.phone_count = 2;
    strcpy(a.emails[0], "ivanov@example.com");
    a.email_count = 1;

    pb_init(&pb);
    pb_init(&loaded);
    pb_add(&pb, &a);
    pb_add(&pb, &b);

    ASSERT(pb_save(&pb, TEST_FILE) == PB_OK);
    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);

    REQUIRE(pb_count(&loaded) == 2);
    ASSERT(list_ok(&loaded));

    /* Smith (латиница) сортируется раньше Иванова:
     * коды латиницы меньше кодов кириллицы */
    const Contact *got = pb_get(&loaded, 0);
    REQUIRE(got != NULL);
    ASSERT(strcmp(got->surname, "Smith") == 0);

    got = pb_get(&loaded, 1);
    REQUIRE(got != NULL);
    ASSERT(strcmp(got->surname, "Иванов") == 0);
    ASSERT(strcmp(got->patronymic, "Иванович") == 0);
    ASSERT(got->phone_count == 2);
    ASSERT(strcmp(got->phones[1], "+7-383-000-00-00") == 0);
    ASSERT(strcmp(got->emails[0], "ivanov@example.com") == 0);

    pb_free(&pb);
    pb_free(&loaded);
    remove(TEST_FILE);
}

static void test_load_sorts_file(void)
{
    PhoneBook loaded;
    FILE *f = fopen(TEST_FILE, "w");

    REQUIRE(f != NULL);
    /* файл нарочно не по алфавиту */
    fprintf(f, "[contact]\nsurname=Яковлев\nname=Юрий\n\n"
               "[contact]\nsurname=Абрамов\nname=Олег\n\n"
               "[contact]\nsurname=Иванов\nname=Иван\n");
    fclose(f);

    pb_init(&loaded);
    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);
    REQUIRE(pb_count(&loaded) == 3);
    ASSERT(list_ok(&loaded)); /* включает проверку сортировки */
    ASSERT(strcmp(pb_get(&loaded, 0)->surname, "Абрамов") == 0);
    ASSERT(strcmp(pb_get(&loaded, 2)->surname, "Яковлев") == 0);

    pb_free(&loaded);
    remove(TEST_FILE);
}

static void test_load_missing_and_broken(void)
{
    PhoneBook loaded;

    pb_init(&loaded);
    ASSERT(pb_load(&loaded, "нет_такого_файла.txt") == PB_IO_ERROR);
    ASSERT(pb_count(&loaded) == 0);

    FILE *f = fopen(TEST_FILE, "w");
    REQUIRE(f != NULL);
    fprintf(f, "мусор\n[contact]\nsurname=БезИмени\n\n"
               "[contact]\nsurname=Иванов\nname=Иван\n"
               "неизвестный_ключ=значение\n");
    fclose(f);

    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);
    REQUIRE(pb_count(&loaded) == 1);
    ASSERT(strcmp(pb_get(&loaded, 0)->surname, "Иванов") == 0);

    pb_free(&loaded);
    remove(TEST_FILE);
}

int main(void)
{
    test_utf8_casecmp();
    test_contact_cmp();
    test_sorted_insert();
    test_case_insensitive_order();
    test_add_invalid();
    test_duplicates_allowed();
    test_remove();
    test_remove_tail();
    test_update_moves_node();
    test_get_bounds();
    test_find();
    test_free_and_reuse();
    test_save_load_roundtrip();
    test_load_sorts_file();
    test_load_missing_and_broken();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
