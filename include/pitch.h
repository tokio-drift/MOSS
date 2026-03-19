#ifndef PITCH_H
#define PITCH_H

#include <pthread.h>
#include <stdbool.h>
#include <x86_64-linux-gnu/bits/pthreadtypes.h>
#include "types.h"
#include "constants.h"

extern delivery_event pitch_buffer[PITCH_SIZE];

extern int pitch_index; // index where the bowler writes the next ball

extern pthread_mutex_t pitch_mutex; // protecting pitch buffer access

extern pthread_cond_t ball_ready_cond; 
extern pthread_cond_t ball_consumed_cond;
extern bool ball_ready;
extern bool ball_consumed;

void init_pitch();

void reset_pitch();

void pitch_write(delivery_event ball);

delivery_event pitch_read();

#endif