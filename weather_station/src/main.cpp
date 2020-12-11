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


  D4 Wind speed is conected to 4.7k pull up resistor
  D5 Rain sensor is conected to 4.7k pull up resistor
  D6 is connected to 1-wire with 2.2k pull up resistor
*/
#include <OneWire.h>
#include <DS18B20.h>
//#define MY_DEBUG_VERBOSE_TRANSPORT_HAL
// Enable debug prints to serial monitor
#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_RFM69_REGISTERS
//#define MY_DEBUG_VERBOSE_RFM69
//#define MY_SIGNAL_REPORT_ENABLED

void(* resetFunc) (void) = 0;

#define MY_RADIO_RFM69
#define MY_RFM69_NEW_DRIVER
#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY (RFM69_433MHZ)
#define MY_RFM69_ATC_TARGET_RSSI_DBM (-70)  // target RSSI -70dBm
#define MY_RFM69_MAX_POWER_LEVEL_DBM (20)   // max. TX power 10dBm = 10mW
#define MY_RFM69_NETWORKID (45)
#define MY_NODE_ID 88

// Enable repeater functionality for this node
//#define MY_REPEATER_FEATURE

#define CHILD_ID_WIND         50
#define CHILD_ID_RAIN         51
#define CHILD_ID_TEMP         52
#define CHILD_ID_BATTERY      53
#define CHILD_ID_SOLAR_OUTPUT 54
#define CHILD_ID_CHARGING     55

#define CHILD_ID_DONE         57
#define CHILD_ID_SEND_RSSI    58
#define CHILD_ID_REC_RSSI     59
#define CHILD_ID_TX_POWER     60


#define WIND_SPEED_PIN         4
#define RAIN_SENSOR_PIN        5
#define ONE_WIRE_BUS           6
#define CHARGING_STATUS_PIN    7
#define DONE_STATUS_PIN        9

#define SOLAR_PANEL_SENSE_PIN  A0  // select the input pin for the solar panel output (if any). If DC plugged in it will show that voltage
#define BATTERY_SENSE_PIN      A1  // select the input pin for the battery sense point
#define WIND_DIRECTION_PIN     A2

uint32_t  MIN_TX_DELAY = 5 * 1000UL; // sleep time between reads (seconds * 1000 milliseconds)
uint32_t  MIN_TX_DELAY_INFREQUENT = 1 * 60 * 1000UL; // sleep time between reads (seconds * 1000 milliseconds)
int oldBatteryPcnt = 0;

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DS18B20  sensor(&oneWire);

// RAIN
#define RAIN_SLIDING_AVG_LEN       11
#define RAIN_SLIDING_AVG_THRESHOLD 5
int rainState = HIGH;
int lastRainStateArr[RAIN_SLIDING_AVG_LEN] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
int lastRainIndex = 0;
// Shared between ISR and apication code should be volatile
volatile unsigned long rainTipCount = 0;
#define UNDEF -1
unsigned long reportedRain = UNDEF;

// WIND
#define WIND_SLIDING_AVG_LEN       5
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

float reportedSolarVoltage = -3.0;
float reportedBatteryVoltage = -3.0;


int reportedChargingStatus = -1;
int reportedDoneStatus = -1;

unsigned long lastVoltageCheckTime = 0L;
unsigned long lastReportTime = 0L;

#include <MySensors.h>


unsigned long lap = 0;

MyMessage msgWSpeed(CHILD_ID_WIND, V_WIND);
MyMessage msgWGust(CHILD_ID_WIND, V_GUST);
MyMessage msgWDirection(CHILD_ID_WIND, V_DIRECTION);

MyMessage msgRain(CHILD_ID_RAIN, V_RAIN);

MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

MyMessage msgBatteryVoltage(CHILD_ID_BATTERY, V_VOLTAGE);
MyMessage msgSolarVoltage(CHILD_ID_SOLAR_OUTPUT, V_VOLTAGE);

MyMessage msgChargingStatus(CHILD_ID_CHARGING, V_STATUS);
MyMessage msgDoneStatus(CHILD_ID_DONE, V_STATUS);

MyMessage msgSendRssi(CHILD_ID_SEND_RSSI, V_LEVEL);
MyMessage msgRecRssi(CHILD_ID_REC_RSSI, V_LEVEL);
MyMessage msgTxPower(CHILD_ID_TX_POWER, V_LEVEL);

////////////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  pinMode(WIND_SPEED_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(ONE_WIRE_BUS, INPUT);
  pinMode(CHARGING_STATUS_PIN, INPUT);
  pinMode(DONE_STATUS_PIN, INPUT);

  sensor.begin();

  // TIMER 1 for interrupt frequency 2000 Hz:
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
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
ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  lastRainStateArr[lastRainIndex] = digitalRead(RAIN_SENSOR_PIN);
  lastRainIndex++;
  if (lastRainIndex >= RAIN_SLIDING_AVG_LEN) lastRainIndex = 0;
  int tot = 0;
  int i = 0;
  for (i = 0; i < RAIN_SLIDING_AVG_LEN; i++) {
    tot = tot + lastRainStateArr[i];
  }
  if (tot < RAIN_SLIDING_AVG_THRESHOLD) {
    if (rainState == HIGH) {
      rainTipCount++;
      rainState = LOW;
    }
  }
  else if (tot > RAIN_SLIDING_AVG_THRESHOLD) {
    if (rainState == LOW) {
      rainState = HIGH;
    }
  }

  lastWindStateArr[lastWindIndex] = digitalRead(WIND_SPEED_PIN);
  lastWindIndex++;
  if (lastWindIndex >= WIND_SLIDING_AVG_LEN) lastWindIndex = 0;
  tot = 0;
  for (i = 0; i < WIND_SLIDING_AVG_LEN; i++) {
    tot = tot + lastWindStateArr[i];
  }
  if (tot < WIND_SLIDING_AVG_THRESHOLD) {
    if (windState == HIGH) {
      windSpeedCount++;
      windState = LOW;
    }
  }
  else if (tot > WIND_SLIDING_AVG_THRESHOLD) {
    if (windState == LOW) {
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
  sendSketchInfo("Weather Station", "0.1");

  present(CHILD_ID_WIND, S_WIND);
  present(CHILD_ID_RAIN, S_RAIN);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_BATTERY, S_MULTIMETER);
  present(CHILD_ID_SOLAR_OUTPUT, S_MULTIMETER);
  present(CHILD_ID_CHARGING, S_BINARY);
  present(CHILD_ID_DONE, S_BINARY);

  present(CHILD_ID_SEND_RSSI, S_SOUND);
  present(CHILD_ID_REC_RSSI, S_SOUND);
  present(CHILD_ID_TX_POWER, S_SOUND);

}

////////////////////////////////////////////////////////////////////////////////////////
//
// Map Wind Direction (0-1023) to index 0-15 (1024 / 16 = 64)
// However normal map doesn't work since index 0 (north) is in range
// 0 +-32 (+-11.25 degrees)
////////////////////////////////////////////////////////////////////////////////////////
int map_direction(int deg) {
  int val = deg / 64;
  int reminder = deg - (val * 64);
  if (reminder >= 32) {
    val++;
  }

  if (val >= 16) val = 0;

  return val;
}

void loop() {


  // We need to call the transportDisable() after we have entered the loop(). Otherwise we

  //transportDisable();
  while (millis() - lastReportTime < MIN_TX_DELAY) {
    continues_task_radio_is_sleeping();
  }

  // Start the radio (we will most likely send something)
  //transportReInitialise();
  periodic_report_check();

 
  // Check voltage, RSSI less frequent
  if (millis() - lastVoltageCheckTime > MIN_TX_DELAY_INFREQUENT) {
    infrequent_periodic_report_check();
    lastVoltageCheckTime = millis();
  }
}

void continues_task_radio_is_sleeping() {
  bool sensorReady = sensor.begin();
  if(sensorReady != prevSensorState) {
    if(sensorReady) {
      sensor.setResolution(12);
      sensor.setConfig(DS18B20_CRC);  // or 1
      sensor.requestTemperatures();
    } else {
      // Sensor lost
      temp = UNDEFINED_TEMP;
      prevTemp = UNDEFINED_TEMP;
    }
    prevSensorState = sensorReady;

  } else if(sensorReady) {
    if (sensor.isConversionComplete()) {

      float val = sensor.getTempC();

      // We only update the temp if two consecutive readings are within 1.0 degrees
      if (fabs(val - prevTemp) <= 1.0) {
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
void periodic_report_check() {


  if(!isTransportReady()) {
    resetFunc();
  }
  
  unsigned long curr_millis = millis();


  // Clear values before sending as transmission might take time and miss a few samples
  // Do this atomically by temp disabling interrupts
  noInterrupts ();
  unsigned long tmpSpeedCount = windSpeedCount;
  windSpeedCount = 0;
  interrupts ();

  // 1600 rotations = 1mph i.e. V=P(2.25/T)
  // mph to m/s is 1:2.237 i.e. V=P/T for m/s
  // multiple count with 1000 dividing time (in ms) to not loose precision in ms -> s conversion

  float mps = (tmpSpeedCount * 1000.0);
  mps = mps / (float)(curr_millis - lastReportTime);


  lastReportTime = curr_millis;

  if (fabs(mps - reportedWind) >= 1.0) {
    send(msgWSpeed.set(mps, 1));
    reportedWind = mps;
  }

  int dirValue = analogRead(WIND_DIRECTION_PIN);

  int dirIndex = map_direction(dirValue);
  if (reportedDirectionIndex != dirIndex) {
    send(msgWDirection.set(22.5 * dirIndex, 1));
    reportedDirectionIndex = dirIndex;
  }

  //*********************************
  // Rain
  //*********************************
  if (rainTipCount != reportedRain) {


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
    send(msgRain.set(rainMetric, 1));
  }

  //*********************************
  // Temperature
  //*********************************
  if(temp != UNDEFINED_TEMP) {
    if (fabs(temp - reportedTemp) >= 0.1) {
     send(msgTemp.set(temp, 1));
     reportedTemp = temp;
    }
  }



  int chargingStatus = !digitalRead(CHARGING_STATUS_PIN);
  if (chargingStatus != reportedChargingStatus) {
    send(msgChargingStatus.set(chargingStatus));
    reportedChargingStatus = chargingStatus;
  }

  int doneStatus = !digitalRead(DONE_STATUS_PIN);
  if (doneStatus != reportedDoneStatus) {
    send(msgDoneStatus.set(doneStatus));
    reportedDoneStatus = doneStatus;
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

#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// infrequent_periodic_report_check runs every MIN_TX_DELAY
////////////////////////////////////////////////////////////////////////////////////////
void infrequent_periodic_report_check() {

  //*************************************************************************
  //**
  //** Solar Panel
  //**
  //************************************************************************
  // get the Solar Panel Voltage
  int solarValue = analogRead(SOLAR_PANEL_SENSE_PIN);

  // 820K, 1M divider for input (either DC or solar panel) should never exceed 6V
  // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
  // ((1M+820k)/1M)*3.3 = Vmax = 6.006 Volts
  // 6.006/1023 = Volts per bit = 0.0058709677419355
  float solarVolatage  = solarValue * 0.005870968;
  if (fabs(solarVolatage - reportedSolarVoltage) > 0.05) {
    send(msgSolarVoltage.set(solarVolatage, 2));
    reportedSolarVoltage = solarVolatage;
  }

#ifdef MY_DEBUG
  Serial.print("Solar Panel:");
  Serial.print(solarVolatage);
  Serial.println("V");
#endif

  //*************************************************************************
  //**
  //** Battery
  //**
  //************************************************************************
  // get the Battery Voltage (note if there is enough sun or a DC plug we will not consume any battery
  int batteryValue = analogRead(BATTERY_SENSE_PIN);

  // 360k, 1M divider across battery should not exceed 4.4V
  // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
  // ((1M+360k)/1M)*3.3 = Vmax = 4.488 Volts
  // 4.488/1023 = Volts per bit = 0.0043870967741935
  float batteryVolatage  = batteryValue * 0.004387097;
  if (fabs(batteryVolatage - reportedBatteryVoltage) > 0.05) {
    send(msgBatteryVoltage.set(batteryVolatage, 2));
    reportedBatteryVoltage = batteryVolatage;
  }

#ifdef MY_DEBUG
  Serial.print("Battery:");
  Serial.print(batteryVolatage);
  Serial.println("V");
#endif

  int16_t txRssi = RFM69_getSendingRSSI();
  send(msgSendRssi.set(txRssi));   
   int16_t rxRssi = RFM69_getReceivingRSSI();
  send(msgRecRssi.set(rxRssi));
  rfm69_powerlevel_t txPowerLevel = RFM69_getTxPowerLevel();
  send(msgTxPower.set(txPowerLevel));   

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
