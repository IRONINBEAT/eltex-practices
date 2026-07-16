#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

#include "calc.h"

/*
 * Таблица команд калькулятора. Меню и вызов операции строятся из неё,
 * поэтому новая операция добавляется одной строкой ct_add_* в main.c —
 * ui.c менять не нужно.
 */

typedef CalcStatus (*CalcUnaryFn)(double, double *);
typedef CalcStatus (*CalcBinaryFn)(double, double, double *);
typedef CalcStatus (*CalcNaryFn)(const double *, size_t, double *);

typedef enum {
    OP_UNARY,
    OP_BINARY,
    OP_NARY
} OpArity;

typedef struct {
    const char *name;   /* пункт меню */
    const char *symbol; /* знак для вывода результата */
    OpArity     arity;  /* определяет активный член union fn */
    union {
        CalcUnaryFn  unary;
        CalcBinaryFn binary;
        CalcNaryFn   nary;
    } fn;
} Command;

#define MAX_COMMANDS 32

typedef struct {
    Command items[MAX_COMMANDS];
    size_t  count;
} CommandTable;

void ct_init(CommandTable *t);

/* Возвращают 0 при успехе, -1 если таблица заполнена */
int ct_add_unary(CommandTable *t, const char *name, const char *symbol,
                 CalcUnaryFn fn);
int ct_add_binary(CommandTable *t, const char *name, const char *symbol,
                  CalcBinaryFn fn);
int ct_add_nary(CommandTable *t, const char *name, const char *symbol,
                CalcNaryFn fn);

size_t ct_count(const CommandTable *t);

/* NULL, если индекс за пределами count */
const Command *ct_get(const CommandTable *t, size_t index);

#endif /* COMMAND_H */
