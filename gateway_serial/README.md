# gateway_serial
A [mysensors.org](https://www.mysensors.org/) serial gateway for RFM69HW (20dB @ ~130mA) or RFM69W (13dB @ ~45mA) max transmit power.

I use Arduino pro mini 3.3V @ 8MHz to get the right voltage (RFM69 is sensitive and not 5V tolerant) and a [FT232 adapter](https://www.ebay.com/itm/272329095424) for the USB connection.

BOM:
* https://www.ebay.com/itm/Pro-Mini-ATMEGA328P-5V-16MHz-3-3V-8MHz-Compatible-to-Arduino-PRO-mini/253135496642?hash=item3af00d25c2:m:mu7vfkUp6EGgxFbtG1p2IDQ:rk:1:pf:0
* https://www.ebay.com/itm/RFM69HW-RFM69CW-RFM69W-443Mhz-868Mhz-915Mhz-Wireless-Transceiver-Rfm12b-W/331976853568?hash=item4d4b5cc840:m:mA0YixY-WyExZBEEMyyt4yw:rk:1:pf:0
* https://www.ebay.com/itm/272329095424


![1](https://github.com/epkboan/epkboan.github.io/blob/master/myhome_gateway_serial.jpg?raw=true "MySensors Serial Gateway")

The RFM69 chips are suitible to mount to the backside of the arduino mini pro with ~5mm spacing and just have solid legs between the PCB boards

![2](https://github.com/epkboan/epkboan.github.io/blob/master/garage_sensor_stacking_of_radio.jpg?raw=true "Stacking of Radio")

I'm using Domoticz as my controller running on an RPI 3
