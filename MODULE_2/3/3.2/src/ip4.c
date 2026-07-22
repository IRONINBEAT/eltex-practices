#include <stdio.h>
#include <stdlib.h>

#include "ip4.h"

int ip4_parse(const char *s, uint32_t *out)
{
    uint32_t ip = 0;

    for (int i = 0; i < 4; i++) {
        unsigned v = 0;
        int digits = 0;

        while (*s >= '0' && *s <= '9') {
            v = v * 10 + (unsigned)(*s - '0');
            digits++;
            if (v > 255 || digits > 3)
                return -1;
            s++;
        }
        if (digits == 0)
            return -1;

        ip = (ip << 8) | v;

        if (i < 3) {
            if (*s != '.')
                return -1;
            s++;
        }
    }
    if (*s != '\0')
        return -1;

    *out = ip;
    return 0;
}

uint32_t mask_from_prefix(int prefix)
{
    /* сдвиг на 32 — неопределённое поведение, нулевой префикс отдельно */
    if (prefix <= 0)
        return 0;
    if (prefix >= 32)
        return 0xFFFFFFFFu;
    return 0xFFFFFFFFu << (32 - prefix);
}

int mask_prefix_len(uint32_t mask)
{
    int n = 0;

    while (n < 32 && (mask & (0x80000000u >> n)))
        n++;
    return n;
}

int mask_is_valid(uint32_t mask)
{
    return mask == mask_from_prefix(mask_prefix_len(mask));
}

int mask_parse(const char *s, uint32_t *out)
{
    if (s[0] == '/') {
        char *end;
        unsigned long p;

        if (s[1] < '0' || s[1] > '9')
            return -1;
        p = strtoul(s + 1, &end, 10);
        if (*end != '\0' || p > 32)
            return -1;
        *out = mask_from_prefix((int)p);
        return 0;
    }

    uint32_t m;
    if (ip4_parse(s, &m) != 0)
        return -1;
    if (!mask_is_valid(m))
        return -1;
    *out = m;
    return 0;
}

void ip4_format(uint32_t ip, char buf[16])
{
    snprintf(buf, 16, "%u.%u.%u.%u",
             (ip >> 24) & 255u, (ip >> 16) & 255u,
             (ip >> 8) & 255u, ip & 255u);
}

int ip4_same_subnet(uint32_t a, uint32_t b, uint32_t mask)
{
    /* XOR обнуляет совпадающие биты, маска отсекает хостовую часть */
    return ((a ^ b) & mask) == 0;
}
