#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include "../include/runout.h"
#include "../include/players.h"
#include "../include/scheduler.h"
#include "../include/scoreboard.h"
#include "../include/match.h"

pthread_mutex_t end_mutexes[NUM_ENDS];

static pthread_mutex_t runout_mutex = PTHREAD_MUTEX_INITIALIZER;
static int batsman_end[TEAM_SIZE];

void init_runout(void)
{
    pthread_mutex_init(&end_mutexes[0], NULL);
    pthread_mutex_init(&end_mutexes[1], NULL);

    pthread_mutex_lock(&runout_mutex);
    memset(batsman_end, -1, sizeof(batsman_end));
   
    batsman_end[0] = END_1;
    batsman_end[1] = END_2;
    pthread_mutex_unlock(&runout_mutex);
}

void destroy_runout(void)
{
    pthread_mutex_lock(&runout_mutex);
    memset(batsman_end, -1, sizeof(batsman_end));
    pthread_mutex_unlock(&runout_mutex);
}

int attempt_run(int sid, int nsid, int runs,
                bool *runout_striker, int *actual_runs)
{
    int prob;
    switch (runs)
    {
        case 1:  prob = 8; break;
        case 2:  prob = 4; break;
        default: prob = 3; break;  
    }

    bool fires = (rand() % 100 < prob);

    pthread_mutex_lock(&runout_mutex);

    if (fires)
    {
        int completed = runs - 1;
        *actual_runs  = completed;

        bool striker_out = (rand() % 100 < 60);
        *runout_striker  = striker_out;

        if (completed % 2 == 1)
        {
            batsman_end[sid]  = END_2;
            batsman_end[nsid] = END_1;
        }

        int victim = striker_out ? sid : nsid;

        pthread_mutex_unlock(&runout_mutex);

        return victim;
    }

    *actual_runs = runs;
    *runout_striker = false;   

    if (runs % 2 == 1)
    {
        batsman_end[sid]  = END_2;
        batsman_end[nsid] = END_1;
    }

    pthread_mutex_unlock(&runout_mutex);
    return -1;  
}
bool detect_deadlock(void)
{
    return false;
}