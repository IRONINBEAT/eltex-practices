#include <string.h>

#include "contact.h"

void contact_init(Contact *c)
{
    memset(c, 0, sizeof(*c));
}

int contact_is_valid(const Contact *c)
{
    return c->surname[0] != '\0' && c->name[0] != '\0';
}
