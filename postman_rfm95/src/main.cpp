#include <Arduino.h>
#include <avr/sleep.h>

// #define MY_DEBUG  // Uncomment for development/debug

#define MY_RADIO_RFM95
#define MY_RFM95_FIXED_FREQUENCY_868MHZ
#define MY_RFM95_ATC_TARGET_RSSI_DBM (-70) // target RSSI -70dBm
#define MY_RFM95_MAX_POWER_LEVEL_DBM (20)  // max. TX power 10dBm = 10mW  20dBm = 100mW

#define MY_RFM95_NETWORKID (100)
#define MY_NODE_ID 70

#include <MySensors.h>

#define SWITCH_PIN 3           // PD3, digital pin 3, INT1
#define STATUS_PIN A0          // PC0, digital out or analog in
#define BATTERY_VOLTAGE_PIN A6 // ADC6, analog in only
#define DIVIDER_CTRL 7         // PD7, digital pin 7

#define CHILD_ID_POST 1
#define CHILD_ID_SEND_RSSI 2
#define CHILD_ID_REC_RSSI 3
#define CHILD_ID_TX_POWER 4
#define CHILD_ID_BATTERY 5
#define CHILD_ID_TRIP_CNT 6

MyMessage msgPost(CHILD_ID_POST, V_TRIPPED);
MyMessage msgSendRssi(CHILD_ID_SEND_RSSI, V_LEVEL);
MyMessage msgRecRssi(CHILD_ID_REC_RSSI, V_LEVEL);
MyMessage msgTxPower(CHILD_ID_TX_POWER, V_LEVEL);
MyMessage msgBatteryVoltage(CHILD_ID_BATTERY, V_VOLTAGE);
MyMessage msgTripCnt(CHILD_ID_TRIP_CNT, V_KWH);

bool tripped = false;

#define SLEEP_TIME (12L * 60L * 60L * 1000L)  // Twice a day
uint32_t sleepTime = SLEEP_TIME;
int8_t wakeupReason = MY_WAKE_UP_BY_TIMER;

void setup() {
#ifdef MY_DEBUG
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Starting Postman Pat");
#endif

  pinMode(STATUS_PIN, OUTPUT);
  digitalWrite(STATUS_PIN, LOW);

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(DIVIDER_CTRL, OUTPUT);
  digitalWrite(DIVIDER_CTRL, LOW);

  analogReference(INTERNAL);  // 1.1V for AREF
}

void presentation() {
  sendSketchInfo("Postman Pat", "0.2");

  present(CHILD_ID_POST, S_MOTION);
  present(CHILD_ID_SEND_RSSI, S_SOUND);
  present(CHILD_ID_REC_RSSI, S_SOUND);
  present(CHILD_ID_TX_POWER, S_SOUND);
  present(CHILD_ID_BATTERY, S_MULTIMETER);
  present(CHILD_ID_BATTERY, S_MULTIMETER);
  present(CHILD_ID_TRIP_CNT, S_POWER);}


uint16_t tripCount = 0;

void loop() {
  wakeupReason = sleep(digitalPinToInterrupt(SWITCH_PIN), CHANGE, sleepTime);

#ifdef MY_DEBUG
  Serial.println("Woke up");
#endif

  digitalWrite(STATUS_PIN, HIGH);
  digitalWrite(DIVIDER_CTRL, HIGH);
  delay(10);  // optional stabilization

  int raw = analogRead(BATTERY_VOLTAGE_PIN);
  digitalWrite(DIVIDER_CTRL, LOW);
  float batteryVoltage = raw * 0.006628855;
  send(msgBatteryVoltage.set(batteryVoltage, 2));

  int16_t txRssi = RFM95_getSendingRSSI();
  int16_t rxRssi = RFM95_getReceivingRSSI();
  uint8_t txPower = RFM95_getTxPowerLevel();

#ifdef MY_DEBUG
  Serial.print("Battery: "); Serial.println(batteryVoltage);
  Serial.print("TX RSSI: "); Serial.println(txRssi);
  Serial.print("RX RSSI: "); Serial.println(rxRssi);
  Serial.print("TX Power: "); Serial.println(txPower);
  Serial.print("Trip cnt: "); Serial.println(tripCount);
#endif

  if (wakeupReason == digitalPinToInterrupt(SWITCH_PIN)) {
    sleepTime = getSleepRemaining();
    tripped = !tripped;
    send(msgPost.set(tripped ? "1" : "0"));
    tripCount++;
  } else {
    sleepTime = SLEEP_TIME;
  }  

  send(msgSendRssi.set(txRssi));
  send(msgRecRssi.set(rxRssi));
  send(msgTxPower.set(txPower));
  send(msgTripCnt.set(tripCount));


  digitalWrite(DIVIDER_CTRL, LOW); // disable voltage divider
  digitalWrite(STATUS_PIN, LOW);
  delay(100);
}
