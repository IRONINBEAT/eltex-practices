#include <math.h>
#include <stdio.h>
#include <string.h>

#include "plugins.h"

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

static int approx(double a, double b)
{
    return fabs(a - b) < 1e-9;
}

/* Ищет загруженный плагин по знаку операции */
static const Plugin *by_symbol(const PluginList *pl, const char *sym)
{
    for (size_t i = 0; i < pl->count; i++) {
        if (strcmp(pl->items[i].symbol, sym) == 0)
            return &pl->items[i];
    }
    return NULL;
}

static void test_load_directory(void)
{
    PluginList pl;

    plugins_init(&pl);
    REQUIRE(plugins_load(&pl, PLUGIN_DEFAULT_DIR) > 0);
    ASSERT(pl.count > 0);

    /* у каждого загруженного плагина заполнены все поля */
    for (size_t i = 0; i < pl.count; i++) {
        ASSERT(pl.items[i].handle != NULL);
        ASSERT(pl.items[i].name != NULL && pl.items[i].name[0] != '\0');
        ASSERT(pl.items[i].symbol != NULL && pl.items[i].symbol[0] != '\0');
        ASSERT(pl.items[i].apply != NULL);
        ASSERT(pl.items[i].arity == CALC_NARY ||
               pl.items[i].arity == CALC_UNARY ||
               pl.items[i].arity == CALC_BINARY);
    }
    plugins_free(&pl);
}

static void test_missing_directory(void)
{
    PluginList pl;

    plugins_init(&pl);
    ASSERT(plugins_load(&pl, "каталога_нет") == -1);
    ASSERT(pl.count == 0);
    plugins_free(&pl);
}

/* Порядок загрузки стабилен: имена файлов отсортированы */
static void test_sorted_order(void)
{
    PluginList pl;

    plugins_init(&pl);
    REQUIRE(plugins_load(&pl, PLUGIN_DEFAULT_DIR) > 1);
    for (size_t i = 1; i < pl.count; i++)
        ASSERT(strcmp(pl.items[i - 1].file, pl.items[i].file) < 0);
    plugins_free(&pl);
}

/* Главная проверка: операция вызывается ЧЕРЕЗ указатель из библиотеки */
static void test_call_through_pointer(void)
{
    PluginList pl;
    double args[4];
    double r;

    plugins_init(&pl);
    REQUIRE(plugins_load(&pl, PLUGIN_DEFAULT_DIR) > 0);

    const Plugin *p = by_symbol(&pl, "+");
    REQUIRE(p != NULL);
    args[0] = 2; args[1] = 3;
    ASSERT(p->apply(args, 2, &r) == CALC_OK && approx(r, 5));
    ASSERT(p->arity == CALC_BINARY);

    p = by_symbol(&pl, "sqrt");
    REQUIRE(p != NULL);
    args[0] = 16;
    ASSERT(p->apply(args, 1, &r) == CALC_OK && approx(r, 4));
    ASSERT(p->arity == CALC_UNARY);
    args[0] = -1;
    ASSERT(p->apply(args, 1, &r) == CALC_DOMAIN);

    p = by_symbol(&pl, "/");
    REQUIRE(p != NULL);
    args[0] = 10; args[1] = 4;
    ASSERT(p->apply(args, 2, &r) == CALC_OK && approx(r, 2.5));
    args[1] = 0;
    r = 123;
    ASSERT(p->apply(args, 2, &r) == CALC_DIV_ZERO && approx(r, 123));

    p = by_symbol(&pl, "sum");
    REQUIRE(p != NULL);
    ASSERT(p->arity == CALC_NARY);
    args[0] = 1; args[1] = 2; args[2] = 3; args[3] = 4;
    ASSERT(p->apply(args, 4, &r) == CALC_OK && approx(r, 10));

    p = by_symbol(&pl, "avg");
    REQUIRE(p != NULL);
    ASSERT(p->apply(args, 4, &r) == CALC_OK && approx(r, 2.5));
    ASSERT(p->apply(args, 0, &r) == CALC_DIV_ZERO);

    plugins_free(&pl);
}

/* Плагин отказывается работать при неверном числе аргументов */
static void test_wrong_arg_count(void)
{
    PluginList pl;
    double args[2] = {1, 2};
    double r;

    plugins_init(&pl);
    REQUIRE(plugins_load(&pl, PLUGIN_DEFAULT_DIR) > 0);

    const Plugin *p = by_symbol(&pl, "+");
    REQUIRE(p != NULL);
    ASSERT(p->apply(args, 1, &r) == CALC_DOMAIN);
    ASSERT(p->apply(args, 0, &r) == CALC_DOMAIN);

    plugins_free(&pl);
}

static void test_free_and_reload(void)
{
    PluginList pl;
    size_t first;

    plugins_init(&pl);
    REQUIRE(plugins_load(&pl, PLUGIN_DEFAULT_DIR) > 0);
    first = pl.count;
    plugins_free(&pl);
    ASSERT(pl.count == 0);
    ASSERT(pl.items == NULL);

    /* повторная загрузка даёт тот же результат */
    REQUIRE(plugins_load(&pl, PLUGIN_DEFAULT_DIR) > 0);
    ASSERT(pl.count == first);
    plugins_free(&pl);
    plugins_free(&pl); /* повторный вызов безопасен */
}

static void test_strerror(void)
{
    ASSERT(calc_strerror(CALC_OK)[0] != '\0');
    ASSERT(calc_strerror(CALC_DIV_ZERO)[0] != '\0');
    ASSERT(calc_strerror(CALC_DOMAIN)[0] != '\0');
    ASSERT(calc_strerror(CALC_RANGE)[0] != '\0');
}

int main(void)
{
    test_load_directory();
    test_missing_directory();
    test_sorted_order();
    test_call_through_pointer();
    test_wrong_arg_count();
    test_free_and_reload();
    test_strerror();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
