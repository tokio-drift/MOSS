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

void init_scheduler(void);
void set_scheduling_policy(int policy);

int select_next_bowler(player team[], int n);
int schedule_rr(player team[], int n);
int schedule_priority(player team[], int n, scoreboard *match);
int schedule_sjf(player team[], int n);

void init_batting_order(void);


int on_wicket(void);


int on_wicket_nonstriker(void);

void swap_strike(void);
int  get_striker(void);
int  get_non_striker(void);

void end_over(player team[], int n);
void update_match_intensity(scoreboard *match);

#endif 