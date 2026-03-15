#ifndef PITCH_H
#define PITCH_H

#include <pthread.h>
#include "types.h"

#define PITCH_SIZE 6

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