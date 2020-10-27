#include <Arduino.h>
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



  D4 Wind speed is conected to 4.7k pull up resistor
 
*/
//#define MY_DEBUG_VERBOSE_TRANSPORT_HAL
// Enable debug prints to serial monitor
//#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_RFM69_REGISTERS
//#define MY_DEBUG_VERBOSE_RFM69
//#define MY_SIGNAL_REPORT_ENABLED

void (*resetFunc)(void) = 0;

#define MY_RADIO_RFM69
#define MY_RFM69_NEW_DRIVER
#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY (RFM69_433MHZ)
#define MY_RFM69_ATC_TARGET_RSSI_DBM (-70) // target RSSI -70dBm
#define MY_RFM69_MAX_POWER_LEVEL_DBM (20)  // max. TX power 10dBm = 10mW
#define MY_RFM69_NETWORKID (135)
#define MY_NODE_ID 20

// Enable repeater functionality for this node
//#define MY_REPEATER_FEATURE

#define CHILD_ID_WIND 11
#define CHILD_ID_BATTERY 12
#define CHILD_ID_SOLAR_OUTPUT 13

#define CHILD_ID_SEND_RSSI 14
#define CHILD_ID_REC_RSSI 15
#define CHILD_ID_TX_POWER 16

#define WIND_SPEED_PIN 3
#define TIME_INT_PIN 5 // We recive a transition every second from the attiny85 (i.e. HIGH and one sec later LOW one second later HIGH...)

#define SOLAR_PANEL_SENSE_PIN A0 // select the input pin for the solar panel output (if any). If DC plugged in it will show that voltage
#define BATTERY_SENSE_PIN A1     // select the input pin for the battery sense point
#define WIND_DIRECTION_PIN A2

#define SAMPLE_PERIOD 4
#define MIN_TX_DELAY_INFREQUENT 150 // WDT 4sec x 150 = every 600 sec = every 10 min

int oldBatteryPcnt = 0;

void gotoSleep(void);
void periodic_report_check();
void infrequent_periodic_report_check();
void wakeOnInterrupt();
// WIND

// Shared between ISR and application code should be volatile
unsigned long windSpeedCount = 0;
float reportedWind = -3.0;
int reportedDirectionIndex = 0xff;

float reportedSolarVoltage = -3.0;
float reportedBatteryVoltage = -3.0;

unsigned long lastVoltageCheckTime = 0L;
unsigned long lastReportTime = 0L;

#include <MySensors.h>

volatile boolean extInterrupt; //external interrupt flag (wind reed switch...)
volatile boolean wdtInterrupt; //watchdog timer interrupt flag

unsigned long lap = 0;

MyMessage msgWSpeed(CHILD_ID_WIND, V_WIND);
MyMessage msgWGust(CHILD_ID_WIND, V_GUST);
MyMessage msgWDirection(CHILD_ID_WIND, V_DIRECTION);

MyMessage msgBatteryVoltage(CHILD_ID_BATTERY, V_VOLTAGE);
MyMessage msgSolarVoltage(CHILD_ID_SOLAR_OUTPUT, V_VOLTAGE);

MyMessage msgSendRssi(CHILD_ID_SEND_RSSI, V_LEVEL);
MyMessage msgRecRssi(CHILD_ID_REC_RSSI, V_LEVEL);
MyMessage msgTxPower(CHILD_ID_TX_POWER, V_LEVEL);

void pciSetup(byte pin) // enables pin change for given pin
{
  *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin)); // enable pin
  PCIFR |= bit(digitalPinToPCICRbit(pin));                   // clear any outstanding interrupt
  PCICR |= bit(digitalPinToPCICRbit(pin));                   // enable interrupt for the group
}
void pciClear(byte pin) // disables pin change for given pin
{
  *digitalPinToPCMSK(pin) &= ~(bit(digitalPinToPCMSKbit(pin))); // disable pin
  PCIFR |= bit(digitalPinToPCICRbit(pin));                      // clear any outstanding interrupt
  PCICR &= ~(bit(digitalPinToPCICRbit(pin)));                   // disable interrupt for the group
}

////////////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  pinMode(TIME_INT_PIN, INPUT);
  pinMode(WIND_SPEED_PIN, INPUT);
  pciSetup(TIME_INT_PIN);
  pciSetup(WIND_SPEED_PIN);
}

////////////////////////////////////////////////////////////////////////////////////////
// presentation
////////////////////////////////////////////////////////////////////////////////////////
void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Anemometer", "0.2");

  present(CHILD_ID_WIND, S_WIND);
  present(CHILD_ID_BATTERY, S_MULTIMETER);
  present(CHILD_ID_SOLAR_OUTPUT, S_MULTIMETER);

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

// Report after one WDT
uint32_t checkInfrequentCount = MIN_TX_DELAY_INFREQUENT;

void powerDown(void)
{
  ADCSRA &= ~(1 << ADEN); //disable ADC - 120uA
  sleep_bod_disable();    //disable bod  - 15uA
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  wdt_disable();
  sleep_enable();
  sleep_cpu();
  sleep_disable();
  ADCSRA |= (1 << ADEN); //enable ADC
}

ISR(PCINT2_vect) //pins d0-d7
{
}

byte prevTimeState = LOW;
byte prevWindSpeedState = LOW;
unsigned long seconds = 0;
void loop()
{

  while (1 == 1)
  {

    powerDown();
    if ((digitalRead(TIME_INT_PIN) == LOW) && (prevTimeState == HIGH))
    {
      seconds++;
      prevTimeState = LOW;
    }
    else if ((digitalRead(TIME_INT_PIN) == HIGH) && (prevTimeState == LOW))
    {
      seconds++;
      prevTimeState = HIGH;

      if (seconds >= SAMPLE_PERIOD)
      {
        seconds = 0;
        transportReInitialise();
        periodic_report_check();

        checkInfrequentCount++;
        // Check voltage, RSSI less frequent
        if (checkInfrequentCount > MIN_TX_DELAY_INFREQUENT)
        {
          infrequent_periodic_report_check();
          checkInfrequentCount = 0;
        }
      }
      transportDisable();
    }

    if ((digitalRead(WIND_SPEED_PIN) == LOW) && (prevWindSpeedState == HIGH))
    {
      prevWindSpeedState = LOW;
    }
    else if ((digitalRead(WIND_SPEED_PIN) == HIGH) && (prevWindSpeedState == LOW))
    {
      prevWindSpeedState = HIGH;
      windSpeedCount++;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////
// periodic_report_check runs every MIN_TX_DELAY
////////////////////////////////////////////////////////////////////////////////////////
void periodic_report_check()
{

  // I don't really know why but sometimes the initialize
  if (!isTransportReady())
  {
    resetFunc();
  }

  // Clear values before sending as transmission might take time and miss a few samples
  // Do this atomically by temp disabling interrupts
  unsigned long tmpSpeedCount = windSpeedCount;
  windSpeedCount = 0;

  // 1600 rotations = 1mph i.e. V=P(2.25/T)
  // mph to m/s is 1:2.237 i.e. V=P/T for m/s
  // multiple count with 1000 dividing time (in ms) to not loose precision in ms -> s conversion

  float mps = (tmpSpeedCount * 1000.0);
  mps = mps / (SAMPLE_PERIOD * 1000.0);

  if (fabs(mps - reportedWind) >= 1.0)
  {
    send(msgWSpeed.set(mps, 1));
    reportedWind = mps;
  }

  int dirValue = analogRead(WIND_DIRECTION_PIN);

  int dirIndex = map_direction(dirValue);
  if (reportedDirectionIndex != dirIndex)
  {
    send(msgWDirection.set(22.5 * dirIndex, 1));
    reportedDirectionIndex = dirIndex;
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
  Serial.flush();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
// infrequent_periodic_report_check runs every MIN_TX_DELAY
////////////////////////////////////////////////////////////////////////////////////////
void infrequent_periodic_report_check()
{

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
  float solarVolatage = solarValue * 0.005870968;
  if (fabs(solarVolatage - reportedSolarVoltage) > 0.05)
  {
    send(msgSolarVoltage.set(solarVolatage, 2));
    reportedSolarVoltage = solarVolatage;
  }

#ifdef MY_DEBUG
  Serial.print("Solar Panel:");
  Serial.print(solarVolatage);
  Serial.println("V");
  Serial.flush();
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
  float batteryVolatage = batteryValue * 0.004387097;
  if (fabs(batteryVolatage - reportedBatteryVoltage) > 0.05)
  {
    send(msgBatteryVoltage.set(batteryVolatage, 2));
    reportedBatteryVoltage = batteryVolatage;
  }

#ifdef MY_DEBUG
  Serial.print("Battery:");
  Serial.print(batteryVolatage);
  Serial.println("V");
  Serial.flush();
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
  Serial.flush();
#endif
}
