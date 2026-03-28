#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "../include/runout.h"
#include "../include/players.h"
#include "../include/scheduler.h"
#include "../include/scoreboard.h"

pthread_mutex_t end_mutexes[NUM_ENDS];
static resource_request requests[TEAM_SIZE]; 
static int request_count = 0;

void init_runout() {
    for (int i = 0; i < NUM_ENDS; i++) {
        pthread_mutex_init(&end_mutexes[i], NULL);
    }
    request_count = 0;
}

void destroy_runout() {
    for (int i = 0; i < NUM_ENDS; i++) {
        pthread_mutex_destroy(&end_mutexes[i]);
    }
}

bool acquire_end(int batsman_id, int end_id) {

    requests[request_count].batsman_id = batsman_id;
    requests[request_count].requested_end = end_id;
    requests[request_count].held_end = (end_id == END_1) ? END_2 : END_1; // Assuming they hold the opposite
    request_count++;

    if (detect_deadlock()) {
        handle_runout(batsman_id);
        request_count--; // Remove request
        return false;
    }

    pthread_mutex_lock(&end_mutexes[end_id]);
    return true;
}

void release_end(int batsman_id, int end_id) {
    pthread_mutex_unlock(&end_mutexes[end_id]);
    
    for (int i = 0; i < request_count; i++) {
        if (requests[i].batsman_id == batsman_id) {
            for (int j = i; j < request_count - 1; j++) {
                requests[j] = requests[j + 1];
            }
            request_count--;
            break;
        }
    }
}
