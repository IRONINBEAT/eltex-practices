#ifndef UI_H
#define UI_H

#include "command.h"

/* Пункты меню — команды из table. Возвращает управление при выборе
 * "Выход" или по EOF. */
void ui_run(const CommandTable *table);

#endif /* UI_H */
