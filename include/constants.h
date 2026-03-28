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

#define LOG_DIR   "./logs"
#define LOG_FILE  "./logs/log.txt"
#define GANTT_TXT "./logs/gantt.txt"

// ANSI colors for terminal Gantt
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_TOP     "\033[34m"    // blue   - top order
#define ANSI_MIDDLE  "\033[33m"    // yellow - middle order
#define ANSI_TAIL    "\033[31m"    // red    - tail
#define ANSI_WICKET  "\033[35m"    // magenta - wicket ball
#define ANSI_DIM     "\033[2m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"

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