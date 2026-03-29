#ifndef PLAYERS_H
#define PLAYERS_H

#include <stdio.h>
#include <stdbool.h>
#include "types.h"

shot_result    play_shot(player *batsman, player *bowler, delivery_event ball);

void log_event(FILE *fp,
               player *bowler,
               player *batsman,
               delivery_event ball,
               shot_result r,
               int fielder_id,
               int caught,
               bool caught_by_keeper,
               bool runout,
               bool runout_striker);

delivery_event generate_delivery(player *bowler);

void *batsman_thread(void *arg);
void *bowler_thread(void *arg);
void *fielder_thread(void *arg);

#endif 