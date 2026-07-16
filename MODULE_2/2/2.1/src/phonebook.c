#include <ctype.h>
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

/* Регистр игнорируется только для ASCII: кириллица в UTF-8 занимает
 * несколько байт, tolower на них не действует. */
static const char *strcasestr_ascii(const char *haystack, const char *needle)
{
    size_t nlen = strlen(needle);
    if (nlen == 0)
        return haystack;

    for (; *haystack; haystack++) {
        size_t i = 0;
        while (i < nlen &&
               tolower((unsigned char)haystack[i]) ==
               tolower((unsigned char)needle[i]))
            i++;
        if (i == nlen)
            return haystack;
    }
    return NULL;
}

size_t pb_find(const PhoneBook *pb, const char *query,
               size_t *out, size_t out_size)
{
    size_t found = 0;

    for (size_t i = 0; i < pb->count && found < out_size; i++) {
        if (strcasestr_ascii(pb->items[i].surname, query) != NULL ||
            strcasestr_ascii(pb->items[i].name, query) != NULL) {
            out[found++] = i;
        }
    }
    return found;
}
