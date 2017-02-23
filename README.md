# ADF4351-Arduino-LCDSHIELD

First release of Arduino sketch to drive ADF4351 module from SV1AFN

See sketch for wiring details and other things :)

![Screenshot](image.jpg)


pinout:<br>

Arduino A4 -> 7 (MUX)<BR>
Arduino 13 -> 1 (CLK)<BR>
arduino 11 -> 4 (DATA)<BR>
Arduino 3  -> 3 (LE)<BR>

Remember to use 3.3V as source for the ADF4351 module (pin 7 vcc and pin 5 as GND)and a voltage divider for signals:

![Screenshot](divider.png)

<b>Feature:</b>

-Set Frequency from 34 Mhz to 4400 Mhz<br>
-EEprom saving and readings (automatically on startup on memory 0)<br>


<b>To do :</b>

-Better Interface <br>
-Output power handling <br>
-Rotative Encoder <br>

73
Giorgio IZ2XBZ



