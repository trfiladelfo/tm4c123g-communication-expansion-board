/**
  ******************************************************************************
  * @file    DataLink/PLC-MAC/st7580.h
  * @author  Francesco Ricciardi
  * @version V1.0.0
  * @date    15-May-2013
  * @brief   ST7580 Power Line Communication modem header file.
  ******************************************************************************
  *
  ******************************************************************************
  * <h2><center>&copy; COPYRIGHT 2013 Cedar Solutions</center></h2>
  ******************************************************************************
  */

#ifndef __ST7580_H
#define __ST7580_H


/* Command code definitions */

/* Request commands codes */
#define BIO_RESET_REQUEST          0x3C
#define MIB_WRITE_REQUEST          0x08
#define MIB_READ_REQUEST           0x0C
#define MIB_ERASE_REQUEST          0x10
#define PING_REQUEST               0x2C
#define PHY_DATA_REQUEST           0x24
#define DL_DATA_REQUEST            0x50
#define SS_DATA_REQUEST            0x54


/* Confirm commands codes */
#define BIO_RESET_CONFIRM          0x3D
#define MIB_WRITE_CONFIRM          0x09
#define MIB_READ_CONFIRM           0x0D
#define MIB_ERASE_CONFIRM          0x11
#define PING_CONFIRM               0x2D
#define PHY_DATA_CONFIRM           0x25
#define DL_DATA_CONFIRM            0x51
#define SS_DATA_CONFIRM            0x55


/* Error commands codes */
#define BIO_RESET_ERROR            0x3F
#define MIB_WRITE_ERROR            0x0B
#define MIB_READ_ERROR             0x0F
#define MIB_ERASE_ERROR            0x13
#define PHY_DATA_ERROR             0x27
#define DL_DATA_ERROR              0x53
#define SS_DATA_ERROR              0x57
#define CMD_SYNTAX_ERROR           0x36


/* Error data codes */
#define WRONG_PAR_LEN_EC           0x02
#define WRONG_PAR_VAL_EC           0x03
#define BUSY_EC                    0x04
#define THERMAL_ERROR_EC           0x0B
#define GENERAL_ERROR_EC           0xFF


/* Indication commands codes */
#define BIO_RESET_INDICATION       0x3E
#define PHY_DATA_INDICATION        0x26
#define DL_DATA_INDICATION         0x52
#define DL_SNIFFER_INDICATION      0x5A
#define SS_DATA_INDICATION         0x56
#define SS_SNIFFER_INDICATION      0x5E


/* Start of frame code */
#define STX_CODE1                  0x02
#define STX_CODE2                  0x03
#define STATUS_START_CODE          0x3F


/* Acknowledges code */
#define ACK_CODE                   0x06
#define NACK_CODE                  0x15


/* Status byte masks */
#define STATUS_BYTE_CONFIGURATION_MASK  0x01
#define STATUS_BYTE_TRANSMISSION_MASK   0x02
#define STATUS_BYTE_RECEPTION_MASK      0x04
#define STATUS_BYTE_ACTIVE_LAYER_MASK   0x18
#define STATUS_BYTE_OVERCURRENT_MASK    0x20
#define STATUS_BYTE_TEMP_MASK           0xC0

/* Status byte value */
#define STATUS_BYTE_PROPERLY_CONFIGURED 0x01
#define STATUS_BYTE_TRANSMITTING        0x02
#define STATUS_BYTE_RECEIVING           0x04
#define STATUS_BYTE_AL_PHYSICAL         0x00
#define STATUS_BYTE_AL_DATALINK         0x08
#define STATUS_BYTE_AL_SS               0x10
#define STATUS_BYTE_AL_NOT_CONF         0x18
#define STATUS_BYTE_OVERCURRENT         0x20
#define STATUS_BYTE_TEMP_70             0x00
#define STATUS_BYTE_TEMP_70_100         0x40
#define STATUS_BYTE_TEMP_100_125        0x80
#define STATUS_BYTE_TEMP_125            0xC0



#endif
