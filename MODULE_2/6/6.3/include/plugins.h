#ifndef PLUGINS_H
#define PLUGINS_H

#include "calcplugin.h"

#define PLUGIN_DEFAULT_DIR "libs"

typedef struct {
    void       *handle;  /* результат dlopen, нужен для dlclose */
    const char *name;    /* указатели ведут ВНУТРЬ загруженной библиотеки, */
    const char *symbol;  /* поэтому действительны до dlclose */
    int         arity;
    CalcFn      apply;
    char        file[64];
} Plugin;

typedef struct {
    Plugin *items;
    size_t  count;
    size_t  capacity;
} PluginList;

void plugins_init(PluginList *pl);

/* Загружает все библиотеки из каталога dir. Возвращает количество
 * загруженных плагинов или -1, если каталог не открылся.
 * Библиотеки без нужных символов пропускаются с сообщением. */
int plugins_load(PluginList *pl, const char *dir);

/* Закрывает все библиотеки и освобождает список */
void plugins_free(PluginList *pl);

const char *calc_strerror(CalcStatus st);

#endif /* PLUGINS_H */
