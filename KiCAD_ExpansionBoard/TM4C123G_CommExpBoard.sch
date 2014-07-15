EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:boosterpack
LIBS:cdsot23-sm712
LIBS:dip_switch
LIBS:power_flag
LIBS:TPS62160
LIBS:transceiver
LIBS:tvs
LIBS:bid_tvs
LIBS:TM4C123G_CommExpBoard-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 3
Title "TM4C123G Communication Expansion Board"
Date "15 jul 2014"
Rev "0.3.1"
Comp "Luca Buccolini, Student @ Università Politecnica delle Marche"
Comment1 "Expansion Board for the Texas Instruments TM4C123G Launchpad "
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Sheet
S 6350 1850 2800 3750
U 53959336
F0 "Expansion Board" 50
F1 "Expansion_board.sch" 50
F2 "RS485_B" O L 6350 2000 60 
F3 "RS485_A" O L 6350 2100 60 
F4 "RS485_RXEN#" I L 6350 2800 60 
F5 "RS485_TXEN" I L 6350 2900 60 
F6 "UART3_RX" O L 6350 3000 60 
F7 "UART3_TX" I L 6350 3100 60 
F8 "CAN_H" O L 6350 3500 60 
F9 "CAN_L" O L 6350 3600 60 
F10 "CAN0_EN" I L 6350 4000 60 
F11 "CAN0_RS" I L 6350 4100 60 
F12 "CAN0_RX" O L 6350 3800 60 
F13 "CAN0_TX" I L 6350 3900 60 
F14 "RFM_GPIO1" O L 6350 4450 60 
F15 "RFM_GPIO0" O L 6350 4550 60 
F16 "RFM_SDN" I L 6350 4800 60 
F17 "RFM_MOSI" I L 6350 4900 60 
F18 "RFM_MISO" O L 6350 5000 60 
F19 "RFM_SSEL" I L 6350 5100 60 
F20 "RFM_SCLK" I L 6350 5200 60 
F21 "RFM_CLK" O L 6350 5300 60 
F22 "RFM_NIRQ" O L 6350 5400 60 
$EndSheet
Wire Wire Line
	4750 2000 6350 2000
Wire Wire Line
	6350 2100 4750 2100
Wire Wire Line
	4750 2800 6350 2800
Wire Wire Line
	4750 2900 6350 2900
Wire Wire Line
	6350 3000 4750 3000
Wire Wire Line
	4750 3100 6350 3100
Wire Wire Line
	4750 3500 6350 3500
Wire Wire Line
	6350 3600 4750 3600
Wire Wire Line
	4750 3800 6350 3800
Wire Wire Line
	6350 3900 4750 3900
Wire Wire Line
	4750 4000 6350 4000
Wire Wire Line
	6350 4100 4750 4100
Wire Wire Line
	4750 4450 6350 4450
Wire Wire Line
	6350 4550 4750 4550
Wire Wire Line
	6350 5400 4750 5400
Wire Wire Line
	4750 5300 6350 5300
Wire Wire Line
	6350 5200 4750 5200
Wire Wire Line
	4750 5100 6350 5100
Wire Wire Line
	6350 5000 4750 5000
Wire Wire Line
	4750 4900 6350 4900
Wire Wire Line
	6350 4800 4750 4800
$Sheet
S 2450 1850 2300 3750
U 539592FB
F0 "TM4C123G Connectors" 50
F1 "TM4C123G_connectors.sch" 50
F2 "RS485_A" I R 4750 2100 60 
F3 "RS485_B" I R 4750 2000 60 
F4 "CAN_H" I R 4750 3500 60 
F5 "CAN_L" I R 4750 3600 60 
F6 "RS485_RXEN#" O R 4750 2800 60 
F7 "RS485_TXEN" O R 4750 2900 60 
F8 "UART3_RX" I R 4750 3000 60 
F9 "UART3_TX" O R 4750 3100 60 
F10 "CAN0_RX" I R 4750 3800 60 
F11 "CAN0_TX" O R 4750 3900 60 
F12 "CAN0_EN" O R 4750 4000 60 
F13 "CAN0_RS" O R 4750 4100 60 
F14 "RFM_GPIO0" I R 4750 4450 60 
F15 "RFM_GPIO1" I R 4750 4550 60 
F16 "RFM_SDN" O R 4750 4800 60 
F17 "RFM_MOSI" O R 4750 4900 60 
F18 "RFM_NIRQ" I R 4750 5400 60 
F19 "RFM_MISO" I R 4750 5000 60 
F20 "RFM_SSEL" O R 4750 5100 60 
F21 "RFM_SCLK" O R 4750 5200 60 
F22 "RFM_CLK" I R 4750 5300 60 
$EndSheet
$EndSCHEMATC
