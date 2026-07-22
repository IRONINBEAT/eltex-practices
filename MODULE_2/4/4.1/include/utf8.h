#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

size_t utf8_decode(const char *s, uint32_t *cp);

uint32_t utf8_fold_lower(uint32_t cp);

int utf8_casecmp(const char *a, const char *b);

int utf8_casestr(const char *haystack, const char *needle);

#endif /* UTF8_H */
