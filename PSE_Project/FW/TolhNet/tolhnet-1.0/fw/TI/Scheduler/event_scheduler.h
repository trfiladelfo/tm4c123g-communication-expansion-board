/**
  ******************************************************************************
  * @file    EventScheduler/event_scheduler.h
  * @author  Francesco Ricciardi
  * @version V1.0.0
  * @date    14-June-2013
  * @brief   Event scheduler library header file.
  ******************************************************************************
  * @attention
  *
  * This library realize an event scheduler. We should call the function
  * "event_scheduler_init()" to realize initialization of the timer.
  *
  *
  ******************************************************************************
  * <h2><center>&copy; COPYRIGHT 2013 Cedar Solutions</center></h2>
  ******************************************************************************
  */

#ifndef __EVENT_SCHEDULER_H
#define __EVENT_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SCHEDULABLE_EVENT 10


/* Timer initializazion function */
void event_scheduler_init();


/* Function that realize event scheduling */
void event_schedule(uint16_t taskBit, uint16_t timespan);

/* Function that remove an event by event_id from the scheduling queue */
void remove_event_schedule(uint16_t taskBit);


/* Function that realize timer ISR */
void scheduler_timer_ISR();


/**************PRIVATE FUNCTION******************/

/*
 * This function realize, in MODULO-16 arithmetic,
 * a comparison between a and b. Return 1 if
 * a is greater than b, -1 if a is less than b
 * and 0 if they are equal.
 * */
int8_t a_vs_b(uint16_t a, uint16_t b);





#endif
