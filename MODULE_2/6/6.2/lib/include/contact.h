#ifndef CONTACT_H
#define CONTACT_H

#include <stddef.h>

/* Пустая строка в любом поле означает "не заполнено" */

/* Длины строк с учётом завершающего '\0' */
#define NAME_LEN  64
#define TEXT_LEN  128
#define PHONE_LEN 24
#define LINK_LEN  128

#define MAX_PHONES     5
#define MAX_EMAILS     5
#define MAX_SOCIALS    5
#define MAX_MESSENGERS 5

typedef struct {
    /* Обязательные поля */
    char surname[NAME_LEN];
    char name[NAME_LEN];

    /* Необязательные поля */
    char patronymic[NAME_LEN];
    char workplace[TEXT_LEN];
    char position[TEXT_LEN];

    char   phones[MAX_PHONES][PHONE_LEN];
    size_t phone_count;

    char   emails[MAX_EMAILS][LINK_LEN];
    size_t email_count;

    char   socials[MAX_SOCIALS][LINK_LEN];
    size_t social_count;

    char   messengers[MAX_MESSENGERS][LINK_LEN];
    size_t messenger_count;
} Contact;

void contact_init(Contact *c);

/* Контакт корректен (1), если заполнены фамилия и имя */
int contact_is_valid(const Contact *c);

#endif /* CONTACT_H */
