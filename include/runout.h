#ifndef RUNOUT_H
#define RUNOUT_H

#include <pthread.h>
#include <stdbool.h>
#include "types.h"
#include "constants.h"

extern pthread_mutex_t end_mutexes[NUM_ENDS];   

typedef struct {
    int batsman_id;
    int requested_end;
    int held_end;
} resource_request;

void init_runout(void);

void destroy_runout(void);

int attempt_run(int striker_id, int non_striker_id, int runs);

bool detect_deadlock(void);

#endif