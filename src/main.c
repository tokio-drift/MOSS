#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "../include/types.h"
#include "../include/constants.h"
#include "../include/match.h"
#include "../include/pitch.h"
#include "../include/scoreboard.h"
#include "../include/scheduler.h"
#include "../include/fielder.h"
#include "../include/players.h"

player team1[TEAM_SIZE];
player team2[TEAM_SIZE];

player *batting_team;
player *bowling_team;

volatile int innings_over = 0;

void init_team(player team[], int n)
{
    for (int i = 0; i < n; i++)
    {
        team[i].id             = i;
        team[i].batting_skill  = 40 + rand() % 60;
        team[i].bowling_skill  = 40 + rand() % 60;
        team[i].fielding_skill = 50 + rand() % 50;
        team[i].bowler_type    = rand() % 2;
        team[i].batsmen_type   = rand() % 4;
        team[i].played         = PLAYER_DNB;
        team[i].overs_bowled   = 0;
        team[i].runs_conceded  = 0;
        team[i].wickets_taken  = 0;
        team[i].runs_scored    = 0;
        team[i].balls_faced    = 0;
    }
}

/* Launch one innings.  batting/bowling_team must be set before calling. */
static void play_innings(int innings_num)
{
    innings_over = 0;

    /* Reset pitch state */
    reset_pitch();

    /* Reset batting/bowling orders */
    init_batting_order();
    select_next_bowler(bowling_team, TEAM_SIZE);   /* sets current_bowler_id */

    /* Prepare log file for this innings */
    char logpath[64];
    snprintf(logpath, sizeof(logpath), "../logs/innings%d.txt", innings_num + 1);
    FILE *lf = fopen(logpath, "w");
    if (lf) { fprintf(lf, "=== Innings %d ===\n", innings_num + 1); fclose(lf); }

    printf("\n========================================\n");
    printf("  INNINGS %d  |  Batting: Team %d\n",
           innings_num + 1,
           (batting_team == team1) ? 1 : 2);
    printf("========================================\n");

    /* Slot indices passed to batsman threads */
    int slot0 = 0, slot1 = 1;

    pthread_t bowler_tid;
    pthread_t batsman_tids[2];
    pthread_t fielder_tids[TEAM_SIZE];

    pthread_create(&bowler_tid,       NULL, bowler_thread,  NULL);
    pthread_create(&batsman_tids[0],  NULL, batsman_thread, &slot0);
    pthread_create(&batsman_tids[1],  NULL, batsman_thread, &slot1);

    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_create(&fielder_tids[i], NULL, fielder_thread, &bowling_team[i]);

    /* ---- Poll for innings end ---- */
    while (1)
    {
        pthread_mutex_lock(&score_mutex);
        int over = is_match_over();
        pthread_mutex_unlock(&score_mutex);
        if (over) break;
        struct timespec ts = {0, 1000000}; /* 1 ms */
        nanosleep(&ts, NULL);
    }

    /* Signal all threads to stop */
    innings_over = 1;

    /* Unblock anyone waiting on pitch */
    reset_pitch();

    /* Unblock fielder threads */
    pthread_mutex_lock(&fielder_mutex);
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_cond_broadcast(&fielder_cond[i]);
    pthread_mutex_unlock(&fielder_mutex);

    /* Join everyone */
    pthread_join(batsman_tids[0], NULL);
    pthread_join(batsman_tids[1], NULL);
    pthread_join(bowler_tid,      NULL);
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_join(fielder_tids[i], NULL);

    /* Print scorecard */
    int runs, wickets, overs, balls;
    get_score(&runs, &wickets, &overs, &balls);
    printf("\nInnings %d Result: %d/%d (%d.%d overs)\n",
           innings_num + 1, runs, wickets, overs, balls);

    printf("\n--- Batting Card ---\n");
    print_batting_card(batting_team, TEAM_SIZE);
    printf("\n--- Bowling Card ---\n");
    print_bowling_card(bowling_team, TEAM_SIZE);
}

int main()
{
    srand((unsigned)time(NULL));

    /* Global init */
    init_pitch();
    init_scoreboard();
    init_scheduler();
    init_fielders();

    /* Create logs dir */
    system("mkdir -p ../logs");

    init_team(team1, TEAM_SIZE);
    init_team(team2, TEAM_SIZE);

    /* ============ INNINGS 1: Team 1 bats ============ */
    batting_team = team1;
    bowling_team = team2;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);

    play_innings(0);

    int inn1_runs, inn1_wkts, inn1_overs, inn1_balls;
    get_score(&inn1_runs, &inn1_wkts, &inn1_overs, &inn1_balls);
    int team1_score = inn1_runs;

    /* ============ INNINGS 2: Team 2 bats ============ */
    set_target(team1_score + 1);
    reset_for_second_innings();

    batting_team = team2;
    bowling_team = team1;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);

    /* Reset bowler overs for new innings */
    select_next_bowler(bowling_team, TEAM_SIZE);

    play_innings(1);

    int inn2_runs, inn2_wkts, inn2_overs, inn2_balls;
    get_score(&inn2_runs, &inn2_wkts, &inn2_overs, &inn2_balls);
    int team2_score = inn2_runs;

    /* ============ RESULT ============ */
    printf("\n========================================\n");
    printf("           MATCH RESULT\n");
    printf("========================================\n");
    printf("Team 1: %d/%d (%d.%d overs)\n",
           team1_score, inn1_wkts, inn1_overs, inn1_balls);
    printf("Team 2: %d/%d (%d.%d overs)\n",
           team2_score, inn2_wkts, inn2_overs, inn2_balls);
    printf("----------------------------------------\n");

    if (team2_score > team1_score)
    {
        int wickets_left = 10 - inn2_wkts;
        if (wickets_left < 0) wickets_left = 0;
        printf("Team 2 wins by %d wicket%s!\n",
               wickets_left, wickets_left == 1 ? "" : "s");
    }
    else if (team1_score > team2_score)
    {
        int run_margin = team1_score - team2_score;
        printf("Team 1 wins by %d run%s!\n",
               run_margin, run_margin == 1 ? "" : "s");
    }
    else
    {
        printf("Match tied!\n");
    }
    printf("========================================\n");

    return 0;
}