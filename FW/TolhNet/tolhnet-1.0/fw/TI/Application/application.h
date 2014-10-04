/**
  ******************************************************************************
  * @file    Application/application.h
  * @author  Michele Alessandrini, Giorgio Biagetti
  * @version V1.0.0
  * @date    21-May-2013
  * @brief   Application layer header file.
  ******************************************************************************
  */

#ifndef __APPLICATION_H
#define __APPLICATION_H

#include "../Network/network.h"

/* Network event callback definition */
void network_event_handler (enum nwk_events event_type);

/* Function used to signal data reception */
void data_received (uint16_t src_address, uint16_t sequence, int8_t code, void *data, size_t length);

/* Function used to initialize board hardware */
void app_init (void);

#endif
