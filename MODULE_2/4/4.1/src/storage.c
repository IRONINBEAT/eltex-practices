#include <stdio.h>
#include <string.h>

#include "storage.h"

/* ---------- запись ---------- */

static void write_field(FILE *f, const char *key, const char *val)
{
    if (val[0] != '\0')
        fprintf(f, "%s=%s\n", key, val);
}

static void write_list(FILE *f, const char *key, const char *base,
                       size_t item_size, size_t count)
{
    for (size_t i = 0; i < count; i++)
        fprintf(f, "%s=%s\n", key, base + i * item_size);
}

PbResult pb_save(const PhoneBook *pb, const char *path)
{
    FILE *f = fopen(path, "w");

    if (f == NULL)
        return PB_IO_ERROR;

    for (const PbNode *n = pb_head(pb); n != NULL; n = n->next) {
        const Contact *c = &n->data;

        fprintf(f, "[contact]\n");
        write_field(f, "surname", c->surname);
        write_field(f, "name", c->name);
        write_field(f, "patronymic", c->patronymic);
        write_field(f, "workplace", c->workplace);
        write_field(f, "position", c->position);
        write_list(f, "phone", &c->phones[0][0],
                   PHONE_LEN, c->phone_count);
        write_list(f, "email", &c->emails[0][0],
                   LINK_LEN, c->email_count);
        write_list(f, "social", &c->socials[0][0],
                   LINK_LEN, c->social_count);
        write_list(f, "messenger", &c->messengers[0][0],
                   LINK_LEN, c->messenger_count);
        fprintf(f, "\n");
    }

    if (fclose(f) != 0)
        return PB_IO_ERROR;
    return PB_OK;
}

/* ---------- чтение ---------- */

static void set_field(char *dst, size_t size, const char *val)
{
    strncpy(dst, val, size - 1);
    dst[size - 1] = '\0';
}

/* Значения сверх max молча отбрасываются */
static void list_put(char *base, size_t item_size, size_t max,
                     size_t *count, const char *val)
{
    if (*count >= max)
        return;
    set_field(base + (*count) * item_size, item_size, val);
    (*count)++;
}

/* Неизвестные ключи игнорируются, чтобы файл с другим набором полей
 * всё равно читался. */
static void apply_field(Contact *c, const char *key, const char *val)
{
    if (strcmp(key, "surname") == 0)
        set_field(c->surname, NAME_LEN, val);
    else if (strcmp(key, "name") == 0)
        set_field(c->name, NAME_LEN, val);
    else if (strcmp(key, "patronymic") == 0)
        set_field(c->patronymic, NAME_LEN, val);
    else if (strcmp(key, "workplace") == 0)
        set_field(c->workplace, TEXT_LEN, val);
    else if (strcmp(key, "position") == 0)
        set_field(c->position, TEXT_LEN, val);
    else if (strcmp(key, "phone") == 0)
        list_put(&c->phones[0][0], PHONE_LEN, MAX_PHONES,
                 &c->phone_count, val);
    else if (strcmp(key, "email") == 0)
        list_put(&c->emails[0][0], LINK_LEN, MAX_EMAILS,
                 &c->email_count, val);
    else if (strcmp(key, "social") == 0)
        list_put(&c->socials[0][0], LINK_LEN, MAX_SOCIALS,
                 &c->social_count, val);
    else if (strcmp(key, "messenger") == 0)
        list_put(&c->messengers[0][0], LINK_LEN, MAX_MESSENGERS,
                 &c->messenger_count, val);
}

PbResult pb_load(PhoneBook *pb, const char *path)
{
    char line[512];
    Contact c;
    int in_contact = 0;
    FILE *f;

    pb_free(pb);

    f = fopen(path, "r");
    if (f == NULL)
        return PB_IO_ERROR;

    while (fgets(line, sizeof(line), f) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0')
            continue;

        if (strcmp(line, "[contact]") == 0) {
            if (in_contact)
                pb_add(pb, &c);
            contact_init(&c);
            in_contact = 1;
            continue;
        }
        if (!in_contact)
            continue;

        char *eq = strchr(line, '=');
        if (eq == NULL)
            continue;
        *eq = '\0';
        apply_field(&c, line, eq + 1);
    }
    if (in_contact)
        pb_add(pb, &c);

    fclose(f);
    return PB_OK;
}
