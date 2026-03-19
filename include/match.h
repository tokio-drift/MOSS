#ifndef MATCH_H
#define MATCH_H

#include "types.h"
#include "constants.h"

/* teams */
extern player team1[TEAM_SIZE];
extern player team2[TEAM_SIZE];

/* pointers to current teams */
extern player *batting_team;
extern player *bowling_team;

#endif