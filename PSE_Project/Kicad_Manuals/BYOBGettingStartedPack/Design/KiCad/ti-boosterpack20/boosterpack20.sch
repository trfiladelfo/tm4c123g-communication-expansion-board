EESchema Schematic File Version 2
LIBS:boosterpack
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
LIBS:boosterpack20-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date "11 sep 2013"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_3 J5
U 1 1 5080A33C
P 9350 6750
F 0 "J5" V 9300 6750 50  0000 C CNN
F 1 "CONN_3" V 9400 6750 40  0000 C CNN
F 2 "" H 9350 6750 60  0001 C CNN
F 3 "" H 9350 6750 60  0001 C CNN
	1    9350 6750
	1    0    0    1   
$EndComp
Wire Wire Line
	9000 6650 8750 6650
Wire Wire Line
	9000 6850 8750 6850
$Comp
L VCC #PWR01
U 1 1 5080A56F
P 8750 6850
F 0 "#PWR01" H 8750 6950 30  0001 C CNN
F 1 "VCC" H 8750 6950 30  0000 C CNN
F 2 "" H 8750 6850 60  0001 C CNN
F 3 "" H 8750 6850 60  0001 C CNN
	1    8750 6850
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR02
U 1 1 5080A57E
P 8750 6650
F 0 "#PWR02" H 8750 6650 30  0001 C CNN
F 1 "GND" H 8750 6580 30  0001 C CNN
F 2 "" H 8750 6650 60  0001 C CNN
F 3 "" H 8750 6650 60  0001 C CNN
	1    8750 6650
	0    1    1    0   
$EndComp
Wire Wire Line
	8850 6750 8850 6650
Connection ~ 8850 6650
Wire Wire Line
	9000 6750 8850 6750
$Comp
L GND #PWR03
U 1 1 5080AA99
P 9900 5800
F 0 "#PWR03" H 9900 5800 30  0001 C CNN
F 1 "GND" H 9900 5730 30  0001 C CNN
F 2 "" H 9900 5800 60  0001 C CNN
F 3 "" H 9900 5800 60  0001 C CNN
	1    9900 5800
	0    1    1    0   
$EndComp
$Comp
L VCC #PWR04
U 1 1 5080AA9F
P 9900 4250
F 0 "#PWR04" H 9900 4350 30  0001 C CNN
F 1 "VCC" H 9900 4350 30  0000 C CNN
F 2 "" H 9900 4250 60  0001 C CNN
F 3 "" H 9900 4250 60  0001 C CNN
	1    9900 4250
	0    -1   -1   0   
$EndComp
$Comp
L TI_BOOSTER_40_J1 J1
U 1 1 5080DB5C
P 10500 4700
F 0 "J1" H 10450 5350 60  0000 C CNN
F 1 "TI_BOOSTER_20_J1" H 10500 4050 60  0000 C CNN
F 2 "" H 10500 4700 60  0001 C CNN
F 3 "" H 10500 4700 60  0001 C CNN
	1    10500 4700
	1    0    0    -1  
$EndComp
$Comp
L TI_BOOSTER_40_J2 J2
U 1 1 5080DBF4
P 10500 6250
F 0 "J2" H 10450 6900 60  0000 C CNN
F 1 "TI_BOOSTER_20_J2" H 10500 5600 60  0000 C CNN
F 2 "" H 10500 6250 60  0001 C CNN
F 3 "" H 10500 6250 60  0001 C CNN
	1    10500 6250
	1    0    0    -1  
$EndComp
$EndSCHEMATC
