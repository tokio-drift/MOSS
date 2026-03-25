#ifndef MATCH_H
#define MATCH_H

#include "types.h"
#include "constants.h"

extern player team1[TEAM_SIZE];
extern player team2[TEAM_SIZE];

extern player *batting_team;
extern player *bowling_team;

/* set when innings is over so threads can exit */
extern volatile int innings_over;

#endif