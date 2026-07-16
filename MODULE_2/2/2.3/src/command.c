#include "command.h"

void ct_init(CommandTable *t)
{
    t->count = 0;
}

/* NULL, если таблица заполнена */
static Command *ct_reserve(CommandTable *t, const char *name,
                           const char *symbol, OpArity arity)
{
    Command *c;

    if (t->count >= MAX_COMMANDS)
        return NULL;
    c = &t->items[t->count++];
    c->name = name;
    c->symbol = symbol;
    c->arity = arity;
    return c;
}

int ct_add_unary(CommandTable *t, const char *name, const char *symbol,
                 CalcUnaryFn fn)
{
    Command *c = ct_reserve(t, name, symbol, OP_UNARY);

    if (c == NULL)
        return -1;
    c->fn.unary = fn;
    return 0;
}

int ct_add_binary(CommandTable *t, const char *name, const char *symbol,
                  CalcBinaryFn fn)
{
    Command *c = ct_reserve(t, name, symbol, OP_BINARY);

    if (c == NULL)
        return -1;
    c->fn.binary = fn;
    return 0;
}

int ct_add_nary(CommandTable *t, const char *name, const char *symbol,
                CalcNaryFn fn)
{
    Command *c = ct_reserve(t, name, symbol, OP_NARY);

    if (c == NULL)
        return -1;
    c->fn.nary = fn;
    return 0;
}

size_t ct_count(const CommandTable *t)
{
    return t->count;
}

const Command *ct_get(const CommandTable *t, size_t index)
{
    if (index >= t->count)
        return NULL;
    return &t->items[index];
}
