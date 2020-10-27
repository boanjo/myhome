/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2015 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   REVISION HISTORY
   Version 1.0: Yveaux

   DESCRIPTION
   This sketch provides an example of how to implement a humidity/temperature
   sensor using a Si7021 sensor.

   For more information, please visit:
   http://www.mysensors.org/build/humiditySi7021

*/

#include <avr/eeprom.h>

//NOTE!!!!!  DON'T FORGET COMMENT OUT FIRST TIME TO INITIALIZE THE EE MEMORY
//#define FIRST_TIME 1

// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RFM69
#define MY_RFM69_NEW_DRIVER
#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY (RFM69_433MHZ)
#define MY_RFM69_ATC_TARGET_RSSI_DBM (-70) // target RSSI -70dBm
#define MY_RFM69_MAX_POWER_LEVEL_DBM (20)  // max. TX power 10dBm = 10mW
#define MY_RFM69_NETWORKID (135)
#define MY_NODE_ID 21

#include <MySensors.h>

#define CHILD_ID_RAIN 1
#define CHILD_ID_SEND_RSSI 10
#define CHILD_ID_REC_RSSI 11
#define CHILD_ID_TX_POWER 12
#define CHILD_ID_BATTERY 13

#define EE_RAIN_COUNT_ADDRESS 204 // Select address in EEPROM memory space
#define PIN_RAIN_SENSOR 3
int BATTERY_SENSE_PIN = A0;
int oldBatteryPcnt = 0;
float reportedBatteryVoltage = -3.0;

volatile unsigned long rainTipCount = 0;
unsigned long reportedRain = -1;

// Sleep time between sensor updates (in milliseconds)
static const uint64_t UPDATE_INTERVAL = 60000 * 240; // 240min or 4h max between reports

MyMessage msgRain(CHILD_ID_RAIN, V_RAIN);
MyMessage msgSendRssi(CHILD_ID_SEND_RSSI, V_LEVEL);
MyMessage msgRecRssi(CHILD_ID_REC_RSSI, V_LEVEL);
MyMessage msgTxPower(CHILD_ID_TX_POWER, V_LEVEL);
MyMessage msgBatteryVoltage(CHILD_ID_BATTERY, V_VOLTAGE);

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

void presentation()
{
  // Send the sketch info to the gateway
  sendSketchInfo("Rain", "1.0");

  // Present sensors as children to gateway
  present(CHILD_ID_RAIN, S_RAIN);
  present(CHILD_ID_BATTERY, S_MULTIMETER);

  present(CHILD_ID_SEND_RSSI, S_SOUND);
  present(CHILD_ID_REC_RSSI, S_SOUND);
  present(CHILD_ID_TX_POWER, S_SOUND);
}

void setup()
{
  pinMode(PIN_RAIN_SENSOR, INPUT);
  pciSetup(PIN_RAIN_SENSOR);

#ifdef FIRST_TIME
  eeprom_write_dword((uint32_t *)EE_RAIN_COUNT_ADDRESS, (uint32_t)0);
#endif

  rainTipCount = (unsigned long)eeprom_read_dword((const uint32_t *)EE_RAIN_COUNT_ADDRESS);

  // use the 1.1 V internal reference
#if defined(__AVR_ATmega2560__)
  analogReference(INTERNAL1V1);
#else
  analogReference(INTERNAL);
#endif
}

void reportRain()
{
  reportedRain = rainTipCount;
  eeprom_write_dword((uint32_t *)EE_RAIN_COUNT_ADDRESS, (uint32_t)rainTipCount);

  float rainMetric = 0.254 * reportedRain;

  send(msgRain.set(rainMetric, 1));
#ifdef MY_DEBUG
  Serial.print(F("tip "));
  Serial.print(reportedRain);
  Serial.print(F("\tMetric "));
  Serial.println(rainMetric);
#endif
}

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
  if (digitalRead(PIN_RAIN_SENSOR) == LOW)
  {
    rainTipCount++;
  }
}

void loop()
{
  while (1 == 1)
  {
    transportDisable();
    powerDown();
    transportReInitialise();

    reportRain();

    // get the battery Voltage
    int batteryValue = analogRead(BATTERY_SENSE_PIN);
#ifdef MY_DEBUG
    Serial.println(batteryValue);
#endif

    // 1M, 470K divider across battery and using internal ADC ref of 1.1V
    // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
    // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
    // 3.44/1023 = Volts per bit = 0.003363075

    int batteryPcnt = batteryValue / 10;
    float batteryVolatage = batteryValue * 0.003363075;
    if (fabs(batteryVolatage - reportedBatteryVoltage) > 0.05)
    {
      send(msgBatteryVoltage.set(batteryVolatage, 2));
      reportedBatteryVoltage = batteryVolatage;
    }
#ifdef MY_DEBUG
    Serial.print("Battery Voltage: ");
    Serial.print(batteryVolatage);
    Serial.println(" V");

    Serial.print("Battery percent: ");
    Serial.print(batteryPcnt);
    Serial.println(" %");
#endif

    if (oldBatteryPcnt != batteryPcnt)
    {
      sendBatteryLevel(batteryPcnt);
      oldBatteryPcnt = batteryPcnt;
    }

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
    wait(1);
#endif
  }
}