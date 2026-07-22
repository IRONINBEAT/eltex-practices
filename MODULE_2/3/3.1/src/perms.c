/* Для объявления stat при сборке с -std=c11 на glibc */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "perms.h"

int perm_parse_octal(const char *s, perm_t *out)
{
    char *end;
    unsigned long v;

    if (s[0] == '\0')
        return -1;
    v = strtoul(s, &end, 8);
    if (*end != '\0' || end - s > 4 || v > PERM_MASK)
        return -1;
    *out = (perm_t)v;
    return 0;
}

int perm_parse_symbolic(const char *s, perm_t *out)
{
    static const char rwx[] = "rwxrwxrwx";
    perm_t m = 0;

    if (strlen(s) != 9)
        return -1;

    for (int i = 0; i < 9; i++) {
        char c = s[i];
        int bit = 8 - i;

        if (c == '-')
            continue;
        if (c == rwx[i]) {
            m |= 1u << bit;
            continue;
        }
        if (i == 2 && (c == 's' || c == 'S')) {
            m |= P_SUID;
            if (c == 's')
                m |= P_UX;
            continue;
        }
        if (i == 5 && (c == 's' || c == 'S')) {
            m |= P_SGID;
            if (c == 's')
                m |= P_GX;
            continue;
        }
        if (i == 8 && (c == 't' || c == 'T')) {
            m |= P_STICKY;
            if (c == 't')
                m |= P_OX;
            continue;
        }
        return -1;
    }
    *out = m;
    return 0;
}

int perm_parse(const char *s, perm_t *out)
{
    if (s[0] >= '0' && s[0] <= '9')
        return perm_parse_octal(s, out);
    return perm_parse_symbolic(s, out);
}

int perm_from_file(const char *path, perm_t *out, char *ftype)
{
    struct stat st;

    if (stat(path, &st) != 0)
        return -1;

    *out = (perm_t)(st.st_mode & PERM_MASK);

    if (S_ISREG(st.st_mode))       *ftype = '-';
    else if (S_ISDIR(st.st_mode))  *ftype = 'd';
    else if (S_ISLNK(st.st_mode))  *ftype = 'l';
    else if (S_ISCHR(st.st_mode))  *ftype = 'c';
    else if (S_ISBLK(st.st_mode))  *ftype = 'b';
    else if (S_ISFIFO(st.st_mode)) *ftype = 'p';
    else                           *ftype = 's';
    return 0;
}

/* Разбирает одну команду до запятой или конца строки. */
static int apply_one(perm_t *m, const char **p)
{
    unsigned who = 0; /* битовая маска: 1 - u, 2 - g, 4 - o */
    unsigned rwx = 0;
    int set_s = 0, set_t = 0;
    char op;
    const char *s = *p;

    while (*s == 'u' || *s == 'g' || *s == 'o' || *s == 'a') {
        if (*s == 'u') who |= 1;
        else if (*s == 'g') who |= 2;
        else if (*s == 'o') who |= 4;
        else who |= 7;
        s++;
    }
    if (who == 0)
        who = 7; /* если "кому" не указано то как chmod a */

    op = *s;
    if (op != '+' && op != '-' && op != '=')
        return -1;
    s++;

    while (*s != '\0' && *s != ',') {
        switch (*s) {
        case 'r': rwx |= 4; break;
        case 'w': rwx |= 2; break;
        case 'x': rwx |= 1; break;
        case 's': set_s = 1; break;
        case 't': set_t = 1; break;
        default: return -1;
        }
        s++;
    }

    perm_t bits = 0;
    if (who & 1) {
        bits |= rwx << 6;
        if (set_s)
            bits |= P_SUID;
    }
    if (who & 2) {
        bits |= rwx << 3;
        if (set_s)
            bits |= P_SGID;
    }
    if (who & 4)
        bits |= rwx;
    if (set_t)
        bits |= P_STICKY;

    switch (op) {
    case '+':
        *m |= bits;
        break;
    case '-':
        *m &= ~bits;
        break;
    case '=': {
        /* '=' очищает rwx и спецбит каждой указанной группы,
         * затем ставит перечисленное */
        perm_t clear = 0;
        if (who & 1) clear |= (7u << 6) | P_SUID;
        if (who & 2) clear |= (7u << 3) | P_SGID;
        if (who & 4) clear |= 7u | P_STICKY;
        *m = (*m & ~clear) | bits;
        break;
    }
    }

    *p = s;
    return 0;
}

int perm_apply(perm_t *mode, const char *commands)
{
    perm_t m = *mode; 
    const char *p = commands;

    if (*p == '\0')
        return -1;

    for (;;) {
        if (apply_one(&m, &p) != 0)
            return -1;
        if (*p == '\0')
            break;
        p++; /* ',' */
        if (*p == '\0')
            return -1; 
    }

    *mode = m & PERM_MASK;
    return 0;
}

void perm_to_symbolic(perm_t m, char buf[10])
{
    static const char rwx[] = "rwxrwxrwx";

    for (int i = 0; i < 9; i++)
        buf[i] = ((m >> (8 - i)) & 1u) ? rwx[i] : '-';

    /* спецбиты отображаются на месте x: строчная — x есть, заглавная — нет */
    if (m & P_SUID)
        buf[2] = (m & P_UX) ? 's' : 'S';
    if (m & P_SGID)
        buf[5] = (m & P_GX) ? 's' : 'S';
    if (m & P_STICKY)
        buf[8] = (m & P_OX) ? 't' : 'T';

    buf[9] = '\0';
}

void perm_to_octal(perm_t m, char buf[8])
{
    snprintf(buf, 8, "%04o", m & PERM_MASK);
}

void perm_to_binary(perm_t m, char buf[16])
{
    int pos = 0;

    for (int bit = 11; bit >= 0; bit--) {
        buf[pos++] = ((m >> bit) & 1u) ? '1' : '0';
        if (bit % 3 == 0 && bit != 0)
            buf[pos++] = ' ';
    }
    buf[pos] = '\0';
}
