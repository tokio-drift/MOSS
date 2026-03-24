#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/types.h"
#include "../include/constants.h"
#include "../include/pitch.h"
#include "../include/scoreboard.h"
#include "../include/sync.h"
#include "../include/scheduler.h"
// Thread arrays
pthread_t bowler_threads[MAX_BOWLERS];
pthread_t batsman_threads[2];
pthread_t fielder_threads[MAX_FIELDERS];
// Player objects
bowler bowlers[MAX_BOWLERS];
batsman batsmen[MAX_BATSMEN];
fielder fielders[MAX_FIELDERS];
// Thread functions (defined elsewhere)
extern void* bowler_thread(void* arg);
extern void* batsman_thread(void* arg);
extern void* fielder_thread(void* arg);
void init_players()
{
    for(int i = 0; i < MAX_BOWLERS; i++)
    {
        bowlers[i].id = i;
        bowlers[i].skill_level = rand() % 100;
        bowlers[i].overs_bowled = 0;
        bowlers[i].wickets_taken = 0;
    }
    for(int i = 0; i < MAX_BATSMEN; i++)
    {
        batsmen[i].id = i;
        batsmen[i].skill_level = rand() % 100;
        batsmen[i].runs_scored = 0;
        batsmen[i].balls_faced = 0;
    }
    for(int i = 0; i < MAX_FIELDERS; i++)
    {
        fielders[i].id = i;
        fielders[i].catch_level = rand() % 100;
        fielders[i].field_level = rand() % 100;
    }
}
void create_threads()
{
    // Create bowler threads
    for(int i = 0; i < MAX_BOWLERS; i++)
    {
        pthread_create(&bowler_threads[i], NULL, bowler_thread, &bowlers[i]);
    }
    // Create batsmen (only 2 active at start)
    for(int i = 0; i < 2; i++)
    {
        pthread_create(&batsman_threads[i], NULL, batsman_thread, &batsmen[i]);
    }
    // Create fielders
    for(int i = 0; i < MAX_FIELDERS; i++)
    {
        pthread_create(&fielder_threads[i], NULL, fielder_thread, &fielders[i]);
    }
}
void join_threads()
{
    for(int i = 0; i < MAX_BOWLERS; i++)
    {
        pthread_join(bowler_threads[i], NULL);
    }
    for(int i = 0; i < 2; i++)
    {
        pthread_join(batsman_threads[i], NULL);
    }
    for(int i = 0; i < MAX_FIELDERS; i++)
    {
        pthread_join(fielder_threads[i], NULL);
    }
}
int main()
{
    srand(time(NULL));
    printf("=== T20 Match Simulation Started ===\n");
    // Initialize shared systems
    init_sync();        // mutex + cond vars
    init_pitch();       // pitch buffer
    init_scoreboard();  // score, wickets, etc.
    init_scheduler();   // bowling order
    // Initialize players
    init_players();
    // Create threads
    create_threads();
    // Start match (optional barrier / signal)
    start_match();
    // Wait for match to end
    wait_for_match_end();
    // Join threads
    join_threads();
    // Final result
    print_final_score();
    // Cleanup
    destroy_sync();
    printf("=== Match Finished ===\n");
    return 0;
}