# Indoor Temperature Sensor
Here is my favorite solution for indoor temperature (or other sensors too) sensor using MySensors framework mysensors.org

The asing is done in Fusion 360 and printed in PLA and is very easy to postprocess to your demands. Key here is the cylindric shape which makes sanding easy. When you finish with wet sanding 1200 paper then it will be very smooth. Here are some samples of some sanded or spray painted sensors. 
![1](https://github.com/boanjo/boanjo.github.io/blob/master/temp_indoor_samples.jpg?raw=true "Pic 1")

Building the sensor. First step is to remove the voltage regulator (If you are using battery powering or the sensor will die in a few days) also remove the power LED. It's quite easy to do with a sharp set of pliers. Or you can solder it off: https://www.mysensors.org/build/battery

![2](https://github.com/boanjo/boanjo.github.io/blob/master/temp_indoor_1_remove.jpg?raw=true "Pic 2")

Next add the step up (and or down) converter and arduino with some hot glue.
![3](https://github.com/boanjo/boanjo.github.io/blob/master/temp_indoor_2_hot_glue.jpg?raw=true "Pic 3")

Prepare the Radio (RFM69CW, the non high power version) wires: https://www.mysensors.org/build/connect_radio
![4](https://github.com/boanjo/boanjo.github.io/blob/master/temp_indoor_3_radio.jpg?raw=true "Pic 4")

Then attach the sensor SI7021 (temp + hum), the resistors and capacitor to measure the battery https://www.mysensors.org/build/battery and then also the home-made antenna http://www.byvac.com/downloads/RLnn/How-to-make-a-Air-Cooled-433MHz-antenna.pdf
![5](https://github.com/boanjo/boanjo.github.io/blob/master/temp_indoor_4_sensor.jpg?raw=true "Pic 5")

Here is the final product that you can put pretty much anywhere
![6](https://github.com/boanjo/boanjo.github.io/blob/master/temp_indoor_window.jpg?raw=true "Pic 6")

Here is a YouTube video showing the setup and powerconsumption (5-35uA in slep mode and ~40mA during transmission) so it's good for low power battery operations.
https://youtu.be/DC6JAksRpOY

Enjoy!