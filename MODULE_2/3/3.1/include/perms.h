#ifndef PERMS_H
#define PERMS_H

/*
 *   бит 11 10  9   8 7 6   5 4 3   2 1 0
 *       su sg st   r w x   r w x   r w x
 *       (спец.)   (владелец)(группа)(остальные)
 */

typedef unsigned int perm_t;

enum {
    P_OX = 1u << 0,  P_OW = 1u << 1,  P_OR = 1u << 2,
    P_GX = 1u << 3,  P_GW = 1u << 4,  P_GR = 1u << 5,
    P_UX = 1u << 6,  P_UW = 1u << 7,  P_UR = 1u << 8,
    P_STICKY = 1u << 9, P_SGID = 1u << 10, P_SUID = 1u << 11
};

#define PERM_MASK ((1u << 12) - 1)

/* Все функции разбора возвращают 0 при успехе, -1 при ошибке;
 * при ошибке *out не изменяется. */

/* "755", "0644", "4755" — восьмеричная запись, 1-4 цифры */
int perm_parse_octal(const char *s, perm_t *out);

/* "rw-r--r--" — ровно 9 символов, позиции фиксированы;
 * в позициях x допускаются s/S (suid, sgid) и t/T (sticky) */
int perm_parse_symbolic(const char *s, perm_t *out);

/* Автоопределение: начинается с цифры — восьмеричная, иначе буквенная */
int perm_parse(const char *s, perm_t *out);

/* Права файла через stat */
int perm_from_file(const char *path, perm_t *out, char *ftype);

/* Команды как chmod: "u+x", "go-w", "a=rx", или через запятую. */
int perm_apply(perm_t *mode, const char *commands);

void perm_to_symbolic(perm_t m, char buf[10]);
void perm_to_octal(perm_t m, char buf[8]);    
void perm_to_binary(perm_t m, char buf[16]);  

#endif /* PERMS_H */
