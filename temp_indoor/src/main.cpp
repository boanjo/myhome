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

// Enable and select radio type attached
#define MY_RADIO_RFM69
#define MY_RFM69_NEW_DRIVER
//#define MY_IS_RFM69HW
#define MY_RFM69_FREQUENCY (RFM69_433MHZ)
#define MY_RFM69_ATC_TARGET_RSSI_DBM (-70) // target RSSI -70dBm
//#define MY_RFM69_MAX_POWER_LEVEL_DBM (20)   // max. TX power 10dBm = 10mW
#define MY_RFM69_NETWORKID (135)
#define MY_NODE_ID 39

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

int BATTERY_SENSE_PIN = A0;
int oldBatteryPcnt = 0;
float reportedBatteryVoltage = -3.0;

static bool metric = true;

#define MAX_UPDATE_DELAY 240 // min
static const uint64_t UPDATE_INTERVAL = 60000;

#include "SI7021.h"
static SI7021 sensor;

float reportedTemp = -100.0;
int tmoCount = MAX_UPDATE_DELAY + 1;

void presentation()
{
  // Send the sketch info to the gateway
  sendSketchInfo("TemperatureAndHumidity", "1.0");

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
  // use the 1.1 V internal reference
#if defined(__AVR_ATmega2560__)
  analogReference(INTERNAL1V1);
#else
  analogReference(INTERNAL);
#endif
  while (not sensor.begin())
  {
    Serial.println(F("Sensor not detected!"));
    delay(5000);
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

  
    tmoCount = 0;
  }
  tmoCount++;

  // Sleep until next update to save energy
  sleep(UPDATE_INTERVAL);
}