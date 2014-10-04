/**
  ******************************************************************************
  * @file    EventScheduler/event_scheduler.c
  * @author  Francesco Ricciardi
  * @version V1.0.0
  * @date    14-June-2013
  * @brief   Event scheduler source file.
  ******************************************************************************
  * <h2><center>&copy; COPYRIGHT 2013 Cedar Solutions</center></h2>
  ******************************************************************************
  */
#include "event_scheduler.h"
#include "tasks.h"

#include <string.h>


/* Defining event struct and event struct type */
struct event
{
	uint16_t taskBit;
	uint16_t time;
};
typedef struct event event_type;



/* Definition of possible scheduler state */
#define EV_SCHEDULER_NOT_INITIALIZED  0
#define EV_SCHEDULER_READY            1
#define EV_SCHEDULER_ERROR            2

/* Definition of scheduler status variable */
volatile uint8_t scheduler_status = EV_SCHEDULER_NOT_INITIALIZED;



/* Definition of event list */
event_type event_list[MAX_SCHEDULABLE_EVENT];
/* Definition of list entries counter */
volatile uint8_t list_total_entries = 0;

/* List for happened event */
event_type happened_event_list[MAX_SCHEDULABLE_EVENT];
uint8_t  happened_count = 0;



/* Timer initializazion function */
void event_scheduler_init()
{
#if 0
	if(scheduler_status == EV_SCHEDULER_NOT_INITIALIZED)
	{
	/* Configuration of TIM4 timer */
	/* With a system clock of 72 MHz and a prescaler set @ 36000-1
	 * we have a timer clock of 2000Hz, so every period of TIM4 is of 0.5ms.
	 * To achieve an interrupt after Nbyte*10ms we should multiply Nbyte by 20
	 * and set this value inside TIM_Period (see uart_receive_raw_data() function).
	 * */
		/* Setting the update request selection to don't have interrupt
		 * on timer register software update. I.E. during counter reset. */
		TIM_UpdateRequestConfig(USED_TIMER, TIM_UpdateSource_Regular);
		/* Initializion structure */
		TIM_TimeBaseStructInit(&timer_handler_initstructure);
		timer_handler_initstructure.TIM_Period = 65536-1;
		timer_handler_initstructure.TIM_Prescaler = 36000-1;
		timer_handler_initstructure.TIM_ClockDivision = TIM_CKD_DIV1;
		timer_handler_initstructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(USED_TIMER,	&timer_handler_initstructure);

		/* Channel 1 configuration for output compare */
		TIM_OCStructInit(&TIM_OCInitStructure);
		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
		TIM_OCInitStructure.TIM_Pulse = 0;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
		TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
		TIM_OC1Init(USED_TIMER, &TIM_OCInitStructure);

		/* Disable channel preload */
		TIM_OC1PreloadConfig(USED_TIMER, DISABLE);

		/* Disable TIM interrupt */
		TIM_ITConfig( USED_TIMER, TIM_IT_CC1, DISABLE );

		/* Enable timer */
		TIM_Cmd(TIM4, ENABLE);


		/* Enable timer interrupt in NVIC */
#ifdef TIMER_TIM2
		NVIC_EnableIRQ(TIM2_IRQn);
		NVIC_SetPriority(TIM2_IRQn, TIMER_PRIORITY);
#elif defined(TIMER_TIM4)
		NVIC_EnableIRQ(TIM4_IRQn);
		NVIC_SetPriority(TIM4_IRQn, TIMER_PRIORITY);
#endif

		scheduler_status = EV_SCHEDULER_READY;
	} /* End of if(!initialized)*/
#endif
}


/* Function that realize event scheduling */
void event_schedule(uint16_t taskBit, uint16_t timespan)
{
	uint16_t counter = 0;//, c1;
	event_type processing_event;
	uint8_t i;

	/* The time must be positive so the event will be scheduled in future */
	if(timespan == 0) return;

	/* Disable timer interrupt in NVIC */

	/* Get USED_TIMER counter value */
//	counter = USED_TIMER->CNT;

	/* Control if the list is full */
	if(list_total_entries>=MAX_SCHEDULABLE_EVENT)
	{
		/* Enable timer interrupt in NVIC */
		return;
	}

	/* Fill the process event structure with event_id
	 * and its future timestamp */
	processing_event.taskBit  = taskBit;
	processing_event.time = counter + timespan;


	/* Insert the event in the list */
	i=0;
	/* Search the first event that will be happen after processing_event */
	while(i<list_total_entries && a_vs_b(processing_event.time,event_list[i].time)>=0) i++;
	/* Move the event that will happen after processing_event (if there are) */
	if(i<list_total_entries)
		memmove(event_list + i + 1, event_list + i, (list_total_entries - i) * sizeof (struct event));
	/* Increment the event counter */
	list_total_entries++;
	/* Copy the event in the list entry */
	event_list[i] = processing_event;
#if 0
	/* If there is only one event: schedule it. Else if there are more than one control the next event timestamp. */
	if(list_total_entries == 1)
	{
		/* Write the value to the compare register */
		USED_TIMER->CCR1 = processing_event.time;
		/* Read the timer counter again */
		c1 = USED_TIMER->CNT;

		/* Compare C1 with the actual timer compare value */
		if(a_vs_b(c1,USED_TIMER->CCR1) >= 0)
		{
			/* The timer has passed the c1 compare value: call the event */
			scheduler_timer_ISR();
		}

	}
	else if(a_vs_b(USED_TIMER->CCR1,processing_event.time) >= 0)
	{
		/*
		 * The event will happen before the next scheduled event. Schedule ASAP.
		 * */

		/* Write the value to the compare register */
		USED_TIMER->CCR1 = processing_event.time;
		/* Read the timer counter again */
		c1 = USED_TIMER->CNT;
		/* Control if the timer has passed the new compare value */
		if (a_vs_b(c1,USED_TIMER->CCR1) >= 0)
		{
			/* The event is just happened. Call the ISR. */
			scheduler_timer_ISR();
		}

	} /* End of  else if(a_vs_b(USED_TIMER->CCR1,processing_event.timestamp) >= 0 ) */


	/* Enable TIM interrupt */
	if(list_total_entries > 0)
		TIM_ITConfig( USED_TIMER, TIM_IT_CC1, ENABLE );
	else
		TIM_ITConfig( USED_TIMER, TIM_IT_CC1, DISABLE );

#endif
	/* Enable timer interrupt in NVIC */
}


/* Function that remove an event by event_id from the scheduling queue */
void remove_event_schedule(uint16_t taskBit)
{
	uint8_t i;
	/* Disable timer interrupt in NVIC */

	/* Security control */
	if(list_total_entries == 0)
	{
//		TIM_ITConfig( USED_TIMER, TIM_IT_CC1, DISABLE );
		/* Reset the compare register */
//		USED_TIMER->CCR1 = 0;
		/* Enable timer interrupt in NVIC */
		return;
	}


	/* Search the event in the list */
	i=0;
	while(i<list_total_entries && event_list[i].taskBit!=taskBit) i++;


	/* Control if the event is found */
	if(i<list_total_entries)
	{
		/* If the event is found overwrite it */
		memmove(event_list + i, event_list + i + 1, (list_total_entries - i - 1) * sizeof (struct event));
		/* Decrement the event counter */
		list_total_entries--;

		/* If the event is the first in the list (i==0) we should reschedule the first event in
		 * the list here. But we prefer to wait the scheduler_timer_ISR() that realize this anyway.
		 */
	}

	/* Enable timer interrupt in NVIC */
}





/* Function that realize timer ISR */
void scheduler_timer_ISR()
{
//	volatile uint16_t counter, c1, c2;
//	uint8_t i;
//	bool finished;
	/* Disable timer interrupt in NVIC */

	/* Security control */
	if(list_total_entries == 0)
	{
//		TIM_ITConfig( USED_TIMER, TIM_IT_CC1, DISABLE );
		/* Reset the compare register */
//		USED_TIMER->CCR1 = 0;
		/* Enable timer interrupt in NVIC */
		return;
	}


	/* Insert the happened event in the list */
	happened_count = 0;
//	finished = false;
#if 0
	while(!finished)
	{
		/* Read the timer counter value */
		counter = USED_TIMER->CNT;

		i = 0;
		/* Search the "happened" event */
		while(i<list_total_entries && a_vs_b(counter,event_list[i].time)>=0)
		{
			happened_event_list[happened_count++] = event_list[i++];
		}

		/* Reorder the list */
		if(i<list_total_entries)
			memmove(event_list, event_list + i, (list_total_entries - i) * sizeof (struct event));
		/* Decrement the event counter */
		list_total_entries -= i;

		if(list_total_entries==0)
		{
			/* Disable the timer interrupt */
			TIM_ITConfig( USED_TIMER, TIM_IT_CC1, DISABLE );

			/* Reset the compare register */
			USED_TIMER->CCR1 = 0;

			break;
		}

		/* Read the timer counter value */
		c1 = USED_TIMER->CNT;
		/* Compare C1 with the next event timestamp */
		if(a_vs_b(c1,event_list[0].time) >= 0)
		{
			/* The timer has already passed the next event timestamp and we should review the list.
			 * Finished flag remain unchanged.
			 * */
		}
		else
		{
			/* Write the value to the compare register */
			USED_TIMER->CCR1 = event_list[0].time;

			/* Read the timer counter again */
			c2 = USED_TIMER->CNT;
			/* Control if the timer has passed the new compare value */
			if (a_vs_b(c2,USED_TIMER->CCR1) >= 0)
			{
				/* The timer has already passed the compare value and we should review the list.
				 * Finished flag remain unchanged.
				 * */
			}
			else
			{
				/* Everything is ok */
				finished = true;
			}
		}
	} /* End of while(!finished) */

	/* Call the callbacks */
	if(happened_count>0)
	{
		for(i=0;i<happened_count;i++)
		{
			BB_RAM(&tasks1, happened_event_list[i].taskBit) = 1;  // atomically set bit
		}
	}
#endif
	/* Enable timer interrupt in NVIC */
}




/*
 * This function realize, in MODULO-16 arithmetic,
 * a comparison between a and b. Return 1 if
 * a is greater than b, -1 if a is lesser than b
 * and 0 if they are equal.
 * */
int8_t a_vs_b(uint16_t a, uint16_t b)
{
	uint16_t result = a - b;

	if(result==0) return 0;
	if(result>32767)
		return -1;
	else
		return 1;
}









