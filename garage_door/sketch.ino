/*
 * github.com/epkboan/myhome
 * Garage Door controller using mysensors.org
 * Binary switch to remote control the door using wireless remote (delivered
 * with the garage door)
 * Two window/door sensors to establish if the garage is fully open, fully
 * closed or in between.
 * Operation of the garage door (in my case Crawford) is toogle direction
 * and stop at movement in any direction for every push on 1 single button
 */

#define MY_DEBUG
#define MY_RADIO_RFM69
#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY RFM69_433MHZ // RFM69_433MHZ for development branch, RF69_433MHZ for master
#define MY_RFM69_NEW_DRIVER 

#define MY_REPEATER_FEATURE
#define MY_NODE_ID 60

#define MY_RFM69_NETWORKID (170)

#define CHILD_ID_RELAY     2
#define CHILD_ID_DOOR_DOWN 3
#define CHILD_ID_DOOR_UP   4

#define DOOR_TOGGLE_RELAY_PIN 4
#define DOOR_DOWN_PIN         6
#define DOOR_UP_PIN           7

#include <MySensors.h>
#include <Bounce2.h>

MyMessage doorUpMsg(CHILD_ID_DOOR_UP, V_TRIPPED);
MyMessage doorDownMsg(CHILD_ID_DOOR_DOWN, V_TRIPPED);

// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE


Bounce debouncerDown = Bounce(); 
Bounce debouncerUp = Bounce(); 

static bool toggleSwitch=false;
static uint8_t sentValueDown=2;
static uint8_t sentValueUp=2;

void before()
{
}

void setup()
{
  pinMode(DOOR_TOGGLE_RELAY_PIN, OUTPUT);   
  digitalWrite(DOOR_TOGGLE_RELAY_PIN, LOW); 

  // Window sensor that is closed when garage door is down
  pinMode(DOOR_DOWN_PIN, INPUT);   
  digitalWrite(DOOR_DOWN_PIN, HIGH);   // Activate internal pull-ups
  // After setting up the button, setup debouncer
  debouncerDown.attach(DOOR_DOWN_PIN);
  debouncerDown.interval(5);

  // Window sensor that is closed when garage door fully up
  pinMode(DOOR_UP_PIN, INPUT);   
  digitalWrite(DOOR_UP_PIN, HIGH);     // Activate internal pull-ups
  // After setting up the button, setup debouncer
  debouncerUp.attach(DOOR_UP_PIN);
  debouncerUp.interval(5);

}

void presentation()
{
    sendSketchInfo("Garage", "1.1");
    
    present(CHILD_ID_RELAY, S_BINARY);  
    present(CHILD_ID_DOOR_DOWN, S_DOOR);
    present(CHILD_ID_DOOR_UP, S_DOOR);
}

void loop()
{
  uint8_t value;

  // Have we received the order to toggle the Garge Door
  if(toggleSwitch == true) {
    digitalWrite(DOOR_TOGGLE_RELAY_PIN, HIGH);
    // Emulate the lenght of a human button push ;-)
    wait(500);
    digitalWrite(DOOR_TOGGLE_RELAY_PIN, LOW);
    toggleSwitch = false;
  }

  // Garage door fully down sensor
  debouncerDown.update();
  value = debouncerDown.read();
  if (value != sentValueDown) {
    // Value has changed from last transmission, send the updated value
    send(doorDownMsg.set(value==HIGH));
    sentValueDown = value;
  }

  // Garage door fully up sensor
  debouncerUp.update();
  value = debouncerUp.read();
  if (value != sentValueUp) {
    // Value has changed from last transmission, send the updated value
    send(doorUpMsg.set(value==HIGH));
    sentValueUp = value;
  }

}

void receive(const MyMessage &message)
{
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_STATUS) {
    toggleSwitch = true;    
  }
}