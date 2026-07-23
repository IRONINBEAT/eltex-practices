#include <stddef.h>

#include "utf8.h"

size_t utf8_decode(const char *s, uint32_t *cp)
{
    const unsigned char *p = (const unsigned char *)s;

    if (p[0] < 0x80) {
        *cp = p[0];
        return 1;
    }
    if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(p[0] & 0x1F) << 6) |
               (uint32_t)(p[1] & 0x3F);
        return 2;
    }
    if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 &&
        (p[2] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(p[0] & 0x0F) << 12) |
              ((uint32_t)(p[1] & 0x3F) << 6) |
               (uint32_t)(p[2] & 0x3F);
        return 3;
    }
    if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 &&
        (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(p[0] & 0x07) << 18) |
              ((uint32_t)(p[1] & 0x3F) << 12) |
              ((uint32_t)(p[2] & 0x3F) << 6) |
               (uint32_t)(p[3] & 0x3F);
        return 4;
    }
    *cp = p[0];
    return 1;
}

uint32_t utf8_fold_lower(uint32_t cp)
{
    if (cp >= 'A' && cp <= 'Z')
        return cp - 'A' + 'a';
    if (cp >= 0x0410 && cp <= 0x042F)   /* А-Я */
        return cp + 0x20;               /* -> а-я */
    if (cp == 0x0401)                   /* Ё */
        return 0x0451;                  /* -> ё */
    return cp;
}

int utf8_casecmp(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        uint32_t ca, cb;

        a += utf8_decode(a, &ca);
        b += utf8_decode(b, &cb);
        ca = utf8_fold_lower(ca);
        cb = utf8_fold_lower(cb);
        if (ca != cb)
            return ca < cb ? -1 : 1;
    }
    /* короткая строка-префикс считается меньшей */
    if (*a != '\0')
        return 1;
    if (*b != '\0')
        return -1;
    return 0;
}

int utf8_casestr(const char *haystack, const char *needle)
{
    if (needle[0] == '\0')
        return 1;

    for (const char *start = haystack; *start != '\0'; ) {
        const char *h = start;
        const char *n = needle;
        uint32_t hc, nc;

        while (*n != '\0' && *h != '\0') {
            size_t hlen = utf8_decode(h, &hc);
            size_t nlen = utf8_decode(n, &nc);

            if (utf8_fold_lower(hc) != utf8_fold_lower(nc))
                break;
            h += hlen;
            n += nlen;
        }
        if (*n == '\0')
            return 1;

        start += utf8_decode(start, &hc);
    }
    return 0;
}
