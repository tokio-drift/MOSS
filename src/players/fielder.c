#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../../include/fielder.h"
#include "../../include/match.h"
#include "../../include/constants.h"

int  active_fielder_id = -1;
bool catch_taken       = false;
pthread_mutex_t fielder_mutex;
pthread_cond_t  fielder_cond[TEAM_SIZE];

void init_fielders()
{
    pthread_mutex_init(&fielder_mutex, NULL);
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_cond_init(&fielder_cond[i], NULL);
    active_fielder_id = -1;
    catch_taken       = false;
}

void notify_fielder(int fielder_id, bool aerial)
{
    (void)aerial;
    pthread_mutex_lock(&fielder_mutex);
    active_fielder_id = fielder_id;
    catch_taken       = false;
    pthread_cond_signal(&fielder_cond[fielder_id]);
    pthread_mutex_unlock(&fielder_mutex);
}

int select_fielder(player fielding_team[], int n)
{
    (void)fielding_team;
    return rand() % n;
}

// ! Move catching logic here?
void *fielder_thread(void *arg)
{
    player *f  = (player *)arg;
    int     id = f->id;

    while (1)
    {
        pthread_mutex_lock(&fielder_mutex);
        while (active_fielder_id != id && !innings_over)
            pthread_cond_wait(&fielder_cond[id], &fielder_mutex);
        pthread_mutex_unlock(&fielder_mutex);

        if (innings_over) break;

        // Catch logic and probabilities in batsman thread 
        // Just woken up so cond_wait doesn't block it permanently
    }

    pthread_exit(NULL);
}

void reset_fielder_state()
{
    pthread_mutex_lock(&fielder_mutex);
    active_fielder_id = -1;
    catch_taken       = false;
    // Wake all fielders so none stays stuck after innings ends
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_cond_signal(&fielder_cond[i]);
    pthread_mutex_unlock(&fielder_mutex);
}