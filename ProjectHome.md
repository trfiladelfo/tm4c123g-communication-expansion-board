

**Table of contents**


# Introduction #
This is an HW/FW project. The goal is to design an expansion board for the Texas Instruments TM4C123G Launchpad with different communication features:
  * CAN;
  * RS-485;
  * RF onboard module;
  * other (UART, I2C, etc.)
The schematic circuit are available in a pdf file [here](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/TM4C123G_CommExpBoard.pdf).
# HW description #
The description of the circuit is divided into several functional blocks:

## Power Supply: TPS62160 ##
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/PSU.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/PSU.png)

The expansion board has an internal regulator U2, [TPS62160](http://www.ti.com/product/TPS62160) to power the Tiva Launchpad motherboard at +3.3V: if the Launchpad is disconnected from USB, the expansion board regulator will be on, otherwise it will be off.
This functionality is provided by a digital transistor (the NPN Q3) that enable the switching regulator U2 (Texas Instruments TPS62160) via [R1](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=1) pull-up resistor.
If the Launchpad is disconnected from USB, the +5V\_TLP line is floating and NPN is off, thus EN pin is digital high and the regulator is ON, otherwise is off.
At meantime, if the Launchpad is disconnected from USB, the PMOS Q1 is on and the current flow from the regulator towards the exp. board and the Launchpad. If the Launchpad is connected to USB, Q1 is off and the regulator U2 is inhibited.

## RS-485 transceiver: SN65HVD11 ##
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/RS-485.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/RS-485.png)

The RS-485 transceiver U3,[SN65HVD11](http://www.ti.com/product/sn65hvd11), is connected to USART3 peripheral of Tiva-Launchpad board's microcontroller through the tx, rx and tx-enable and rx-enable lines.

To the outside, there's a 4PDT slide switch (SW2, used as a 3PST switch) to connect the  120ohm resistor terminal(![R14](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=14)) and the pull-up & pull-down 560 ohm resistors (![R13](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=13) and [R15](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=15)). The pull-up and pull-down resistors form an external fail-safe bias network. These resistors bring the differential voltage in the RS-485 line into a well defined voltage avoiding misleading states in case of open-circuit, short-circuit and idle-bus. In-fact, these conditions can cause conventional receivers to assume random output states when the input signal is zero. Modern transceiver designs include biasing circuits for open-circuit, short-circuit, and idle-bus failsafe, that force the receiver output to a determined state under an loss-of-signal (LOS) condition. The SN65HVD11 is one of these. A drawback of these failsafe designs is their worst-case noise margin of 10 mV only, thus requiring external failsafe circuitry ([R13](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=13) and [R15](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=15)) to increase noise margin for applications in noisy environments. The value for both resistors is a function of the line's noise (560 ohm is a standard value that lead to Va-Vb to 321 mV in LOS condition).
The 120 ohm terminal resistor is needed only if the transceiver is the last or the first in the RS-485 transmission line thus the switch SW2 must be closed only this case. SW2 in close position means that also the fail-safe bias network is connected.

Towards external RS-485 line, there are two resistors ([R16](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=16) and [R17](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=17)) and a Transient Voltage Suppressor (TVS2) helping to suppress overvoltages on the line due to EFT, ESD, surge, etc.

## CAN transceiver: SN65HVD234 ##
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/CAN.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/CAN.png)

The CAN transceiver U1, [SN65HVD234](http://www.ti.com/product/sn65hvd234), is connected to CAN module 0 of TIVA -Launchpad board's microcontroller through the tx, rx, Rs, and enable lines.
Like the RS-485 transceiver, to the outside, there's an SPDT switch (SW1, used as a SPST switch) to connect the  120ohm resistor terminal(RR4) to the CAN bus. However in this case no need of the fail-safe bias network because the CAN bus standard work with only two possible states: dominant and recessive.
The two signal lines of the bus, CANH and CANL, in the quiescent recessive state, are passively biased to ≈ 2.5 V. The dominant state on the bus takes CANH ≈ 1 V higher to ≈ 3.5 V, and takes CANL ≈ 1 V lower to ≈ 1.5 V, creating a typical 2-V differential signal. If the signal is missing, there will be no bad reading.

As in the RS-485 line, in the CAN bus, there are two resistors ([R10](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=10) and [R11](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=11)) and a Transient Voltage Suppressor (TVS1) that help to suppress overvoltages on the line such as EFT, ESD, surge, etc.

## RF module: RFM23B ##
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/RFM23B.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/RFM23B.png)

The expansion board include also an RF module U4,[RFM23B](http://www.hoperf.com/rf/fsk_module/RFM23B.htm) by Hope Microelectronics. The module is connected via the SPI bus and several GPIO to the Tiva-Launchpad microcontroller. Are provided two external pads to connect a piece of wire (the antenna). The module is supplied by 3.3 V and a pull-up resistor [R18](https://code.google.com/p/tm4c123g-communication-expansion-board/source/detail?r=18) is provided to maintain the module in off state if it is not used.
See [RFM23B datasheet](http://www.hoperf.com/upload/rf/RFM22B_23B.pdf) for more info.

## Other ##
The board has further minor functionality such as:
### Transient & polarity protections of power supply rail ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/Transient_revPol_prot.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/Transient_revPol_prot.png)

This network provide over-voltages protection thanks to TVS3, a big TVS working at 12 V. Higher (bidirectional) voltages are clamped at a maximum voltage of 19.9 V preserving the regulator U2.
The polarity protection is provided by the low-Rds PMOS Q2. The bulk diode polarize Vsg to be greather than Vth if there's a +12 V on +12V\_BUS\_UNP thus the Pmos will be ON. Otherwise, with a negative voltage respect to GND at +12V\_BUS\_UNP line, the PMOS remains OFF.

### ADC filter ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/ADC_filter.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/ADC_filter.png)

Since two analog inputs are available to the expansion boards, two first-order anti-aliasing RC filter are placed near the J3 connector. They're two general purpose analog inputs, thus the filters have a cut-off frequency equal to 7.23 MHz.
### Bus voltage monitor ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/Voltage_monitor.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/Voltage_monitor.png)

A simple voltage divider to +12V\_BUS allow to monitor the bus voltage(either CAN and RS-485). Since the divider ratio is 6.236, the maximum measurable voltage is approximately 20.5 V. A 10 nF capacitor filters out the noise and the fluctuation of the bus voltage.
### I2C pull-up resistor ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/4k7_I2C_pull-up.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/4k7_I2C_pull-up.png)

Two I2C bus are available in the GPIO0 connector, thus there are four 4.7 kohm (standard value that could be changed) pull-up resistor to the I2C lines (I2C0\_SDA, I2C0\_SCL,I2C1\_SDA, I2C1\_SCL).

## Connectors ##
The expansion provide:
  * two UART connectors linked to Tiva-Launchpad motherboard;
  * a CAN connector linked to CAN transceiver and to internal switching regulator;
  * an RS-485 connector linked to CAN transceiver and to internal switching regulator;
  * a GPIO connector linked to two Tiva-Launchpad I2C buses, two RFM23B GPIO pins and two Tiva-Launchpad analog inputs.
See following pin-out.
### UART1 ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/UART1.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/UART1.png)

It's a 3x2 male header (2.54 mm pitch)
### UART2 ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/UART2.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/UART2.png)

It's a 3x2 male header (2.54 mm pitch)
### RS-485 ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/RS-485_conn.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/RS-485_conn.png)

It's a 4 contacts terminal block.
### CAN ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/CAN_conn.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/CAN_conn.png)

It's a 4 contacts terminal block.
### GPIO0 ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/GPIO0_conn.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/GPIO0_conn.png)

It's a 5x2 male header (2.54 mm pitch)
### Connectors to Tiva Launchpad ###
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/J1-J4_conn.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/J1-J4_conn.png)

They're four 10x1 (or two 10x2) male stackable header (2.54 mm pitch).

# PCB #
Based on schematic rev. 0.3.5, was designed a 2.0 x 2.3 in PCB. The gerber files are available on [download](https://code.google.com/p/tm4c123g-communication-expansion-board/wiki/Download) section in wiki tab. This is a 3D draft of the board rev 0.5.
![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/TM4C123G_CommExpBoard_3D.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/TM4C123G_CommExpBoard_3D.png)

This is the aspect of the board Rev 1.0, finally mounted on November, 2014.

![https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/mounted_board.png](https://dl.dropboxusercontent.com/u/43091897/HostedFile/tm4c123g-communication-expansion-board/mounted_board.png)

Actually, in [wiki section](https://code.google.com/p/tm4c123g-communication-expansion-board/wiki/Download), is available the last revision of the board: rev 1.1. With this revision have been corrected the wrong footprint pin-out of Q3.

# FW description #
COOMING SOON!!! :)

<a href='http://creativecommons.org/licenses/by-sa/3.0/'><img src='http://i.creativecommons.org/l/by-sa/3.0/88x31.png' alt='Creative Commons License' /></a><br />
TM4C123G Communication Expansion Board by [Luca Buccolini](https://code.google.com/p/tm4c123g-communication-expansion-board/) is licensed under a [Creative Commons Attribution-ShareAlike 3.0 Unported License](http://creativecommons.org/licenses/by-sa/3.0/).

