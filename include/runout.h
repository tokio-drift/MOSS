#ifndef RUNOUT_H
#define RUNOUT_H

#include <pthread.h>
#include <stdbool.h>
#include "types.h"

#define NUM_ENDS 2
#define END_1    0   /* striker's end (batting crease)     */
#define END_2    1   /* non-striker's end (bowling crease) */

extern pthread_mutex_t end_mutexes[NUM_ENDS];

void init_runout(void);

void destroy_runout(void);


int attempt_run(int striker_id, int non_striker_id, int runs,
                bool *runout_striker, int *actual_runs);

bool detect_deadlock(void);

#endif 