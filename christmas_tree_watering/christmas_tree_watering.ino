#define MY_DEBUG
#define MY_RADIO_RFM69
#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY RFM69_433MHZ // RFM69_433MHZ for development branch, RF69_433MHZ for master
#define MY_RFM69_NEW_DRIVER

#define MY_REPEATER_FEATURE
#define MY_NODE_ID 42

#define MY_RFM69_NETWORKID (170)

#define CHILD_ID_LEVEL 2
#define CHILD_ID_PUMP  3

#include <MySensors.h>

MyMessage levelMsg(CHILD_ID_LEVEL,V_LEVEL);
MyMessage pumpMsg(CHILD_ID_PUMP, V_TRIPPED);

long double HEARTBEAT_TIME = 60000;
long double last_heartbeat_time = millis();
const int read = A0; //Sensor AO pin to Arduino pin A0
const int sensor_enable = 6;

int value = -1;
int readValue;
int prevSentValue = -1;

#define FILL_PIN 8
#define STATE_IDLE 0
#define STATE_FILL 1
int state = STATE_IDLE;

void before()
{
  pinMode(sensor_enable, OUTPUT);
  digitalWrite(sensor_enable, LOW);
  pinMode(FILL_PIN, OUTPUT);
  digitalWrite(FILL_PIN, LOW);
}

void setup()
{

}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Cristmas Tree", "1.0");

  present(CHILD_ID_PUMP, S_BINARY);
  // Register this device as dust sensor as that seems easiest to map
  // to whatever you want (we want adc value)
  present(CHILD_ID_LEVEL, S_DUST);
}


void loop()
{

  // No need to update more frequently than every second (or some more)
  wait(1000);

  // Enable the current in the water (Reduce corrosion compared to
  // continous 3.3V)
  digitalWrite(sensor_enable, HIGH);
  wait(10);
  readValue = analogRead(read);
  digitalWrite(sensor_enable, LOW);
  Serial.print("readValue = ");
  Serial.print(readValue);

  // Do some simple averaging unless first sample
  if(value == -1) {
    value = readValue;
  } else {
    value = readValue + value;
    value = value / 2;
  }
  Serial.print(", value = ");
  Serial.println(value);

  if(state == STATE_IDLE) {
    // The adc reading goes from 0 to ~150 when first in contact with water
    // 150-200 is within the first millimeter. Then the values increase in
    // a more linear manner.
    if(value < 200) {
      state = STATE_FILL;
      digitalWrite(FILL_PIN, HIGH);
      // Send both the level and the on so we can monitor for how long we
      // fill and are idle
      send(pumpMsg.set(state==STATE_FILL));
      send(levelMsg.set(value));
      prevSentValue = value;
    }
  }
  else if(state == STATE_FILL) {
    // Threshold at 360 (which is roughly half way on the mm scale
    // on the sensor ~20mm from empty)
    // Note that since we are taking an average it will take a few seconds
    // until we stop and so the value will be higher ~420
    if(value > 360) {
      state = STATE_IDLE;
      digitalWrite(FILL_PIN, LOW);
      // Send both the level and the on so we can monitor for how long we
      // fill and are idle
      send(pumpMsg.set(state==STATE_FILL));
      send(levelMsg.set(value));
      prevSentValue = value;
    }
  }

  int diff = value - prevSentValue;
  // Allow some jitter
  if(diff < 15 && diff > -15) {

    // no change but send anyway
    long double temp = (millis() - last_heartbeat_time);
    if (temp > HEARTBEAT_TIME) {
      // If it exceeds the heartbeat time then send a level anyway
      last_heartbeat_time = millis();
      send(levelMsg.set(value));
      prevSentValue = value;

    }
  } else {
    send(levelMsg.set(value));
    prevSentValue = value;
  }


}

void receive(const MyMessage &message)
{
  Serial.print("msg");
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_STATUS) {
    // pulse = true;
    Serial.print("Incoming change for sensor:");
  }
}
