/*
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * http://www.mysensors.org/build/relay
 */

#define MY_DEBUG
#define MY_RADIO_RFM69
#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY RFM69_433MHZ // RFM69_433MHZ for development branch, RF69_433MHZ for master
#define MY_RFM69_NEW_DRIVER 

#define MY_REPEATER_FEATURE
#define MY_NODE_ID 20
#define MY_RFM69_NETWORKID (170)

#define CHILD_ID_FEED_AMOUNT 2
#define CHILD_ID_FEED_TOTAL  3
#define FEED_PIN             8

#include <MySensors.h>

static bool feedNow = false;
static int feedAmount = 10;
static float feedTotal = 0;

#define FORWARD 1
#define REVERSE 2
MyMessage volumeMsg(CHILD_ID_FEED_TOTAL,V_VOLUME);
MyMessage lastAmountMsg(CHILD_ID_FEED_AMOUNT,V_PERCENTAGE);
void setup()  
{  
  digitalWrite(FEED_PIN, LOW);
  pinMode(FEED_PIN, OUTPUT);
     
  // Fetch last known feeding amount from controller
  request(CHILD_ID_FEED_AMOUNT, V_PERCENTAGE);

}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Fishfeeder", "2.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_FEED_AMOUNT, S_DIMMER);
  present(CHILD_ID_FEED_TOTAL, S_WATER);
}


//  Check if digital input has changed and send in new value
void loop() 
{
    if(feedNow) {
   //   feed(3, REVERSE);
  //    feed(3, FORWARD);

      feedNow = false;
      feed(feedAmount*10, FORWARD);
      feedTotal += feedAmount;
      send(volumeMsg.set(feedTotal/1000,3));    // Send total value to gw and we need 3 decimals to not loose any digits
    }
}

void feed(int pulses, int dir)
{  
  for(int i = 0; i < pulses; i++)
  {
    digitalWrite(FEED_PIN, LOW);
    delay(20-dir);
    digitalWrite(FEED_PIN, HIGH);
    delay(dir);
  }
  digitalWrite(FEED_PIN, LOW);
}

void receive(const MyMessage &message) {
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }

  if (message.type == V_PERCENTAGE ) {
     
     // Write some debug info
     Serial.print("Feed Amount:");
    feedAmount = message.getInt();
     Serial.println(message.getInt());
   } 

   if (message.type == V_STATUS) {
         Serial.println("Feed Now!: ");
          feedNow = true;
   }
}
