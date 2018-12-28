# Christmas Tree Watering Node
How to make your Christmas tree to remain green for several weeks? Plenty of water!

Here is a project that will take care of the watering for You. Of course it can run without radio or mysensors but is great for monitoring and remote control (And you han add some extra alarms and safety nets)

But be careful! We are dealing with water on places where we don't want any spill or leaks!

[link]

A simple water sensor to measure the level in the base of the christmas tree. Normally i use etape for working with water levels but i considered it to be a bit too pricy (range 40-60$) for this project.

The level is checked roughly every second but the current is only on for 10ms to decrease the corrosion of the sensor. I think it will last one christmas at least ☺

To fill the base i use a Peristaltic Liquid Pump. You can pick one up from your local Adafruit dealer or from ebay for ~15$. The pump can move 100ml / minute (i.e. like dripping once every two seconds or so) which is a very nice speed for this type of project. The sound level is like a brand new sewing machine so it's not supernoisy but nut supersilent either. I use a 5liter bucket with lid and all tubes are connected under the lid so if one gets disconnected it should stay in the bucket. Then both the tube and the water sensor is striped or taped to the tree.

In my setup the pump works for just above one minute so it has then filled up around 100ml (but that depends on the thresholds you set of course)

You can see the sensor box below and it's pretty simple don't think there is a need for schematics. Use a diode and a TIP120 to control the DC motor like below (I have a step up circuit from 5V to 12V)

0_1545478676207_IMG_20181220_211319.jpg
0_1545478544563_tip120-solenoid.png

If you hook up the sensor to your Homeautomation you should be able to monitor the sensor in action. You can then also take measures if the fill state (on) is longer than expected and raise an alarm and possibly to stop the watering all together.

0_1545484636146_63bd7c01-d29e-460c-b2d4-2f65387a12cb-image.png

As you can see the "normal" periodicity for me is watering every 3-4 hours and it fills for 1 to 1½ minute.

0_1545484452499_chart.jpeg

Wrap everything up in a nice present and enjoy! 