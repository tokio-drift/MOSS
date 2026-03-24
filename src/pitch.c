#include <pthread.h>
#include <stdbool.h>
#include "../include/pitch.h"

// Global State
delivery_event pitch_buffer[PITCH_SIZE];
int pitch_index = 0;

pthread_mutex_t pitch_mutex;
pthread_cond_t ball_ready_cond;
pthread_cond_t ball_consumed_cond;

bool ball_ready = false;
bool ball_consumed = true;

void init_pitch()
{
    pthread_mutex_init(&pitch_mutex, NULL);
    pthread_cond_init(&ball_ready_cond, NULL);
    pthread_cond_init(&ball_consumed_cond, NULL);

    pitch_index = 0;
    ball_ready = false;
    ball_consumed = true;
}

void reset_pitch()
{
    pthread_mutex_lock(&pitch_mutex);
    pitch_index = 0;
    pthread_mutex_unlock(&pitch_mutex);
}

// bowler only
void pitch_write(delivery_event ball)
{
    pthread_mutex_lock(&pitch_mutex);
    while (!ball_consumed)
        pthread_cond_wait(&ball_consumed_cond, &pitch_mutex);

    pitch_buffer[pitch_index] = ball;
    pitch_index = (pitch_index + 1) % PITCH_SIZE;
    ball_ready = true;
    ball_consumed = false;
    pthread_cond_signal(&ball_ready_cond);
    pthread_mutex_unlock(&pitch_mutex);
}

// batsman only
delivery_event pitch_read()
{
    pthread_mutex_lock(&pitch_mutex);
    while (!ball_ready)
        pthread_cond_wait(&ball_ready_cond, &pitch_mutex);
    int read_index = (pitch_index - 1 + PITCH_SIZE) % PITCH_SIZE;
    delivery_event ball = pitch_buffer[read_index];
    ball_ready = false;
    ball_consumed = true;
    pthread_cond_signal(&ball_consumed_cond);
    pthread_mutex_unlock(&pitch_mutex);
    return ball;
}