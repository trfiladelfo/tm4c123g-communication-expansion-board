// Task manager working out of interrupt context
// author: Michele Alessandrini


#ifndef __TASKS_H
#define __TASKS_H


#include <stdint.h>
#include <stdbool.h>



// variables holding tasks as single bits
extern volatile uint32_t tasks1;  // bits for the event scheduler
extern volatile uint32_t tasks2;  // bits for other sources

typedef void (*EventCallback)(uint16_t);
typedef void (*IrqCallback)(void);


// task definitions (bit number)

// Group of bits set by the event scheduler (tasks1)
#define TASK_PLC_STATE_RECEIVE                0
#define TASK_PLC_STATE_STATUS_BYTE_WAITING    1
#define TASK_PLC_STATE_MODEM_BUSY             2
#define TASK_PLC_STATE_TRANSMIT_TREQ_WAITING  3
#define TASK_PLC_STATE_TRANSMIT_ACK_WAITING   4

#define TASK_UART_RECEIVE                     5

#define TASK_W433_STATE_INITIALIZING          6
#define TASK_W433_STATE_RESETTING             7
#define TASK_W433_STATE_ABORTED               9
#define TASK_W433_STATE_TRANSMIT              10
#define TASK_W433_STATE_RECEIVE               11

#define TASK_EM_STATE_INITIALIZING            12

extern const EventCallback evSchTasks[32];

// Group of bits for other interrupt sources (tasks2)
#define TASK_USART_DETECT_START               0
#define TASK_PLC_DETECT_START                 1
#define TASK_W433_NIRQ_REQUEST                2
#define TASK_USART_DMA_RXTC_ISR               3
#define TASK_USART_DMA_TXTC_ISR               4
#define TASK_PLC_DMA_RXTC_ISR                 5
#define TASK_PLC_DMA_TXTC_ISR                 6
#define TASK_W433_DMA_RXTC_ISR                7
#define TASK_W433_DMA_TXTC_ISR                8
#define TASK_EM_DMA_RXTC_ISR                  9

extern const IrqCallback irqTasks[32];


#endif
