#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ip4.h"

/* rand() гарантирует лишь 15 случайных бит (RAND_MAX >= 32767),
 * поэтому 32-битное число собирается из трёх вызовов */
static uint32_t rand_u32(void)
{
    uint32_t a = (uint32_t)rand() & 0x7FFFu;
    uint32_t b = (uint32_t)rand() & 0x7FFFu;
    uint32_t c = (uint32_t)rand() & 0x3u;

    return (a << 17) | (b << 2) | c;
}

static void usage(const char *prog)
{
    fprintf(stderr,
            "Использование: %s <IP шлюза> <маска> <кол-во пакетов> [зерно]\n"
            "  маска  — точечная (255.255.255.0) или префикс (/24)\n"
            "  зерно  — необязательное число для повторяемости результата\n"
            "Пример: %s 192.168.1.1 /24 10\n",
            prog, prog);
}

int main(int argc, char **argv)
{
    uint32_t gw, mask;
    unsigned long n, own = 0;
    unsigned long seed;
    char buf[16], mbuf[16];
    char *end;

    if (argc < 4 || argc > 5) {
        usage(argv[0]);
        return 1;
    }

    if (ip4_parse(argv[1], &gw) != 0) {
        fprintf(stderr, "Некорректный IP шлюза: \"%s\"\n", argv[1]);
        return 1;
    }
    if (mask_parse(argv[2], &mask) != 0) {
        fprintf(stderr, "Некорректная маска: \"%s\"\n", argv[2]);
        return 1;
    }
    n = strtoul(argv[3], &end, 10);
    if (argv[3][0] == '\0' || *end != '\0' || n == 0) {
        fprintf(stderr, "Некорректное количество пакетов: \"%s\"\n",
                argv[3]);
        return 1;
    }
    if (argc == 5) {
        seed = strtoul(argv[4], &end, 10);
        if (argv[4][0] == '\0' || *end != '\0') {
            fprintf(stderr, "Некорректное зерно: \"%s\"\n", argv[4]);
            return 1;
        }
    } else {
        seed = (unsigned long)time(NULL);
    }
    srand((unsigned)seed);

    ip4_format(gw, buf);
    ip4_format(mask, mbuf);
    printf("Шлюз:  %s\n", buf);
    printf("Маска: %s (/%d)\n", mbuf, mask_prefix_len(mask));
    ip4_format(gw & mask, buf);
    printf("Сеть:  %s/%d\n", buf, mask_prefix_len(mask));
    printf("Зерно: %lu\n\n", seed);

    printf("Имитация отправки %lu пакетов:\n", n);
    for (unsigned long i = 0; i < n; i++) {
        uint32_t dst = rand_u32();
        int local = ip4_same_subnet(dst, gw, mask);

        ip4_format(dst, buf);
        printf("%4lu) %-15s — %s\n", i + 1, buf,
               local ? "своя подсеть" : "другая сеть");
        own += (unsigned long)local;
    }

    printf("\n===== СТАТИСТИКА =====\n");
    printf("Своя подсеть: %lu из %lu (%.1f%%)\n",
           own, n, (double)own * 100.0 / (double)n);
    printf("Другие сети:  %lu из %lu (%.1f%%)\n",
           n - own, n, (double)(n - own) * 100.0 / (double)n);
    return 0;
}
