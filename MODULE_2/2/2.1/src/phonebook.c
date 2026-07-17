#include <stdint.h>
#include <string.h>

#include "phonebook.h"

void pb_init(PhoneBook *pb)
{
    memset(pb, 0, sizeof(*pb));
}

PbResult pb_add(PhoneBook *pb, const Contact *c)
{
    if (pb->count >= MAX_CONTACTS)
        return PB_FULL;
    if (!contact_is_valid(c))
        return PB_INVALID;

    pb->items[pb->count] = *c;
    pb->count++;
    return PB_OK;
}

PbResult pb_update(PhoneBook *pb, size_t index, const Contact *c)
{
    if (index >= pb->count)
        return PB_NOT_FOUND;
    if (!contact_is_valid(c))
        return PB_INVALID;

    pb->items[index] = *c;
    return PB_OK;
}

PbResult pb_remove(PhoneBook *pb, size_t index)
{
    if (index >= pb->count)
        return PB_NOT_FOUND;

    memmove(&pb->items[index], &pb->items[index + 1],
            (pb->count - index - 1) * sizeof(Contact));
    pb->count--;
    return PB_OK;
}

const Contact *pb_get(const PhoneBook *pb, size_t index)
{
    if (index >= pb->count)
        return NULL;
    return &pb->items[index];
}

/* Декодирует один символ UTF-8 и возвращает его длину в байтах.
 * Для некорректного байта возвращает 1, чтобы перебор не зациклился. */
static size_t utf8_decode(const char *s, uint32_t *cp)
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

/* Приводит символ к нижнему регистру: латиница, А-Я и Ё.
 * tolower применить нельзя — он работает с одним байтом, а буква
 * кириллицы в UTF-8 занимает два. */
static uint32_t fold_lower(uint32_t cp)
{
    if (cp >= 'A' && cp <= 'Z')
        return cp - 'A' + 'a';
    if (cp >= 0x0410 && cp <= 0x042F)   /* А-Я */
        return cp + 0x20;               /* -> а-я */
    if (cp == 0x0401)                   /* Ё */
        return 0x0451;                  /* -> ё */
    return cp;
}

/* Поиск подстроки без учёта регистра, посимвольно по UTF-8.
 * Пустой запрос совпадает с чем угодно. */
static int utf8_casestr(const char *haystack, const char *needle)
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

            if (fold_lower(hc) != fold_lower(nc))
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

size_t pb_find(const PhoneBook *pb, const char *query,
               size_t *out, size_t out_size)
{
    size_t found = 0;

    for (size_t i = 0; i < pb->count && found < out_size; i++) {
        if (utf8_casestr(pb->items[i].surname, query) ||
            utf8_casestr(pb->items[i].name, query)) {
            out[found++] = i;
        }
    }
    return found;
}
