#ifndef RUNOUT_H
#define RUNOUT_H

#include <pthread.h>
#include <stdbool.h>
#include "types.h"
#include "constants.h"

extern pthread_mutex_t end_mutexes[NUM_ENDS];

typedef struct {
    int batsman_id;
    int requested_end; // END_1 or END_2
    int held_end;      // end currently held
} resource_request;

void init_runout(void);
void destroy_runout(void);

bool acquire_end(int batsman_id, int end_id);
void release_end(int batsman_id, int end_id);

// Deadlock detection
bool detect_deadlock(void);

// Runout (kill one process) on deadlock
void handle_runout(int batsman_id);

#endif