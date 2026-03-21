#ifndef PLAYERS_H
#define PLAYERS_H
#include <stdio.h>
#include "types.h"

shot_result play_shot(player*, player*, delivery_event);
void log_event(FILE*, player*, player*,
            delivery_event, shot_result,int, int);
delivery_event generate_delivery(player*);

#endif