#ifndef GANTT_H
#define GANTT_H

#include <stdbool.h>
#include "types.h"

#define MAX_GANTT_EVENTS 1200

typedef struct
{
    long long bowled_ns;
    long long consumed_ns;
    int bowler_id;
    int batsman_id;
    char bowler_name[32];
    char batsman_name[32];
    int batsman_type;
    int over;
    int ball;
    int runs;
    bool wicket;
    int innings;
} gantt_event;

extern gantt_event gantt_log[MAX_GANTT_EVENTS];
extern int gantt_count;
extern long long gantt_start_ns;

void gantt_init();
void gantt_record(delivery_event *ball, player *bowler, player *batsman,
                  long long consumed_ns, int over, int ball_num,
                  int runs, bool wicket, int innings);
void gantt_print(const char *sched_name,
                 const char *match_title,
                 int t1_runs, int t1_wkts,
                 int t2_runs, int t2_wkts);

#endif