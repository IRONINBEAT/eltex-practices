#ifndef UI_H
#define UI_H

#include "pqueue.h"

void ui_run(PriorityQueue *q);

void ui_simulate(PriorityQueue *q, unsigned long n, unsigned long seed);

#endif /* UI_H */
