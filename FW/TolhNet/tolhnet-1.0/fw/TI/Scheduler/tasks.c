// Task manager working out of interrupt context
// author: Michele Alessandrini


#include "tasks.h"
#include "stddef.h"

volatile uint32_t tasks1 = 0;  // bits for the event scheduler
volatile uint32_t tasks2 = 0;  // bits for other sources

/*

void plc_event_cb(uint16_t event_id);

void w433_event_cb(uint16_t event_id)
{
}

const EventCallback evSchTasks[32] = {
	plc_event_cb,
	plc_event_cb,
	plc_event_cb,
	plc_event_cb,
	plc_event_cb,
	NULL,
	w433_event_cb,
	w433_event_cb,
	w433_event_cb,
	w433_event_cb,
	w433_event_cb,
	w433_event_cb,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

*/

/*

const IrqCallback irqTasks[32] = {
	NULL,
	plc_detect_start_ISR,
	w433_nirq_request,
	uart_dma_RXTC_ISR,
	uart_dma_TXTC_ISR,
	plc_dma_RXTC_ISR,
	plc_dma_TXTC_ISR,
	w433_dma_RXTC_ISR,
	w433_dma_TXTC_ISR,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};
*/
