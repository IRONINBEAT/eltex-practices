#include <stdio.h>
#include <string.h>

#include "phonebook.h"
#include "storage.h"

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

static Contact make_contact(const char *surname, const char *name)
{
    Contact c;

    contact_init(&c);
    strncpy(c.surname, surname, NAME_LEN - 1);
    strncpy(c.name, name, NAME_LEN - 1);
    return c;
}

static void test_contact_validation(void)
{
    Contact c;

    contact_init(&c);
    ASSERT(!contact_is_valid(&c));

    strcpy(c.surname, "Иванов");
    ASSERT(!contact_is_valid(&c));

    strcpy(c.name, "Иван");
    ASSERT(contact_is_valid(&c));

    c.surname[0] = '\0';
    ASSERT(!contact_is_valid(&c));
}

static void test_add(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");
    Contact bad;

    pb_init(&pb);
    ASSERT(pb.count == 0);

    ASSERT(pb_add(&pb, &c) == PB_OK);
    ASSERT(pb.count == 1);
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Иванов") == 0);

    contact_init(&bad);
    ASSERT(pb_add(&pb, &bad) == PB_INVALID);
    ASSERT(pb.count == 1);
}

static void test_add_full(void)
{
    PhoneBook pb;
    Contact c = make_contact("Петров", "Пётр");

    pb_init(&pb);
    for (size_t i = 0; i < MAX_CONTACTS; i++)
        ASSERT(pb_add(&pb, &c) == PB_OK);

    ASSERT(pb.count == MAX_CONTACTS);
    ASSERT(pb_add(&pb, &c) == PB_FULL);
    ASSERT(pb.count == MAX_CONTACTS);
}

static void test_update(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");
    Contact upd = make_contact("Сидоров", "Семён");
    Contact bad;

    pb_init(&pb);
    pb_add(&pb, &c);

    ASSERT(pb_update(&pb, 0, &upd) == PB_OK);
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Сидоров") == 0);

    ASSERT(pb_update(&pb, 5, &upd) == PB_NOT_FOUND);

    contact_init(&bad);
    ASSERT(pb_update(&pb, 0, &bad) == PB_INVALID);
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Сидоров") == 0);
}

static void test_remove(void)
{
    PhoneBook pb;
    Contact a = make_contact("Иванов", "Иван");
    Contact b = make_contact("Петров", "Пётр");
    Contact c = make_contact("Сидоров", "Семён");

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);
    pb_add(&pb, &c);

    ASSERT(pb_remove(&pb, 1) == PB_OK);
    ASSERT(pb.count == 2);
    ASSERT(strcmp(pb_get(&pb, 0)->surname, "Иванов") == 0);
    ASSERT(strcmp(pb_get(&pb, 1)->surname, "Сидоров") == 0);

    ASSERT(pb_remove(&pb, 10) == PB_NOT_FOUND);
    ASSERT(pb.count == 2);

    ASSERT(pb_remove(&pb, 1) == PB_OK);
    ASSERT(pb_remove(&pb, 0) == PB_OK);
    ASSERT(pb.count == 0);
    ASSERT(pb_remove(&pb, 0) == PB_NOT_FOUND);
}

static void test_get(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    pb_init(&pb);
    ASSERT(pb_get(&pb, 0) == NULL);

    pb_add(&pb, &c);
    ASSERT(pb_get(&pb, 0) != NULL);
    ASSERT(pb_get(&pb, 1) == NULL);
}

static void test_find(void)
{
    PhoneBook pb;
    Contact a = make_contact("Smith", "John");
    Contact b = make_contact("Johnson", "Mary");
    Contact c = make_contact("Brown", "Kate");
    size_t out[MAX_CONTACTS];
    size_t n;

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);
    pb_add(&pb, &c);

    n = pb_find(&pb, "JOHN", out, MAX_CONTACTS);
    ASSERT(n == 2);
    ASSERT(out[0] == 0 && out[1] == 1);

    n = pb_find(&pb, "brown", out, MAX_CONTACTS);
    ASSERT(n == 1);
    ASSERT(out[0] == 2);

    n = pb_find(&pb, "нет такого", out, MAX_CONTACTS);
    ASSERT(n == 0);

    n = pb_find(&pb, "john", out, 1);
    ASSERT(n == 1);
}

static void test_multivalue_fields(void)
{
    PhoneBook pb;
    Contact c = make_contact("Иванов", "Иван");

    strcpy(c.phones[0], "+7-999-111-22-33");
    strcpy(c.phones[1], "+7-999-444-55-66");
    c.phone_count = 2;
    strcpy(c.emails[0], "ivanov@example.com");
    c.email_count = 1;

    pb_init(&pb);
    ASSERT(pb_add(&pb, &c) == PB_OK);

    const Contact *got = pb_get(&pb, 0);
    ASSERT(got->phone_count == 2);
    ASSERT(strcmp(got->phones[1], "+7-999-444-55-66") == 0);
    ASSERT(got->email_count == 1);
    ASSERT(strcmp(got->emails[0], "ivanov@example.com") == 0);
}

static void test_save_load_roundtrip(void)
{
    PhoneBook pb, loaded;
    Contact a = make_contact("Иванов", "Иван");
    Contact b = make_contact("Smith", "John");

    strcpy(a.patronymic, "Иванович");
    strcpy(a.workplace, "Eltex");
    strcpy(a.position, "Инженер");
    strcpy(a.phones[0], "+7-999-111-22-33");
    strcpy(a.phones[1], "+7-383-000-00-00");
    a.phone_count = 2;
    strcpy(a.emails[0], "ivanov@example.com");
    a.email_count = 1;
    strcpy(a.socials[0], "vk.com/ivanov");
    a.social_count = 1;
    strcpy(a.messengers[0], "t.me/ivanov");
    a.messenger_count = 1;

    pb_init(&pb);
    pb_add(&pb, &a);
    pb_add(&pb, &b);

    ASSERT(pb_save(&pb, TEST_FILE) == PB_OK);
    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);

    ASSERT(loaded.count == 2);
    const Contact *got = pb_get(&loaded, 0);
    ASSERT(strcmp(got->surname, "Иванов") == 0);
    ASSERT(strcmp(got->name, "Иван") == 0);
    ASSERT(strcmp(got->patronymic, "Иванович") == 0);
    ASSERT(strcmp(got->workplace, "Eltex") == 0);
    ASSERT(strcmp(got->position, "Инженер") == 0);
    ASSERT(got->phone_count == 2);
    ASSERT(strcmp(got->phones[0], "+7-999-111-22-33") == 0);
    ASSERT(strcmp(got->phones[1], "+7-383-000-00-00") == 0);
    ASSERT(got->email_count == 1);
    ASSERT(strcmp(got->emails[0], "ivanov@example.com") == 0);
    ASSERT(got->social_count == 1);
    ASSERT(strcmp(got->socials[0], "vk.com/ivanov") == 0);
    ASSERT(got->messenger_count == 1);
    ASSERT(strcmp(got->messengers[0], "t.me/ivanov") == 0);

    got = pb_get(&loaded, 1);
    ASSERT(strcmp(got->surname, "Smith") == 0);
    ASSERT(got->patronymic[0] == '\0');
    ASSERT(got->phone_count == 0);

    remove(TEST_FILE);
}

static void test_save_load_empty(void)
{
    PhoneBook pb, loaded;

    pb_init(&pb);
    ASSERT(pb_save(&pb, TEST_FILE) == PB_OK);
    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);
    ASSERT(loaded.count == 0);
    remove(TEST_FILE);
}

static void test_load_missing_file(void)
{
    PhoneBook loaded;

    ASSERT(pb_load(&loaded, "нет_такого_файла.txt") == PB_IO_ERROR);
    ASSERT(loaded.count == 0);
}

static void test_load_broken_file(void)
{
    PhoneBook loaded;
    FILE *f = fopen(TEST_FILE, "w");

    fprintf(f, "какой-то мусор без ключа\n"
               "phone=потерянный телефон до [contact]\n"
               "[contact]\n"
               "surname=БезИмени\n"
               "\n"
               "[contact]\n"
               "surname=Иванов\n"
               "name=Иван\n"
               "неизвестный_ключ=значение\n");
    fclose(f);

    ASSERT(pb_load(&loaded, TEST_FILE) == PB_OK);
    ASSERT(loaded.count == 1);
    ASSERT(strcmp(pb_get(&loaded, 0)->surname, "Иванов") == 0);
    remove(TEST_FILE);
}

int main(void)
{
    test_contact_validation();
    test_add();
    test_add_full();
    test_update();
    test_remove();
    test_get();
    test_find();
    test_multivalue_fields();
    test_save_load_roundtrip();
    test_save_load_empty();
    test_load_missing_file();
    test_load_broken_file();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
