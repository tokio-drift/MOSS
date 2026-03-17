#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef struct
{
    int ball_type;
    int speed;
    int extra; // shows type of extra
} delivery_event;

typedef struct
{
    int runs;
    bool aerial; // 0 for in ground and 1 for air
    bool wicket; // 0 if no wicket else 1
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

typedef struct
{
    int id;
    int bowling_skill;
    int fielding_skill;
    int batting_skill;
    bool bowler_type; // 0 for pace and 1 for spin.
    int batsmen_type; // 0 for top-order, 1 for middle-order, 2 for lower-order.
    int overs_bowled;
    int runs_conceded;
    int wickets_taken;
    int runs_scored;
    int balls_faced;
    int played; // DNB, Playing, Out
} player;
#endif