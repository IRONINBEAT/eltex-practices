#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

/* Декодирует один символ UTF-8, возвращает его длину в байтах.
 * Для некорректного байта возвращает 1, чтобы перебор не зациклился. */
size_t utf8_decode(const char *s, uint32_t *cp);

/* Нижний регистр для латиницы, А-Я и Ё; прочие символы без изменений */
uint32_t utf8_fold_lower(uint32_t cp);

/* Сравнение строк без учёта регистра: <0, 0, >0 как у strcmp */
int utf8_casecmp(const char *a, const char *b);

/* 1 — needle входит в haystack как подстрока (без учёта регистра).
 * Пустая needle входит в любую строку. */
int utf8_casestr(const char *haystack, const char *needle);

#endif /* UTF8_H */
