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
static int batsman_request[TEAM_SIZE]; 

void init_runout(void)
{
    pthread_mutex_init(&end_mutexes[0], NULL);
    pthread_mutex_init(&end_mutexes[1], NULL);

    pthread_mutex_lock(&runout_mutex);
    memset(batsman_end, -1, sizeof(batsman_end));
    memset(batsman_request, -1, sizeof(batsman_request));
   
    batsman_end[0] = END_1;
    batsman_end[1] = END_2;
    pthread_mutex_unlock(&runout_mutex);
}

void destroy_runout(void)
{
    pthread_mutex_lock(&runout_mutex);
    memset(batsman_end, -1, sizeof(batsman_end));
    memset(batsman_request, -1, sizeof(batsman_request));
    pthread_mutex_unlock(&runout_mutex);
}

bool detect_deadlock(void)
{
    for (int i = 0; i < TEAM_SIZE; i++) 
    {
        int requested_res = batsman_request[i];
        
        if (requested_res != -1) 
        {
            int holder = -1;
            for (int j = 0; j < TEAM_SIZE; j++) {
                if (batsman_end[j] == requested_res) {
                    holder = j;
                    break;
                }
            }
            
            if (holder != -1 && holder != i) {
                int holder_requested_res = batsman_request[holder];
                
                if (holder_requested_res != -1) {
                    if (batsman_end[i] == holder_requested_res) {
                        return true; 
                    }
                }
            }
        }
    }
    return false;
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

    if (batsman_end[sid] == -1) {
        batsman_end[sid] = (batsman_end[nsid] == END_1) ? END_2 : END_1;
    }
    if (batsman_end[nsid] == -1) {
        batsman_end[nsid] = (batsman_end[sid] == END_1) ? END_2 : END_1;
    }


    if (fires)
    {
        int completed = runs - 1;
        *actual_runs  = completed;

        if (completed % 2 == 1)
        {
            int temp = batsman_end[sid];
            batsman_end[sid] = batsman_end[nsid];
            batsman_end[nsid] = temp;
        }

        int target_sid  = (batsman_end[sid] == END_1) ? END_2 : END_1;
        int target_nsid = (batsman_end[nsid] == END_1) ? END_2 : END_1;
        
        batsman_request[sid]  = target_sid;
        batsman_request[nsid] = target_nsid;

        if (detect_deadlock()) 
        {
            printf("\n\033[1m\033[31m OS UMPIRE INTERVENTION: DEADLOCK DETECTED! \033[0m\n");
            printf("\033[33m   Circular Wait occurred between Striker and Non-Striker.\033[0m\n");
            printf("\033[33m   Kernel resolving deadlock by killing one process (Run Out).\033[0m\n\n");

            bool striker_out = (rand() % 100 < 60);
            *runout_striker  = striker_out;

            int victim = striker_out ? sid : nsid;

            batsman_request[sid] = -1;
            batsman_request[nsid] = -1;
            
            batsman_end[victim] = -1;

            pthread_mutex_unlock(&runout_mutex);
            return victim;
        }
    }

    *actual_runs = runs;
    *runout_striker = false;   

    if (runs % 2 == 1)
    {
        int temp = batsman_end[sid];
        batsman_end[sid] = batsman_end[nsid];
        batsman_end[nsid] = temp;
    }

    pthread_mutex_unlock(&runout_mutex);
    return -1;  
}