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
static int batsman_end[TEAM_SIZE];   

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
    if (runs <= 0) return -1;
    int runout_prob;
    if (runs == 1)      runout_prob = 10;
    else if (runs == 2) runout_prob = 6;
    else                runout_prob = 2;

    bool runout_fires = (rand() % 100 < runout_prob);

    pthread_mutex_lock(&runout_mutex);

    int striker_final_end    = (runs % 2 == 1) ? END_2 : END_1;
    int nonstriker_final_end = (runs % 2 == 1) ? END_1 : END_2;

    batsman_end[striker_id]    = striker_final_end;
    batsman_end[non_striker_id] = nonstriker_final_end;

    int victim = -1;

    if (runout_fires)
    {
      
        victim = (runs % 2 == 1) ? striker_id : non_striker_id;

        pthread_mutex_unlock(&runout_mutex);

        pthread_mutex_lock(&score_mutex);
        player *bat = &batting_team[victim];
        if (bat->played != PLAYER_OUT)
        {
            mark_batsman_out(bat);
            match.wickets++;
        }
        pthread_mutex_unlock(&score_mutex);

        printf("  [RUN OUT] %s - caught short of the crease!\n",
               batting_team[victim].name);
        return victim;
    }

    pthread_mutex_unlock(&runout_mutex);

    if (runs % 2 == 1)
        swap_strike();

    return -1;
}

bool detect_deadlock(void)
{
    return false;
}