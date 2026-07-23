#include <stdio.h>

#include "plugins.h"
#include "ui.h"

int main(int argc, char **argv)
{
    PluginList pl;
    const char *dir = argc > 1 ? argv[1] : PLUGIN_DEFAULT_DIR;
    int n;

    setvbuf(stdout, NULL, _IOLBF, 0);

    plugins_init(&pl);

    printf("Загрузка операций из каталога \"%s\":\n", dir);
    n = plugins_load(&pl, dir);
    if (n < 0) {
        fprintf(stderr, "Не удалось открыть каталог \"%s\".\n"
                        "Соберите библиотеки: make\n", dir);
        return 1;
    }
    if (n == 0) {
        fprintf(stderr, "В каталоге \"%s\" нет подходящих библиотек.\n", dir);
        plugins_free(&pl);
        return 1;
    }

    for (size_t i = 0; i < pl.count; i++)
        printf("  %-22s -> %s (%s)\n", pl.items[i].file,
               pl.items[i].name, pl.items[i].symbol);
    printf("Загружено операций: %d\n", n);

    ui_run(&pl);

    plugins_free(&pl);
    return 0;
}
