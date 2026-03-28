#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <time.h>
#include "./constants.h"

typedef struct
{
    enum ball_type ball_type;
    int speed;
    int extra;
    struct timespec bowled_at;
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

#define PLAYER_DNB     0
#define PLAYER_BATTING 1
#define PLAYER_OUT     2

#define BTYPE_TOP    0
#define BTYPE_MIDDLE 1
#define BTYPE_TAIL   2

typedef struct
{
    int  id;
    char name[32];
    bool is_keeper;

    int  bowling_skill;
    int  fielding_skill;
    int  batting_skill;
    bool bowler_type;
    int  batsmen_type;

    int  overs_bowled;
    int  runs_conceded;
    int  wickets_taken;
    int  runs_scored;
    int  balls_faced;
    int  played;
} player;

#endif