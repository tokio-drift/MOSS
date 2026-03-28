#include <stdio.h>
#include <pthread.h>
#include "../include/scoreboard.h"

scoreboard match;
pthread_mutex_t score_mutex;

void init_scoreboard()
{
    pthread_mutex_init(&score_mutex, NULL);
    match.score          = 0;
    match.wickets        = 0;
    match.overs          = 0;
    match.balls          = 0;
    match.extras         = 0;
    match.target         = 0;
    match.innings        = 0;
    match.match_intensity = 0;
}

void update_batsman_stats(player *batsman, int runs, bool is_legal)
{
    batsman->runs_scored += runs;
    if (is_legal) batsman->balls_faced++;
}

void mark_batsman_out(player *batsman)
{
    batsman->played = PLAYER_OUT;
}

void update_bowler_runs(player *bowler, int runs)
{
    bowler->runs_conceded += runs;
}

void update_bowler_ball(player *bowler, bool is_legal)
{
    if (is_legal) bowler->overs_bowled++;
}

void update_bowler_wicket(player *bowler)
{
    bowler->wickets_taken++;
}

void next_ball(bool is_legal_delivery)
{
    if (!is_legal_delivery) return;
    match.balls++;
    if (match.balls == 6)
    {
        match.overs++;
        match.balls = 0;
    }
}

void add_runs(int runs)   { match.score   += runs; }
void add_wicket()         { match.wickets++;        }

void set_target(int runs) { match.target = runs; }

void reset_for_second_innings()
{
    match.innings  = 1;
    match.score    = 0;
    match.wickets  = 0;
    match.overs    = 0;
    match.balls    = 0;
    match.extras   = 0;
}

bool target_chased()
{
    return (match.innings == 1 && match.score >= match.target);
}

bool is_match_over()
{
    if (match.wickets >= 10) return true;
    if (match.overs   >= 20) return true;
    if (target_chased())     return true;
    return false;
}

void get_score(int *runs, int *wickets, int *overs, int *balls)
{
    *runs    = match.score;
    *wickets = match.wickets;
    *overs   = match.overs;
    *balls   = match.balls;
}

void reset_players(player team[], int n)
{
    for (int i = 0; i < n; i++)
    {
        // Only need to reset runtime stats
        team[i].runs_scored   = 0;
        team[i].balls_faced   = 0;
        team[i].wickets_taken = 0;
        team[i].runs_conceded = 0;
        team[i].overs_bowled  = 0;
        team[i].played        = PLAYER_DNB;
    }
}

/* -----------------------------------------------------------------------
 *  Pretty scorecard helpers
 * --------------------------------------------------------------------- */
#define COL_RESET  "\033[0m"
#define COL_BOLD   "\033[1m"
#define COL_CYAN   "\033[36m"
#define COL_YELLOW "\033[33m"
#define COL_GREEN  "\033[32m"
#define COL_RED    "\033[31m"
#define COL_DIM    "\033[2m"
#define COL_WHITE  "\033[97m"

/* format balls-count (stored as raw legal deliveries) to "O.B" string */
static void fmt_overs(int balls, char *buf, int bufsz)
{
    snprintf(buf, bufsz, "%d.%d", balls / 6, balls % 6);
}

void print_batting_card(player team[], int n)
{
    /* header */
    printf(COL_BOLD COL_CYAN);
    printf("  %-16s %-10s %5s %6s %7s\n",
           "Batsman", "Status", "Runs", "Balls", "SR");
    printf(COL_RESET COL_DIM);
    printf("  %-16s %-10s %5s %6s %7s\n",
           "----------------", "----------", "-----", "------", "-------");
    printf(COL_RESET);

    for (int i = 0; i < n; i++)
    {
        player *p = &team[i];

        if (p->played == PLAYER_DNB)
        {
            printf(COL_DIM "  %-16s %-10s %5s %6s %7s\n" COL_RESET,
                   p->name, "DNB", "-", "-", "-");
            continue;
        }

        const char *status_col = (p->played == PLAYER_OUT) ? COL_RED : COL_GREEN;
        const char *status_str = (p->played == PLAYER_OUT) ? "out" : "not out";

        double sr = (p->balls_faced > 0)
                    ? (double)p->runs_scored * 100.0 / p->balls_faced
                    : 0.0;

        printf("  " COL_WHITE "%-16s" COL_RESET " %s%-10s" COL_RESET
               " %5d %6d %6.1f\n",
               p->name, status_col, status_str,
               p->runs_scored, p->balls_faced, sr);
    }
}

void print_bowling_card(player bowlers[], int n)
{
    printf(COL_BOLD COL_YELLOW);
    printf("  %-16s %6s %6s %8s %7s\n",
           "Bowler", "Overs", "Runs", "Wickets", "Econ");
    printf(COL_RESET COL_DIM);
    printf("  %-16s %6s %6s %8s %7s\n",
           "----------------", "------", "------", "--------", "-------");
    printf(COL_RESET);

    for (int i = 0; i < n; i++)
    {
        player *p = &bowlers[i];
        if (p->overs_bowled == 0) continue;   /* didn't bowl */

        char ov_str[16];
        fmt_overs(p->overs_bowled, ov_str, sizeof(ov_str));

        /* economy = runs / overs_as_decimal */
        double overs_dec = p->overs_bowled / 6.0;
        double econ = (overs_dec > 0) ? (double)p->runs_conceded / overs_dec : 0.0;

        const char *wkt_col = (p->wickets_taken >= 3) ? COL_GREEN :
                              (p->wickets_taken >= 1) ? COL_YELLOW : COL_RESET;

        printf("  " COL_WHITE "%-16s" COL_RESET " %6s %6d %s%8d" COL_RESET " %7.2f\n",
               p->name, ov_str,
               p->runs_conceded,
               wkt_col, p->wickets_taken,
               econ);
    }
}