# Attiny85 as a clock pulse
How do you get a periodic interrupt that is determenistic and that doesn't loose track of time due to other interrupts? Well you can use an RTC or you can add a dirt cheap Attiny85 to do the job for you. 

You basically only need to add GND, VCC and use some pin during operation but since you of course need to program the chip i usually put this little chip on a small breadboard and a header so it can slot in (or reprogram easily) 

![1](https://github.com/boanjo/boanjo.github.io/blob/master/attiny85_parts.jpg?raw=true "Pic 1")

From the pinout below you can see the 6 pins that need to be connected while programming. I'm using an UsbTinyISP but there are other ways to program the chip too.

![2](https://github.com/boanjo/boanjo.github.io/blob/master/attiny85_pinout.jpg?raw=true "Pic 2")

You can program the attiny in arduino selecting the use programmer or like in this project i'm using platform.io (the ini file state what programmer to use). I've made a simple adapter board that is reused between projects that just maps the UsbTinyISP mins to the straight 6 pin module.

![3](https://github.com/boanjo/boanjo.github.io/blob/master/attiny85_adapter.jpg?raw=true "Pic 3")

Here is a YouTube video showing the setup and powerconsumption (6.4uA) so it's good for low power battery operations.
https://youtu.be/Sj62Fr1nCek

Enjoy!