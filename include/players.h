#ifndef PLAYERS_H
#define PLAYERS_H

#include <stdio.h>
#include "types.h"

shot_result play_shot(player *, player *, delivery_event);
void log_event(FILE *, player *, player *, delivery_event, shot_result, int, int);
delivery_event generate_delivery(player *);

/* Batsman thread arg: slot index (0 = striker slot, 1 = non-striker slot) */
void *batsman_thread(void *arg);
void *bowler_thread(void *arg);
void *fielder_thread(void *arg);

#endif