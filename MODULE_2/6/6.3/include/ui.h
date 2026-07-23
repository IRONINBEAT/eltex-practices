#ifndef UI_H
#define UI_H

#include "plugins.h"

/* Меню строится из загруженных плагинов. Возвращает управление при
 * выборе "Выход" или по EOF. */
void ui_run(const PluginList *pl);

#endif /* UI_H */
