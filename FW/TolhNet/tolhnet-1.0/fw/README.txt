This folder contains the example firmware developed for a Texas Instruments
Tiva™ C Series TM4C123G LaunchPad Evaluation Board (EK-TM4C123GXL)

The TI/pinout.txt file highlights the connections of the external components
used to interface the demo board with the network. For the simplest tests,
just two wires for the UART serial line (+ ground wire) are necessary.

In order to compile the example, download the appropriate TivaWare libraries from TI
and put them under the "libs" folder at this level of the hierarchy.
At least the "driverlib" and "inc" subfolders need to placed there.
Be sure that a file named "libs/driverlib/gcc/libdriver.a" exists,
or compile the libraries if necessary.

When all is set up, go into the TI/gcc-linux folder and just run make.
"make flash" will download the freshly built firmware to the demo board.

Enjoy!
