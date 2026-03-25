#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>
#include "types.h"

#define SCHED_RoR       0
#define SCHED_PRIORITY  1
#define SCHED_SJF       2

extern int current_bowler_id;
extern int striker_id;
extern int non_striker_id;
extern int next_batsman_id;
extern int scheduling_policy;

extern pthread_mutex_t scheduler_mutex;

void init_scheduler();
void set_scheduling_policy(int policy);

int select_next_bowler(player team[], int n);
int schedule_rr(player team[], int n);
int schedule_priority(player team[], int n, scoreboard *match);
int schedule_sjf(player team[], int n);

void init_batting_order();
/* on_wicket: sets new striker from remaining batsmen.
   Returns the new striker's player-id, or -1 if all out. */
int on_wicket();
void swap_strike();
int get_striker();
int get_non_striker();

void end_over(player team[], int n);
void update_match_intensity(scoreboard *match);

#endif