# Garage Door Opener 
Maybe you have an old motorized garage that you would like to control in a more flexible way? Or maybe you have lost one remote unit or the number of cars are more than the remotes. And after all it sort of defies the purpose of having a garage door opener if you have to leave the car to enter a code on the keylock :)...

Here is a simle solution to pimp your system without having to hack the central control unit for the garage opener (and still be able to use the keypad if you have one of those). It requires you to strip one of your remote units for the garage door system and use a relay to emulate the button push. In my case the remote unit used a 12V battery so i added a step up from 5V to 12V to never have to bother about the battery life. But the battery could have stayed if you don't want the step up. 

I started out using my own built remote unit (a mysensors button node in low powered mode, deep sleep until button is pressed) but i then came across (flic.io) and then i just had to have some. You can then open the garage door at a longer distance from the house (think of all the seconds you don't have to wait in the car before the door is fully up. You are actually making money from the investment ;-)) as it communicates via your phone via bluetooth and the are really small (size of a coin cell *  9mm) and nice looking.

![1](https://github.com/epkboan/epkboan.github.io/blob/master/garage_mysensors.jpg?raw=true "Pic 1")

BOM:
* Arduino Mini pro ~$2
* RFM69HW ~$2
* Step up 5v to 12v ~$3
* Box ~$5
* Relay module ~$3

So you should be able to build this for around $15. Then to get this really nice and user friendly i can recommend getting some flic.io buttons. They will cost you at least $20 each but they are sooo worth it. Stick it to the dashboard of your car, next to the door before you leave the house and you can reuse the buttons for other automation tasks too. Works with Low power Bluetooth together with your phone(s) via an app.
