#ifndef IP4_H
#define IP4_H

#include <stdint.h>

/*
 * IPv4-адрес хранится как uint32_t: старший байт — первый октет,
 * 192.168.1.10 -> 0xC0A8010A.
 */

/* Функции разбора возвращают 0 при успехе, -1 при ошибке;
 * при ошибке *out не изменяется. */

/* 192.168.1.10 строго 4 десятичных октета 0..255 */
int ip4_parse(const char *s, uint32_t *out);

/* Маска в любой из двух записей: "255.255.255.0" или "/24". */
int mask_parse(const char *s, uint32_t *out);

/* Маска из длины префикса 0..32 */
uint32_t mask_from_prefix(int prefix);

/* Количество ведущих единиц (для вывода вида /24) */
int mask_prefix_len(uint32_t mask);

/* Маска корректна, если единицы идут подряд начиная со старшего бита */
int mask_is_valid(uint32_t mask);

void ip4_format(uint32_t ip, char buf[16]);

/* Вернет 1, если адреса a и b в одной подсети по данной маске */
int ip4_same_subnet(uint32_t a, uint32_t b, uint32_t mask);

#endif /* IP4_H */
