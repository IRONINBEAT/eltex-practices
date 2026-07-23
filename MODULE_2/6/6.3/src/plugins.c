#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plugins.h"

void plugins_init(PluginList *pl)
{
    pl->items = NULL;
    pl->count = 0;
    pl->capacity = 0;
}

/* Расширение динамической библиотеки зависит от системы */
static int is_library(const char *name)
{
    size_t len = strlen(name);

    if (len > 3 && strcmp(name + len - 3, ".so") == 0)
        return 1;
    if (len > 6 && strcmp(name + len - 6, ".dylib") == 0)
        return 1;
    return 0;
}

/* Для qsort: имена файлов сортируются, чтобы порядок пунктов меню
 * не зависел от порядка выдачи readdir */
static int cmp_names(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int list_grow(PluginList *pl)
{
    size_t cap = pl->capacity == 0 ? 8 : pl->capacity * 2;
    Plugin *items = realloc(pl->items, cap * sizeof(Plugin));

    if (items == NULL)
        return -1;
    pl->items = items;
    pl->capacity = cap;
    return 0;
}

/* Открывает одну библиотеку и достаёт из неё описание операции.
 * Возвращает 0 при успехе, -1 если библиотека не подошла. */
static int load_one(PluginList *pl, const char *dir, const char *file)
{
    char path[512];
    void *handle;
    Plugin p;

    snprintf(path, sizeof(path), "%s/%s", dir, file);

    handle = dlopen(path, RTLD_NOW);
    if (handle == NULL) {
        fprintf(stderr, "  %s: не открылась (%s)\n", file, dlerror());
        return -1;
    }

    const char **name = dlsym(handle, CALC_SYM_NAME);
    const char **sym  = dlsym(handle, CALC_SYM_SYMBOL);
    const int   *ar   = dlsym(handle, CALC_SYM_ARITY);
    CalcFn       fn   = (CalcFn)dlsym(handle, CALC_SYM_APPLY);

    if (name == NULL || sym == NULL || ar == NULL || fn == NULL) {
        fprintf(stderr, "  %s: нет нужных символов, пропущена\n", file);
        dlclose(handle);
        return -1;
    }

    if (pl->count == pl->capacity && list_grow(pl) != 0) {
        dlclose(handle);
        return -1;
    }

    p.handle = handle;
    p.name = *name;
    p.symbol = *sym;
    p.arity = *ar;
    p.apply = fn;
    snprintf(p.file, sizeof(p.file), "%s", file);

    pl->items[pl->count++] = p;
    return 0;
}

int plugins_load(PluginList *pl, const char *dir)
{
    DIR *d;
    struct dirent *e;
    char **names = NULL;
    size_t n = 0, cap = 0;

    d = opendir(dir);
    if (d == NULL)
        return -1;

    /* Сначала собираем имена, потом сортируем: readdir выдаёт файлы
     * в порядке файловой системы, а меню должно быть стабильным. */
    while ((e = readdir(d)) != NULL) {
        if (!is_library(e->d_name))
            continue;
        if (n == cap) {
            size_t nc = cap == 0 ? 8 : cap * 2;
            char **tmp = realloc(names, nc * sizeof(char *));
            if (tmp == NULL)
                break;
            names = tmp;
            cap = nc;
        }
        names[n] = strdup(e->d_name);
        if (names[n] == NULL)
            break;
        n++;
    }
    closedir(d);

    qsort(names, n, sizeof(char *), cmp_names);

    for (size_t i = 0; i < n; i++) {
        load_one(pl, dir, names[i]);
        free(names[i]);
    }
    free(names);

    return (int)pl->count;
}

void plugins_free(PluginList *pl)
{
    for (size_t i = 0; i < pl->count; i++)
        dlclose(pl->items[i].handle);
    free(pl->items);
    plugins_init(pl);
}

const char *calc_strerror(CalcStatus st)
{
    switch (st) {
    case CALC_OK:       return "успешно";
    case CALC_DIV_ZERO: return "деление на ноль";
    case CALC_DOMAIN:   return "аргумент вне области определения";
    case CALC_RANGE:    return "результат слишком велик (переполнение)";
    default:            return "неизвестная ошибка";
    }
}
