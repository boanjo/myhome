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

// Enable debug prints
//#define MY_DEBUG

#define MY_RADIO_RFM95
#define MY_RFM95_FIXED_FREQUENCY_868MHZ
#define MY_RFM95_ATC_TARGET_RSSI_DBM (-70) // target RSSI -70dBm
#define MY_RFM95_MAX_POWER_LEVEL_DBM (13)  // max. TX power 10dBm = 10mW  20dBm = 100mW

#define MY_RFM95_NETWORKID (100)
#define MY_NODE_ID 60

#include <MySensors.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1

#define CHILD_ID_SEND_RSSI 10
#define CHILD_ID_REC_RSSI 11
#define CHILD_ID_TX_POWER 12
#define CHILD_ID_BATTERY 13

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

MyMessage msgSendRssi(CHILD_ID_SEND_RSSI, V_LEVEL);
MyMessage msgRecRssi(CHILD_ID_REC_RSSI, V_LEVEL);
MyMessage msgTxPower(CHILD_ID_TX_POWER, V_LEVEL);
MyMessage msgBatteryVoltage(CHILD_ID_BATTERY, V_VOLTAGE);

#define STATUS_PIN A0
#define BATTERY_VOLTAGE_PIN A6 // ADC6, analog in only
#define DIVIDER_CTRL 7         // PD7, digital pin 7

int oldBatteryPcnt = 0;
float reportedBatteryVoltage = -3.0;

static bool metric = true;

#define MAX_UPDATE_DELAY 480 // min
static const uint64_t UPDATE_INTERVAL = 60000;

#include "SI7021.h"
static SI7021 sensor;

float reportedTemp = -100.0;
int tmoCount = MAX_UPDATE_DELAY + 1;

void presentation()
{
  // Send the sketch info to the gateway
  sendSketchInfo("TemperatureAndHumidity", "1.3");

  // Present sensors as children to gateway
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  present(CHILD_ID_BATTERY, S_MULTIMETER);

  present(CHILD_ID_SEND_RSSI, S_SOUND);
  present(CHILD_ID_REC_RSSI, S_SOUND);
  present(CHILD_ID_TX_POWER, S_SOUND);
  metric = getControllerConfig().isMetric;
}

void setup()
{
  pinMode(STATUS_PIN, OUTPUT);
  digitalWrite(STATUS_PIN, LOW);
  pinMode(DIVIDER_CTRL, OUTPUT);
  digitalWrite(DIVIDER_CTRL, LOW);
  analogReference(INTERNAL);
  while (not sensor.begin())
  {
    Serial.println(F("Sensor not detected!"));
    delay(2000);
    digitalWrite(STATUS_PIN, HIGH);
    delay(2000);    
    digitalWrite(STATUS_PIN, LOW);
  }
}

void loop()
{
  // Read temperature & humidity from sensor.
  const float temperature = float(metric ? sensor.getCelsiusHundredths() : sensor.getFahrenheitHundredths()) / 100.0;
  const float humidity = float(sensor.getHumidityBasisPoints()) / 100.0;

#ifdef MY_DEBUG
  Serial.print(F("Temp "));
  Serial.print(temperature);
  Serial.print(metric ? 'C' : 'F');
  Serial.print(F("\tHum "));
  Serial.println(humidity);
#endif

  if ((fabs(temperature - reportedTemp) >= 0.2) || (tmoCount > MAX_UPDATE_DELAY))
  {


    send(msgTemp.set(temperature, 1));
    send(msgHum.set(humidity, 1));

    reportedTemp = temperature;
  }

  if (tmoCount > MAX_UPDATE_DELAY)
  {
      
    digitalWrite(STATUS_PIN, HIGH);
    digitalWrite(DIVIDER_CTRL, HIGH);
    delay(10);  // optional stabilization

    int raw = analogRead(BATTERY_VOLTAGE_PIN);
    digitalWrite(DIVIDER_CTRL, LOW);
    float batteryVoltage = raw * 0.006628855;

    int16_t txRssi = RFM95_getSendingRSSI();
    int16_t rxRssi = RFM95_getReceivingRSSI();
    uint8_t txPower = RFM95_getTxPowerLevel();

    #ifdef MY_DEBUG
    Serial.print("Battery: "); Serial.println(batteryVoltage);
    Serial.print("TX RSSI: "); Serial.println(txRssi);
    Serial.print("RX RSSI: "); Serial.println(rxRssi);
    Serial.print("TX Power: "); Serial.println(txPower);
    #endif

    send(msgBatteryVoltage.set(batteryVoltage, 2));
    send(msgSendRssi.set(txRssi));
    send(msgRecRssi.set(rxRssi));
    send(msgTxPower.set(txPower));


    digitalWrite(DIVIDER_CTRL, LOW); // disable voltage divider
    digitalWrite(STATUS_PIN, LOW);
    reportedBatteryVoltage = batteryVoltage;
      
    tmoCount = 0;
  }
  tmoCount++;

  // Sleep until next update to save energy
  sleep(UPDATE_INTERVAL);
}