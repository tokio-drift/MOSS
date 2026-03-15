#ifndef TYPES_H
#define TYPES_H

typedef struct
{
    int ball_type;
    int speed;

    int extra;     // 0 = none, 1 = wide, 2 = no_ball, 3 = dead

} delivery_event;

typedef struct
{
    int runs;

    int aerial;     // 0 = grounded, 1 = aerial

    int wicket;

} shot_result;

typedef struct
{
    int id;

    int skill_level;

    int overs_bowled;
    int runs_conceded;
    int wickets_taken;

    int bowler_type;     // pace/spin

} bowler;


typedef struct
{
    int id;

    int skill_level;

    int runs_scored;
    int balls_faced;

    int batsman_type;

    int played;     // 0 = not played, 1 = playing, 2 = out

} batsman;

typedef struct
{
    int id;

    int field_level;
    int catch_level;

} fielder;


typedef struct
{
    batsman batsmen[11];

    bowler bowlers[10];

    fielder fielders[11];

} team;


typedef struct
{
    int score;
    int wickets;

    int overs;
    int balls;

    int target;

    int innings;

    int match_intensity;

} scoreboard;


#endif