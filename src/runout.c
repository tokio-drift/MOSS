#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "../include/runout.h"
#include "../include/players.h"
#include "../include/scheduler.h"
#include "../include/scoreboard.h"

pthread_mutex_t end_mutexes[NUM_ENDS];

static pthread_mutex_t runout_mutex = PTHREAD_MUTEX_INITIALIZER;
static int batsman_end[TEAM_SIZE];   /* indexed by player id */

void init_runout(void)
{
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

int attempt_run(int striker_id, int non_striker_id, int runs)
{
    if (runs <= 0 || runs == 4 || runs == 6) {
        return -1;
    }

    int runout_prob;
    if (runs == 1) runout_prob = 8;
    else if (runs == 2) runout_prob = 4;
    else                runout_prob = 3;   

    bool runout_fires = (rand() % 100 < runout_prob);

    pthread_mutex_lock(&runout_mutex);

    int victim = -1;

    if (runout_fires)
        victim = (runs % 2 == 1) ? striker_id : non_striker_id;

    if (runs % 2 == 1) {
        batsman_end[striker_id]     = END_2;
        batsman_end[non_striker_id] = END_1;
    }

    pthread_mutex_unlock(&runout_mutex);

    if (runout_fires && victim >= 0)
    {
        pthread_mutex_lock(&score_mutex);
        player *bat = &batting_team[victim];
        if (bat->played != PLAYER_OUT)
        {
            mark_batsman_out(bat);
            match.wickets++;
        }
        pthread_mutex_unlock(&score_mutex);

        printf("  [RUN OUT] %s - direct hit, short of the crease!\n",
               batting_team[victim].name);
        return victim;
    }

    // strike swap for odd runs
    if (runs % 2 == 1)
        swap_strike();

    return -1;
}

bool detect_deadlock(void)
{
    return false;
}