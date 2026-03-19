#ifndef FIELDER_H
#define FIELDER_H

#include <pthread.h>
#include <stdbool.h>
#include <x86_64-linux-gnu/bits/pthreadtypes.h>
#include "types.h"
#include "constants.h"

extern int active_fielder_id;

/* whether catch has already been taken */
extern bool catch_taken;

extern pthread_mutex_t fielder_mutex;

extern pthread_cond_t fielder_cond[TEAM_SIZE];

/* initialize fielder system */
void init_fielders();

void notify_fielder(int fielder_id, bool aerial);

void *fielder_thread(void *arg);
int select_fielder(player fielding_team[], int n);

/* attempt catch (returns true if catch taken) */
bool attempt_catch(player *fielder, bool aerial);
void reset_fielder_state();


#endif