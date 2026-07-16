#include "calc.h"
#include "command.h"
#include "ui.h"

/* Состав команд калькулятора: порядок регистрации задаёт порядок в меню */
static void register_commands(CommandTable *t)
{
    ct_init(t);

    ct_add_binary(t, "Сложение",             "+",    calc_add);
    ct_add_binary(t, "Вычитание",            "-",    calc_sub);
    ct_add_binary(t, "Умножение",            "*",    calc_mul);
    ct_add_binary(t, "Деление",              "/",    calc_div);
    ct_add_binary(t, "Возведение в степень", "^",    calc_pow);
    ct_add_binary(t, "Остаток от деления",   "%",    calc_mod);
    ct_add_unary (t, "Квадратный корень",    "sqrt", calc_sqrt);
    ct_add_nary  (t, "Сумма чисел",          "sum",  calc_sum);
    ct_add_nary  (t, "Среднее арифметическое", "avg", calc_avg);
}

int main(void)
{
    CommandTable table;

    register_commands(&table);
    ui_run(&table);
    return 0;
}
