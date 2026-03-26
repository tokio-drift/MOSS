#ifndef PLAYERS_H
#define PLAYERS_H

#include <stdio.h>
#include "types.h"

shot_result    play_shot(player *, player *, delivery_event);

void log_event(FILE *, player *bowler, player *batsman,
               delivery_event, shot_result,
               int fielder_id, int caught, bool caught_by_keeper);

delivery_event generate_delivery(player *);

void *batsman_thread(void *arg);
void *bowler_thread(void *arg);
void *fielder_thread(void *arg);

#endif