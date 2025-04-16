/*
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2019 Sensnology AB
   Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   DESCRIPTION


  OneWire (Temp sensor)
  Pull up resistor
  An indicative table for pull up resistors, (E12 series), to get started.

  Note: thicker wires require smaller resistors (typically 1 step in E12 series)
  Length        5.0 Volt  3.3 Volt
  10cm (4")     10K0      6K8
  20cm (8")     8K2       4K7
  50cm (20")    4K7       3K3
  100cm (3'4")  3K3       2K2
  200cm (6'8")  2K2       1K0
  500cm (16'8") 1K0       *
  longer        *         *

  D3 Fan control of Stevenson cage 2.2k in series to TIP120 anode
  D4 Wind speed is conected to 4.7k pull up resistor
  D5 Rain sensor is conected to 4.7k pull up resistor
  D6 is connected to 1-wire with 2.2k pull up resistor
  D7 is connected to LED button 470ohm resistor in series
*/
//#define FIRST_TIME 1

#include <avr/eeprom.h>
#include <Wire.h>
#include <OneWire.h>
#include <DS18B20.h>

#define MY_DEBUG_VERBOSE_TRANSPORT_HAL
// Enable debug prints to serial monitor
#define MY_DEBUG
#define MY_DEBUG_VERBOSE_RFM95_REGISTERS
#define MY_DEBUG_VERBOSE_RFM95
#define MY_SIGNAL_REPORT_ENABLED

void (*resetFunc)(void) = 0;

#define EE_RESET_COUNT_ADDRESS 200 // Select address in EEPROM memory space

#define ATTINY_ADDR 0x10 // I2C address of the ATTINY85


#define MY_RADIO_RFM95
#define MY_RFM95_FREQUENCY (RFM95_868MHZ)
#define RFM95_RETRY_TIMEOUT_MS  (300)
#define MY_RFM95_FREQUENCY      (868.0)  // or 915.0 depending on your hardware/region

//#define MY_RFM95_ATC_TARGET_RSSI_DBM (-70) // target RSSI -70dBm
//#define MY_RFM95_MAX_POWER_LEVEL_DBM (20)  // max. TX power 10dBm = 10mW  20dBm = 100mW

#define MY_RFM95_NETWORKID (100)
#define MY_NODE_ID 50

// Enable repeater functionality for this node
//#define MY_REPEATER_FEATURE

#define CHILD_ID_WIND 0
#define CHILD_ID_RAIN 1
#define CHILD_ID_TEMP 2

#define CHILD_ID_SEND_RSSI 10
#define CHILD_ID_REC_RSSI 11
#define CHILD_ID_TX_POWER 12
#define CHILD_ID_RESET_COUNT 13
#define CHILD_ID_REQ_COUNTERS 15
#define CHILD_ID_REQ_RESET 16

#define SEND(X)                       \
  digitalWrite(LED_STATUS_PIN, HIGH); \
  send(X);                            \
  digitalWrite(LED_STATUS_PIN, LOW);

/*
 
  OneWire (Temp sensor)
  Pull up resistor
  An indicative table for pull up resistors, (E12 series), to get started.

  Note: thicker wires require smaller resistors (typically 1 step in E12 series)
  Length        5.0 Volt  3.3 Volt
  10cm (4")     10K0      6K8
  20cm (8")     8K2       4K7
  50cm (20")    4K7       3K3
  100cm (3'4")  3K3       2K2
  200cm (6'8")  2K2       1K0
  500cm (16'8") 1K0       *
  longer        *         *

  D4 Wind speed is conected to 4.7k pull up resistor
  D6 is connected to 1-wire with 2.2k pull up resistor
  D7 is connected to LED button 470ohm resistor in series
*/
#define WIND_SPEED_PIN 4
#define ONE_WIRE_BUS 6
#define LED_STATUS_PIN A0
#define WIND_DIRECTION_PIN A2

uint32_t MIN_TX_DELAY = 5 * 1000UL;                 // sleep time between reads (seconds * 1000 milliseconds)
uint32_t MIN_TX_DELAY_INFREQUENT = 15 * 60 * 1000UL; // sleep time between reads (min * seconds * 1000 milliseconds)

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

// RAIN
unsigned long rainTipCount = 0;
#define UNDEF 0xFFFFFFFF
unsigned long reportedRain = UNDEF;
unsigned long lastRainReportTime = 0;

// WIND
#define WIND_SLIDING_AVG_LEN 5
#define WIND_SLIDING_AVG_THRESHOLD 3
int windState = HIGH;
int lastWindStateArr[WIND_SLIDING_AVG_LEN] = {HIGH, HIGH, HIGH, HIGH, HIGH};
int lastWindIndex = 0;

// Shared between ISR and application code should be volatile
volatile unsigned long windSpeedCount = 0;
float reportedWind = -3.0;
int reportedDirectionIndex = 0xff;

// TEMP
float UNDEFINED_TEMP = -100.0;
float reportedTemp = UNDEFINED_TEMP;
float temp = UNDEFINED_TEMP;
float prevTemp = UNDEFINED_TEMP;
bool prevSensorState = false;

bool forcedFetch = false;

unsigned long resetCount = 0;
unsigned long reportedResetCount = -1;

unsigned long lastReportTime = 0L;
unsigned long lastPeriodicCheckTime = 0L;
unsigned long lastInfrequentCheckTime = 0L;

#include <MySensors.h>

unsigned long lap = 0;

MyMessage msgWSpeed(CHILD_ID_WIND, V_WIND);
MyMessage msgWGust(CHILD_ID_WIND, V_GUST);
MyMessage msgWDirection(CHILD_ID_WIND, V_DIRECTION);
MyMessage msgRain(CHILD_ID_RAIN, V_RAIN);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

MyMessage msgResetCount(CHILD_ID_RESET_COUNT, V_KWH);
MyMessage msgSendRssi(CHILD_ID_SEND_RSSI, V_LEVEL);
MyMessage msgRecRssi(CHILD_ID_REC_RSSI, V_LEVEL);
MyMessage msgTxPower(CHILD_ID_TX_POWER, V_LEVEL);

void readRainCounter();
void background_task();
void periodic_report_check();
void infrequent_periodic_report_check();

////////////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  wait(5000); 
#ifdef FIRST_TIME
  eeprom_write_dword((uint32_t *)EE_RESET_COUNT_ADDRESS, (uint32_t)0);
#endif

  resetCount = (unsigned long)eeprom_read_dword((const uint32_t *)EE_RESET_COUNT_ADDRESS);
  resetCount++;
  eeprom_write_dword((uint32_t *)EE_RESET_COUNT_ADDRESS, (uint32_t)(resetCount));

  pinMode(WIND_SPEED_PIN, INPUT);
  pinMode(ONE_WIRE_BUS, INPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);

  digitalWrite(LED_STATUS_PIN, LOW);

  sensor.begin();

  // TIMER 1 for interrupt frequency 2000 Hz:
  cli();      // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1 = 0;  // initialize counter value to 0
  // set compare match register for 2000 Hz increments
  OCR1A = 3999; // = 8000000 / (1 * 2000) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 1 prescaler
  TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts
}

////////////////////////////////////////////////////////////////////////////////////////
// Timer1 executes every 0,5 ms (2kHz)
//
// Rain:
// The reed switch pulse for rain is a bit slower and can generate a few more bounces
// hence a longer slidig average compared to wind. According to Sainsmart the pulse
// might go upto 90ms in duration, but i have never seen that length (usually 20-40ms)
//
// Wind
// The reed switch pulse for wind is low for (roughly) 1/6 of a full rotation and one
// rotation per second equals 1m/s. So for 50Hz (50m/s) the pulse width is 3.2ms
// This is why we sample every 0.5ms and have a sliding average that changes every
// 3-4 samples (1.5 - 2.0ms) to be able to capture also such short pulses. We should be
// fine for wind speeds above 50m/s too but it think we have other more serious
// things to think of if we miss a few samples then ;-)
//
//   50Hz = 20ms cycle ttot:    |<-------------- ttot ------------------>|
//                      ________    _____________________________________    _________
//                              |t1|                 t2                  |t1|
//                              ----                                     ----
//                        t1=20ms/6=3.33ms    t2=10/6*5=16.67ms
//
////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER1_COMPA_vect) // timer compare interrupt service routine
{
  
  int tot = 0;
  int i = 0;

  lastWindStateArr[lastWindIndex] = digitalRead(WIND_SPEED_PIN);
  lastWindIndex++;
  if (lastWindIndex >= WIND_SLIDING_AVG_LEN)
    lastWindIndex = 0;
  tot = 0;
  for (i = 0; i < WIND_SLIDING_AVG_LEN; i++)
  {
    tot = tot + lastWindStateArr[i];
  }
  if (tot < WIND_SLIDING_AVG_THRESHOLD)
  {
    if (windState == HIGH)
    {
      windSpeedCount++;
      windState = LOW;
    }
  }
  else if (tot > WIND_SLIDING_AVG_THRESHOLD)
  {
    if (windState == LOW)
    {
      windState = HIGH;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////
// presentation
////////////////////////////////////////////////////////////////////////////////////////
void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Weather Station 3.0", "0.1");

  present(CHILD_ID_WIND, S_WIND);
  present(CHILD_ID_RAIN, S_RAIN);
  present(CHILD_ID_TEMP, S_TEMP);

  present(CHILD_ID_REQ_COUNTERS, S_BINARY);
  present(CHILD_ID_REQ_RESET, S_BINARY);

  present(CHILD_ID_SEND_RSSI, S_SOUND);
  present(CHILD_ID_REC_RSSI, S_SOUND);
  present(CHILD_ID_TX_POWER, S_SOUND);
  present(CHILD_ID_RESET_COUNT, S_POWER);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Map Wind Direction (0-1023) to index 0-15 (1024 / 16 = 64)
// However normal map doesn't work since index 0 (north) is in range
// 0 +-32 (+-11.25 degrees)
////////////////////////////////////////////////////////////////////////////////////////
int map_direction(int deg)
{
  int val = deg / 64;
  int reminder = deg - (val * 64);
  if (reminder >= 32)
  {
    val++;
  }

  if (val >= 16)
    val = 0;

  return val;
}

void loop()
{

  unsigned long curr_millis = millis();
  background_task();

  if ((curr_millis - lastPeriodicCheckTime > MIN_TX_DELAY) || forcedFetch)
  {
    periodic_report_check();
    lastPeriodicCheckTime = curr_millis;
  }

  // Check voltage, RSSI less frequent
  if ((curr_millis - lastInfrequentCheckTime > MIN_TX_DELAY_INFREQUENT) || forcedFetch)
  {
    infrequent_periodic_report_check();
    lastInfrequentCheckTime = curr_millis;
  }

  forcedFetch = false;
}

////////////////////////////////////////////////////////////////////////////////////////
// Here we are when when we have nothing else to do
////////////////////////////////////////////////////////////////////////////////////////
void background_task()
{
  bool sensorReady = sensor.begin();
  if (sensorReady != prevSensorState)
  {
    if (sensorReady)
    {
      sensor.setResolution(12);
      sensor.setConfig(DS18B20_CRC); // or 1
      sensor.requestTemperatures();
    }
    else
    {
      // Sensor lost
      temp = UNDEFINED_TEMP;
      prevTemp = UNDEFINED_TEMP;
    }
    prevSensorState = sensorReady;
  }
  else if (sensorReady)
  {
    if (sensor.isConversionComplete())
    {

      float val = sensor.getTempC();

      // We only update the temp if two consecutive readings are within 1.0 degrees
      if (fabs(val - prevTemp) <= 1.0)
      {
        temp = val;
      }

      prevTemp = val;

      // Send a new command to get temperature readings
      sensor.requestTemperatures();
    }
  }



}
////////////////////////////////////////////////////////////////////////////////////////
// periodic_report_check runs every MIN_TX_DELAY
////////////////////////////////////////////////////////////////////////////////////////
void periodic_report_check()
{

  unsigned long curr_millis = millis();



 // Read the rain counter from the ATTINY85 note this is not atomic and might be interrupted by the
 readRainCounter();
 
  //*********************************
  // Rain
  //*********************************
  if ((rainTipCount != reportedRain) || forcedFetch)
  {

    // First we report the previous reported rain amount. Otherwise the diff within a day will miss the first 
    // tip that day. I.e. since we only report at a change and it might takes days before some rain, then the
    // first tip will send in a value like 23.4. But both min and max will say tha same if there is only one 
    // tip that day, diff=0.0. Hence we first send the previous value and then the new value
    if(reportedRain != UNDEF) {
      float rainMetric = 0.254 * reportedRain;
      send(msgRain.set(rainMetric, 1));
    }

    reportedRain = rainTipCount;

    float rainMetric = 0.254 * reportedRain;

    if (!isTransportReady())
    {
      resetFunc();
    }

    SEND(msgRain.set(rainMetric, 1));
    lastRainReportTime = curr_millis;
  }

  //*********************************
  // Wind
  //*********************************

  // Clear values before sending as transmission might take time and miss a few samples
  // Do this atomically by temp disabling interrupts
  noInterrupts();
  unsigned long tmpSpeedCount = windSpeedCount;
  windSpeedCount = 0;
  interrupts();

  // 1600 rotations = 1mph i.e. V=P(2.25/T)
  // mph to m/s is 1:2.237 i.e. V=P/T for m/s
  // multiple count with 1000 dividing time (in ms) to not loose precision in ms -> s conversion

  float mps = (tmpSpeedCount * 1000.0);
  mps = mps / (float)(curr_millis - lastReportTime);

  lastReportTime = curr_millis;

  if ((fabs(mps - reportedWind) >= 0.5) || forcedFetch)
  {
    SEND(msgWSpeed.set(mps, 1));
    reportedWind = mps;
  }

  int dirValue = analogRead(WIND_DIRECTION_PIN);

  int dirIndex = map_direction(dirValue);
  if ((reportedDirectionIndex != dirIndex) || forcedFetch)
  {
    SEND(msgWDirection.set(22.5 * dirIndex, 1));
    reportedDirectionIndex = dirIndex;
  }

  //*********************************
  // Temperature
  //*********************************
  if (temp != UNDEFINED_TEMP)
  {
    if ((fabs(temp - reportedTemp) >= 0.1) || forcedFetch)
    {
      SEND(msgTemp.set(temp, 1));
      reportedTemp = temp;
    }
  }

  if ((resetCount != reportedResetCount) || forcedFetch)
  {
    reportedResetCount = resetCount;
    SEND(msgResetCount.set(resetCount, 1));
  }

  lap++;

  
#ifdef MY_DEBUG

  Serial.println("");
  Serial.print("Lap:");
  Serial.println(lap);
  Serial.print("Speed:");
  Serial.println(tmpSpeedCount);
  Serial.print("m/s:");
  Serial.println(mps);
  Serial.print("fabs:");
  Serial.println(fabs(mps - reportedWind));
  Serial.print("reportedWind:");
  Serial.println(reportedWind);

  Serial.print("Direction:");
  Serial.print(dirValue);
  Serial.println("");

  Serial.print("Rain:");
  Serial.print(rainTipCount);
  Serial.println("");

  Serial.print("Temp:");
  Serial.print(temp, 1);
  Serial.println("");
  Serial.print("resetCount:");
  Serial.print(resetCount);
  Serial.println("");

#endif

}

////////////////////////////////////////////////////////////////////////////////////////
// infrequent_periodic_report_check runs every MIN_TX_DELAY
////////////////////////////////////////////////////////////////////////////////////////
void infrequent_periodic_report_check()
{

  int16_t txRssi = RFM95_getSendingRSSI();
  SEND(msgSendRssi.set(txRssi));
  int16_t rxRssi = RFM95_getReceivingRSSI();
  SEND(msgRecRssi.set(rxRssi));
  uint8_t txPowerLevel = RFM95_getTxPowerLevel();
  SEND(msgTxPower.set(txPowerLevel));

#ifdef MY_DEBUG
  Serial.print("TxRSSI:");
  Serial.print(txRssi);
  Serial.println("");
  Serial.print("RxRSSI:");
  Serial.print(rxRssi);
  Serial.println("");
  Serial.print("TxPowerLevel:");
  Serial.print(txPowerLevel);
  Serial.println("");

#endif
}

void readRainCounter() {
  Wire.requestFrom(ATTINY_ADDR, 4);
  if (Wire.available() == 4) {
    uint32_t count = 0;
    count |= ((uint32_t)Wire.read()) << 24;
    count |= ((uint32_t)Wire.read()) << 16;
    count |= ((uint32_t)Wire.read()) << 8;
    count |= Wire.read();

    rainTipCount = count;
  }
}

void receive(const MyMessage &message)
{
  Serial.println("******************* message.getType()");
  Serial.println(message.getType());
  Serial.println(message.sensor);
  Serial.println("*******************");
  if ((message.getType() == V_STATUS) && (message.sensor == CHILD_ID_REQ_COUNTERS))
  {
    forcedFetch = true;
    Serial.println("Forced Fetch of counters");
  }
  else if ((message.getType() == V_STATUS) && (message.sensor == CHILD_ID_REQ_RESET))
  {
    Serial.println("Forced Reset");
    resetFunc();
  }
  else
  {
    Serial.print("Unknown type: ");
    Serial.println(message.getType());
  }
}
