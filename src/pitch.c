// src/pitch.c

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include "../include/pitch.h"
#include "../include/match.h"

delivery_event pitch_buffer[PITCH_SIZE];
int pitch_index = 0;

pthread_mutex_t pitch_mutex;
pthread_cond_t ball_ready_cond;
pthread_cond_t ball_consumed_cond;

bool ball_ready    = false;
bool ball_consumed = true;

void init_pitch()
{
    pthread_mutex_init(&pitch_mutex, NULL);
    pthread_cond_init(&ball_ready_cond, NULL);
    pthread_cond_init(&ball_consumed_cond, NULL);
    pitch_index    = 0;
    ball_ready     = false;
    ball_consumed  = true;
}

void reset_pitch()
{
    pthread_mutex_lock(&pitch_mutex);
    pitch_index   = 0;
    ball_ready    = false;
    ball_consumed = true;
    /* Wake anyone blocked so they can re-check innings_over */
    pthread_cond_broadcast(&ball_ready_cond);
    pthread_cond_broadcast(&ball_consumed_cond);
    pthread_mutex_unlock(&pitch_mutex);
}

/* Bowler writes a ball. Blocks until the previous one was consumed. */
void pitch_write(delivery_event ball)
{
    pthread_mutex_lock(&pitch_mutex);
    while (!ball_consumed && !innings_over)
        pthread_cond_wait(&ball_consumed_cond, &pitch_mutex);

    if (innings_over)
    {
        pthread_mutex_unlock(&pitch_mutex);
        return;
    }

    pitch_buffer[pitch_index] = ball;
    pitch_index = (pitch_index + 1) % PITCH_SIZE;
    ball_ready    = true;
    ball_consumed = false;
    pthread_cond_broadcast(&ball_ready_cond);   /* wake striker batsman */
    pthread_mutex_unlock(&pitch_mutex);
}

/* Striker batsman reads the ball. Blocks until one is ready. */
delivery_event pitch_read()
{
    delivery_event ball = {0};
    pthread_mutex_lock(&pitch_mutex);
    while (!ball_ready && !innings_over)
        pthread_cond_wait(&ball_ready_cond, &pitch_mutex);

    if (innings_over)
    {
        pthread_mutex_unlock(&pitch_mutex);
        return ball;
    }

    int read_index = (pitch_index - 1 + PITCH_SIZE) % PITCH_SIZE;
    ball          = pitch_buffer[read_index];
    ball_ready    = false;
    ball_consumed = true;
    pthread_cond_broadcast(&ball_consumed_cond);  /* wake bowler */
    pthread_mutex_unlock(&pitch_mutex);
    return ball;
}