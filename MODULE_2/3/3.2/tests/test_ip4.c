#include <stdio.h>
#include <string.h>

#include "ip4.h"

static int tests_run;
static int tests_failed;

#define ASSERT(cond) do {                                             \
    tests_run++;                                                      \
    if (!(cond)) {                                                    \
        tests_failed++;                                               \
        printf("FAIL: %s (%s:%d)\n", #cond, __FILE__, __LINE__);      \
    }                                                                 \
} while (0)

static void test_ip_parse(void)
{
    uint32_t ip = 0;

    ASSERT(ip4_parse("192.168.1.10", &ip) == 0 && ip == 0xC0A8010Au);
    ASSERT(ip4_parse("0.0.0.0", &ip) == 0 && ip == 0);
    ASSERT(ip4_parse("255.255.255.255", &ip) == 0 && ip == 0xFFFFFFFFu);
    ASSERT(ip4_parse("8.8.8.8", &ip) == 0 && ip == 0x08080808u);
    ASSERT(ip4_parse("1.2.3.4", &ip) == 0 && ip == 0x01020304u);

    ip = 0x1234;
    ASSERT(ip4_parse("", &ip) == -1);
    ASSERT(ip4_parse("1.2.3", &ip) == -1);         /* мало октетов */
    ASSERT(ip4_parse("1.2.3.4.5", &ip) == -1);     /* много октетов */
    ASSERT(ip4_parse("256.1.1.1", &ip) == -1);     /* октет > 255 */
    ASSERT(ip4_parse("1.2.3.999", &ip) == -1);
    ASSERT(ip4_parse("a.b.c.d", &ip) == -1);
    ASSERT(ip4_parse("1..2.3", &ip) == -1);        /* пустой октет */
    ASSERT(ip4_parse(".1.2.3", &ip) == -1);
    ASSERT(ip4_parse("1.2.3.4.", &ip) == -1);      /* хвостовая точка */
    ASSERT(ip4_parse("1.2.3.4x", &ip) == -1);      /* мусор в конце */
    ASSERT(ip4_parse("1.2.3.0004", &ip) == -1);    /* больше 3 цифр */
    ASSERT(ip4_parse("-1.2.3.4", &ip) == -1);
    ASSERT(ip4_parse("1, 2, 3, 4", &ip) == -1);
    ASSERT(ip == 0x1234); /* при ошибках значение не тронуто */
}

static void test_ip_format(void)
{
    char buf[16];

    ip4_format(0xC0A8010Au, buf);
    ASSERT(strcmp(buf, "192.168.1.10") == 0);
    ip4_format(0, buf);
    ASSERT(strcmp(buf, "0.0.0.0") == 0);
    ip4_format(0xFFFFFFFFu, buf);
    ASSERT(strcmp(buf, "255.255.255.255") == 0);
}

/* Разбор и форматирование согласованы */
static void test_ip_roundtrip(void)
{
    static const uint32_t cases[] = {
        0, 0xFFFFFFFFu, 0xC0A8010Au, 0x7F000001u, 0x0A000001u
    };
    char buf[16];
    uint32_t back = 0;

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        ip4_format(cases[i], buf);
        ASSERT(ip4_parse(buf, &back) == 0 && back == cases[i]);
    }
}

static void test_mask_from_prefix(void)
{
    ASSERT(mask_from_prefix(0) == 0);
    ASSERT(mask_from_prefix(1) == 0x80000000u);
    ASSERT(mask_from_prefix(8) == 0xFF000000u);
    ASSERT(mask_from_prefix(24) == 0xFFFFFF00u);
    ASSERT(mask_from_prefix(31) == 0xFFFFFFFEu);
    ASSERT(mask_from_prefix(32) == 0xFFFFFFFFu);
}

static void test_mask_prefix_len(void)
{
    ASSERT(mask_prefix_len(0) == 0);
    ASSERT(mask_prefix_len(0x80000000u) == 1);
    ASSERT(mask_prefix_len(0xFF000000u) == 8);
    ASSERT(mask_prefix_len(0xFFFFFF00u) == 24);
    ASSERT(mask_prefix_len(0xFFFFFFFFu) == 32);
}

static void test_mask_is_valid(void)
{
    ASSERT(mask_is_valid(0));
    ASSERT(mask_is_valid(0xFF000000u));
    ASSERT(mask_is_valid(0xFFFFFF00u));
    ASSERT(mask_is_valid(0xFFFFFFFFu));

    ASSERT(!mask_is_valid(0x00FF0000u));  /* единицы не сверху */
    ASSERT(!mask_is_valid(0xFF00FF00u));  /* разрыв */
    ASSERT(!mask_is_valid(0xFFFFFF01u));  /* единица в хвосте */
    ASSERT(!mask_is_valid(0x00000001u));
}

static void test_mask_parse(void)
{
    uint32_t m = 0;

    /* точечная запись */
    ASSERT(mask_parse("255.255.255.0", &m) == 0 && m == 0xFFFFFF00u);
    ASSERT(mask_parse("255.0.0.0", &m) == 0 && m == 0xFF000000u);
    ASSERT(mask_parse("255.255.255.255", &m) == 0 && m == 0xFFFFFFFFu);
    ASSERT(mask_parse("0.0.0.0", &m) == 0 && m == 0);

    /* запись через префикс */
    ASSERT(mask_parse("/24", &m) == 0 && m == 0xFFFFFF00u);
    ASSERT(mask_parse("/8", &m) == 0 && m == 0xFF000000u);
    ASSERT(mask_parse("/0", &m) == 0 && m == 0);
    ASSERT(mask_parse("/32", &m) == 0 && m == 0xFFFFFFFFu);
    ASSERT(mask_parse("/1", &m) == 0 && m == 0x80000000u);

    m = 0x1234;
    ASSERT(mask_parse("/33", &m) == -1);           /* префикс > 32 */
    ASSERT(mask_parse("/", &m) == -1);
    ASSERT(mask_parse("/abc", &m) == -1);
    ASSERT(mask_parse("/2 4", &m) == -1);
    ASSERT(mask_parse("/-1", &m) == -1);
    ASSERT(mask_parse("255.0.255.0", &m) == -1);   /* разрыв единиц */
    ASSERT(mask_parse("255.255.255.1", &m) == -1);
    ASSERT(mask_parse("128.255.0.0", &m) == -1);
    ASSERT(mask_parse("abc", &m) == -1);
    ASSERT(mask_parse("", &m) == -1);
    ASSERT(m == 0x1234);
}

static void test_same_subnet(void)
{
    uint32_t gw, a, b;

    /* пример из лекции: 192.168.1.10 и 192.168.2.1 при /24 — разные */
    ip4_parse("192.168.1.10", &gw);
    ip4_parse("192.168.2.1", &a);
    ip4_parse("192.168.1.200", &b);
    ASSERT(!ip4_same_subnet(gw, a, 0xFFFFFF00u));
    ASSERT(ip4_same_subnet(gw, b, 0xFFFFFF00u));

    /* при /16 обе в одной сети 192.168.0.0 */
    ASSERT(ip4_same_subnet(gw, a, 0xFFFF0000u));

    /* /32 — совпадение только с самим собой */
    ASSERT(ip4_same_subnet(gw, gw, 0xFFFFFFFFu));
    ASSERT(!ip4_same_subnet(gw, b, 0xFFFFFFFFu));

    /* /0 — все адреса «свои» */
    ip4_parse("8.8.8.8", &a);
    ASSERT(ip4_same_subnet(gw, a, 0));

    /* граница подсети: .255 ещё своя, следующий адрес — уже нет */
    ip4_parse("192.168.1.255", &a);
    ip4_parse("192.168.2.0", &b);
    ASSERT(ip4_same_subnet(gw, a, 0xFFFFFF00u));
    ASSERT(!ip4_same_subnet(gw, b, 0xFFFFFF00u));
}

int main(void)
{
    test_ip_parse();
    test_ip_format();
    test_ip_roundtrip();
    test_mask_from_prefix();
    test_mask_prefix_len();
    test_mask_is_valid();
    test_mask_parse();
    test_same_subnet();

    printf("Тестов выполнено: %d, провалено: %d\n",
           tests_run, tests_failed);
    if (tests_failed == 0)
        printf("ВСЕ ТЕСТЫ ПРОЙДЕНЫ\n");
    return tests_failed != 0;
}
