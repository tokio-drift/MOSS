#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include "./constants.h"

typedef struct
{
    enum ball_type ball_type;
    int speed;
    int extra;
} delivery_event;

typedef struct
{
    int runs;
    bool aerial;
    bool wicket;
} shot_result;

typedef struct
{
    int score;
    int wickets;
    int overs;
    int balls;
    int extras;
    int target;
    bool innings;
    int match_intensity;
} scoreboard;

/* played states */
#define PLAYER_DNB     0
#define PLAYER_BATTING 1
#define PLAYER_OUT     2

typedef struct
{
    int id;
    int bowling_skill;
    int fielding_skill;
    int batting_skill;
    bool bowler_type;  /* 0 = pace, 1 = spin */
    int batsmen_type;  /* 0 = opener, 1 = anchor, 2 = finisher, 3 = allrounder */

    int overs_bowled;
    int runs_conceded;
    int wickets_taken;
    int runs_scored;
    int balls_faced;
    int played;        /* PLAYER_DNB / PLAYER_BATTING / PLAYER_OUT */
} player;

#endif