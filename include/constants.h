#ifndef CONSTANTS_H
#define CONSTANTS_H

#define TEAM_SIZE 11
#define PITCH_SIZE 6
#define MAX_BOWLERS 1
#define MAX_FIELDERS 10
#define MAX_BATSMEN 2
#define MAX_SPEED_PACER 160
#define MIN_SPEED_PACER 100
#define MAX_SPEED_SPIN 120
#define MIN_SPEED_SPIN 80
enum ball_type
{
    YORKER,
    BOUNCER,
    LENGTH,
    FULL,
    SLOWER,
    OFF_BREAK,
    LEG_BREAK,
    CARROM,
    GOOGLY,
    FLIGHTED
};
enum extra_type
{
    NO_EXTRA = 0,
    WIDE,
    NO_BALL,
    BYE,
    LEG_BYE
};
#endif